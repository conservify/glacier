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
	"strings"
	"time"
)

type Sample struct {
	X float32
	Y float32
	Z float32
}

const (
	SamplesPerHour = 500 * 60 * 60
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
	Samples     []Sample
}

func (af *ArchiveFile) ReadSamples() (*Samples, error) {
	df, err := os.Open(af.FileName)
	if err != nil {
		return nil, fmt.Errorf("Error opening %s: %v", af.FileName, err)
	}

	defer df.Close()

	samplesPerFile := 500 * 60
	samples := make([]Sample, samplesPerFile)
	if err := binary.Read(df, binary.LittleEndian, &samples); err != nil {
		return nil, fmt.Errorf("Error reading %s: %v", af.FileName, err)
	}

	return &Samples{
		ArchiveFile: af,
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

func (gr *Rendering) DrawSamples(axis string, samples []Sample, rowNumber, numberOfRows int, strictScaling bool) error {
	log.Printf("Analyzing...")

	as := gr.Analyze(axis, samples)

	dy := gr.Image.Bounds().Dy()
	rowCy := (dy / (numberOfRows + 1))
	offsetY := dy / 2
	if numberOfRows > 1 {
		offsetY = (rowNumber + 1) * rowCy
	}
	numberOfSamples := len(as.Samples)
	scale := 1.0 / float64(numberOfRows)

	if numberOfRows == 1 {
		scale = 1.0 / 6.0
	}

	log.Printf("Analyzed [%f, %f] (%d), drawing...", as.Minimum, as.Maximum, len(as.Samples))

	cd := NewColumnDrawer(gr.Image)
	bounds := gr.Image.Bounds()
	waveform := color.RGBA{255, 0, 0, 255}
	fast := true

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

	// lines := color.RGBA{192, 192, 192, 255}
	lines := color.RGBA{128, 128, 128, 255}
	for c := 0; c < 60; c += 1 {
		x := mapInt(c, 0, 60, 0, bounds.Dx())
		cd.DrawColumn(x, 0, bounds.Dy(), &lines, true)
	}

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

func (r *HourlyRendering) DrawHour(hour int64, files []*ArchiveFile) {
	if r.Start == nil {
		t := time.Unix(hour, 0).UTC()
		r.Start = &t
	}

	samples := make([]Sample, 0)

	for _, af := range files {
		s, err := af.ReadSamples()
		if err != nil {
			panic(err)
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
			r.Save()
		}
	}
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
