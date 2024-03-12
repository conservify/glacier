package main

import (
	"encoding/binary"
	"fmt"
	"image"
	"image/color"
	"image/png"
	"io"
	"log"
	"math"
	"os"
	"runtime"
	"strings"
	"time"
)

type Sample struct {
	X float32
	Y float32
	Z float32
}

const (
	SamplesPerSecond = 60
	SamplesPerFile   = SamplesPerSecond * 60
	SamplesPerHour   = SamplesPerSecond * 60 * 60
	MaximumSamplesPerSecond = 500
	MaximumSamplesPerFile   = MaximumSamplesPerSecond * 60
	MaximumSamplesPerHour   = MaximumSamplesPerSecond * 60 * 60
)

func mapFloat(v, oMin, oMax, nMin, nMax float64) float64 {
	x := (v - oMin) / (oMax - oMin)
	return x*(nMax-nMin) + nMin
}

func mapInt(v, oMin, oMax, nMin, nMax int) int {
	x := float64(v-oMin) / float64(oMax-oMin)
	return int(x*float64(nMax-nMin) + float64(nMin))
}

type ByFileTime []*ArchiveFile

func (a ByFileTime) Len() int           { return len(a) }
func (a ByFileTime) Swap(i, j int)      { a[i], a[j] = a[j], a[i] }
func (a ByFileTime) Less(i, j int) bool { return a[i].Time.Unix() < a[j].Time.Unix() }

type Samples struct {
	ArchiveFile *ArchiveFile
	HourOffset  int
	Samples     []Sample
}

func (af *ArchiveFile) ReadSamples() (*Samples, error) {
	df, err := os.Open(af.FileName)
	if err != nil {
		return nil, fmt.Errorf("Error opening %s: %v", af.FileName, err)
	}

	defer df.Close()

	samples := make([]Sample, MaximumSamplesPerFile)
	if err := binary.Read(df, binary.LittleEndian, &samples); err != nil {
		return nil, fmt.Errorf("Error reading %s: %v", af.FileName, err)
	}

	if MaximumSamplesPerSecond != SamplesPerSecond {
		truncated := make([]Sample, SamplesPerFile)
		for second := 0; second < 60; second += 1 {
			for sample := 0; sample < SamplesPerSecond; sample += 1 {
				truncated[second * SamplesPerSecond + sample] = samples[second * MaximumSamplesPerSecond + sample]
			}
		}
		samples = truncated
	}

	hourOffset := (af.Second + af.Minute*60) * SamplesPerSecond

	return &Samples{
		ArchiveFile: af,
		HourOffset:  int(hourOffset),
		Samples:     samples[:],
	}, nil
}

type Rendering struct {
	Image *image.RGBA
}

func NewRendering(cx, cy int) *Rendering {
	i := image.NewRGBA(image.Rect(0, 0, cx, cy))

	fill(i)

	return &Rendering{
		Image: i,
	}
}

func (gr *Rendering) Clear() {
	fill(gr.Image)
}

type AnalyzedSamples struct {
	Samples []float64
	Minimum float64
	Maximum float64
}

func (gr *Rendering) Analyze(axis string, samples []Sample) *AnalyzedSamples {
	log.Printf("Extracting...")

	selected := make([]float64, len(samples))
	switch axis {
	case "x":
		for i, sample := range samples {
			selected[i] = float64(sample.X)
		}
		break
	case "y":
		for i, sample := range samples {
			selected[i] = float64(sample.Y)
		}
		break
	case "z":
		for i, sample := range samples {
			selected[i] = float64(sample.Z)
		}
		break
	}

	log.Printf("Calculating range...")

	minimum := math.MaxFloat64
	maximum := -math.MaxFloat64
	for i := 0; i < len(selected); i += 1 {
		if selected[i] > maximum {
			maximum = selected[i]
		}
		if selected[i] < minimum {
			minimum = selected[i]
		}
	}
	return &AnalyzedSamples{
		Samples: selected,
		Minimum: minimum,
		Maximum: maximum,
	}
}

type Column struct {
	amp float64
	min float64
	max float64
	n int32
}

func (gr *Rendering) DrawSamples(axis string, samples []Sample, rowNumber, numberOfRows int, strictScaling bool) error {
	fast := runtime.GOARCH == "arm"

	log.Printf("Analyzing (fast = %v, strict = %v)...", fast, strictScaling)

	as := gr.Analyze(axis, samples)

	numberOfSamples := len(as.Samples)
	bounds := gr.Image.Bounds()
	dy := bounds.Dy()
	rowCy := (dy / (numberOfRows + 1))
	numberColumns := bounds.Dx()
	offsetY := dy / 2
	if numberOfRows > 1 {
		offsetY = (rowNumber + 1) * rowCy
	}
	scale := 1.0 / float64(numberOfRows)

	if numberOfRows == 1 {
		scale = 1.0 / 6.0
	}

	log.Printf("Analyzed [%f, %f] (%d samples) (%d columns)...", as.Minimum, as.Maximum, len(as.Samples), numberColumns)

	cd := NewColumnDrawer(gr.Image)
	lines := color.RGBA{128, 128, 128, 255}
	linesHeavy := color.RGBA{32, 32, 32, 255}

	if true {
		columns := make([]Column, numberColumns)
		for i, sample := range as.Samples {
			x := mapInt(i, 0, numberOfSamples, 0, numberColumns)
			if sample != 0 {
				if sample > 0 {
					if sample > columns[x].max {
						// columns[x].max += sample
						columns[x].max = sample
					}
				} else {
					if sample < columns[x].min {
						// columns[x].min += sample
					 columns[x].min = sample
					}
				}
				if math.Abs(sample) > math.Abs(columns[x].amp) {
					columns[x].amp = sample
				}
				// columns[x].n += 1
				columns[x].n = 1
			}
		}

		waveform := color.RGBA{255, 0, 0, 255}
		for x, column := range columns {
			min := int(column.min / float64(column.n) * scale)
			max := int(column.max / float64(column.n) * scale)
			MapToColor(column.amp, as.Minimum, as.Maximum, &waveform)
			cd.DrawColumn(x, offsetY+min, offsetY+max, &waveform, fast)
		}
		cd.DrawRow(offsetY, 0, numberColumns, &linesHeavy)
	} else {
		waveform := color.RGBA{255, 0, 0, 255}
		for i, sample := range as.Samples {
			x := mapInt(i, 0, numberOfSamples, 0, bounds.Dx())
			y := sample * scale
			if strictScaling {
				y = mapFloat(sample, as.Minimum, as.Maximum, float64(-rowCy/2), float64(rowCy/2))
			}

			if fast {
				cd.DrawColumn(x, offsetY, offsetY+int(y), &waveform, fast)
			} else {
				MapToColor(sample, as.Minimum, as.Maximum, &waveform)
				cd.DrawColumn(x, offsetY, offsetY+int(y), &waveform, fast)
			}
		}
	}

	for c := 0; c < 60; c += 1 {
		x := mapInt(c, 0, 60, 0, bounds.Dx())
		if c%10 == 0 {
			cd.DrawColumn(x, 0, bounds.Dy(), &linesHeavy, true)
		} else {
			cd.DrawColumn(x, 0, bounds.Dy(), &lines, true)
		}
	}

	log.Printf("Done drawing")

	return nil
}

func (gr *Rendering) Encode(w io.Writer) error {
	png.Encode(w, gr.Image)

	return nil
}

func (gr *Rendering) SaveTo(fileName string) error {
	f, err := os.OpenFile(fileName, os.O_WRONLY|os.O_CREATE, 0600)
	if err != nil {
		return err
	}

	defer f.Close()

	png.Encode(f, gr.Image)

	return nil
}

type HourlyRendering struct {
	*Rendering
	Axis          string
	StrictScaling bool
	SaveFiles     bool
	Start         *time.Time
	RowNumber     int
	NumberOfRows  int
}

func NewHourlyRendering(axis string, numberOfRows, cx, cy int, strictScaling bool, saveFiles bool) *HourlyRendering {
	return &HourlyRendering{
		Axis:          strings.ToLower(axis),
		Rendering:     NewRendering(cx, cy),
		Start:         nil,
		NumberOfRows:  numberOfRows,
		StrictScaling: strictScaling,
		SaveFiles:     saveFiles,
	}
}

func (r *HourlyRendering) DrawHour(hour int64, files []*ArchiveFile) error {
	hourBegin := time.Unix(hour, 0).UTC()

	if r.Start == nil {
		r.Start = &hourBegin
	}

	samples := make([]Sample, 0)

	for _, af := range files {
		s, err := af.ReadSamples()
		if err != nil {
			return err
		}
		missing := s.HourOffset - len(samples)
		if missing > 0 {
			log.Printf("%v: Gap of %v samples (%v seconds)", af.Time, missing, missing/SamplesPerSecond)
		}
		for i := 0; i < missing; i += 1 {
			samples = append(samples, Sample{})
		}
		samples = append(samples, s.Samples...)
	}

	for i := len(samples); i < SamplesPerHour; i += 1 {
		samples = append(samples, Sample{})
	}

	r.DrawSamples(r.Axis, samples, r.RowNumber, r.NumberOfRows, r.StrictScaling)

	r.RowNumber += 1

	if r.RowNumber == r.NumberOfRows {
		if r.SaveFiles {
			err := r.Save()
			if err != nil {
				return err
			}
		}
	}
	return nil
}

func (r *HourlyRendering) ToFileName(hour int64) string {
	t := time.Unix(hour, 0).UTC()
	s := t.Format("20060102_150405")
	flags := r.Axis
	if r.StrictScaling {
		flags += "_ss"
	}
	return fmt.Sprintf("frame_%s_%d_%s.png", flags, r.NumberOfRows, s)
}

func (r *HourlyRendering) Exists(hour int64) bool {
	name := r.ToFileName(hour)
	if _, err := os.Stat(name); os.IsNotExist(err) {
		return false
	}

	return true
}

func (r *HourlyRendering) DrawAll(afs *ArchiveFileSet, overwrite bool) error {
	grouped := make(map[int64][]int64)
	start := int64(0)
	for _, h := range afs.Hours {
		if start == 0 || len(grouped[start]) == 12 {
			start = h
			grouped[start] = make([]int64, 0)
		}
		grouped[start] = append(grouped[start], h)
	}

	for start, hours := range grouped {
		if overwrite {
			if r.Exists(start) {
				log.Printf("Skipping %v", time.Unix(start, 0))
				continue
			}
		}
		for _, h := range hours {
			files := afs.Hourly[h]
			log.Printf("Adding hour = %v files = %v", time.Unix(h, 0).UTC(), len(files))

			s := time.Now()
			r.DrawHour(h, files)
			elapsed := time.Now().Sub(s)

			log.Printf("Elapsed: %v", elapsed)
		}
	}

	return nil
}

func (r *HourlyRendering) EncodeTo(w io.Writer) error {
	r.Encode(w)

	return nil
}

func (r *HourlyRendering) Save() error {
	if r.Start != nil {
		name := r.ToFileName(r.Start.Unix())
		log.Printf("Saving %s", name)
		r.SaveTo(name)

		r.Clear()

		r.RowNumber = 0
		r.Start = nil
	}
	return nil
}
