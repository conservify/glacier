package main

import (
	"encoding/binary"
	"fmt"
	"image"
	"image/color"
	"image/png"
	"log"
	"math"
	"os"
	"path"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
	"time"
)

type Sample struct {
	X float32
	Y float32
	Z float32
}

type Image struct {
	W int
	H int
}

func mapFloat(v, oMin, oMax, nMin, nMax float64) float64 {
	x := (v - oMin) / (oMax - oMin)
	return x*(nMax-nMin) + nMin
}

func mapInt(v, oMin, oMax, nMin, nMax int) int {
	x := float64(v-oMin) / float64(oMax-oMin)
	return int(x*float64(nMax-nMin) + float64(nMin))
}

type ArchiveFile struct {
	Time     *time.Time
	Hour     *time.Time
	FileName string
}

type ByFileTime []*ArchiveFile

func (a ByFileTime) Len() int           { return len(a) }
func (a ByFileTime) Swap(i, j int)      { a[i], a[j] = a[j], a[i] }
func (a ByFileTime) Less(i, j int) bool { return a[i].Time.Unix() < a[j].Time.Unix() }

func NewArchiveFile(fileName string) (*ArchiveFile, error) {
	re := regexp.MustCompile(".*(\\d{14}).*")
	matches := re.FindAllStringSubmatch(strings.Replace(path.Base(fileName), "_", "", -1), -1)
	if len(matches) == 0 {
		return nil, fmt.Errorf("Error finding time in filename.")
	}

	t, err := time.Parse("20060102150405", matches[0][1])
	if err != nil {
		return nil, fmt.Errorf("Error parsing time: %v", err)
	}

	hour := time.Date(t.Year(), t.Month(), t.Day(), t.Hour(), 0, 0, 0, t.Location())

	return &ArchiveFile{
		Time:     &t,
		Hour:     &hour,
		FileName: fileName,
	}, nil
}

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

type AnalyzedSamples struct {
	Samples []float64
	Minimum float64
	Maximum float64
}

func analyze(samples []Sample) *AnalyzedSamples {
	selected := make([]float64, 0)
	minimum := float64(0.0)
	maximum := float64(0.0)
	for i, sample := range samples {
		value := float64(sample.X)
		if i == 0 || value > maximum {
			maximum = value
		}
		if i == 0 || value < minimum {
			minimum = value
		}
		selected = append(selected, value)
	}
	return &AnalyzedSamples{
		Samples: selected,
		Minimum: minimum,
		Maximum: maximum,
	}
}

type Rendering struct {
	Image *image.RGBA
}

func NewRendering() *Rendering {
	i := image.NewRGBA(image.Rect(0, 0, 3000, 2000))

	fill(i)

	return &Rendering{
		Image: i,
	}
}

func (gr *Rendering) Clear() {
	fill(gr.Image)
}

func (gr *Rendering) DrawSamples(samples []Sample, rowNumber, numberOfRows int) error {
	as := analyze(samples)

	dy := gr.Image.Bounds().Dy()
	offsetY := dy / 2
	if numberOfRows > 1 {
		// offsetY = mapInt(rowNumber, 0, numberOfRows+1, 50, dy-50) + (dy / (numberOfRows + 1))
		offsetY = (rowNumber + 1) * (dy / (numberOfRows + 1))
	}
	numberOfSamples := len(as.Samples)
	scale := 1.0 / float64(numberOfRows)
	for i, sample := range as.Samples {
		x := mapInt(i, 0, numberOfSamples, 0, gr.Image.Bounds().Dx())
		y := sample * scale
		if false {
			y = mapFloat(sample, as.Minimum, as.Maximum, -500, 500)
		}

		hue := mapFloat(math.Pow(math.Abs(sample), 1), 0, math.Pow(400, 1), 0, 255)
		r, g, b := hsbToRgb(hue, 255, 255)
		drawColumn(gr.Image, x, offsetY, offsetY+int(y), color.RGBA{r, g, b, 255})
	}

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
	Start        *time.Time
	RowNumber    int
	NumberOfRows int
}

func NewHourlyRendering(numberOfRows int) *HourlyRendering {
	return &HourlyRendering{
		Rendering:    NewRendering(),
		Start:        nil,
		NumberOfRows: numberOfRows,
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

	r.DrawSamples(samples, r.RowNumber, r.NumberOfRows)

	r.RowNumber += 1

	if r.RowNumber == r.NumberOfRows {
		r.Save()
	}
}

func (r *HourlyRendering) Save() error {
	if r.Start != nil {
		s := r.Start.Format("20060102_150405")
		name := fmt.Sprintf("frame_%d_%s.png", r.NumberOfRows, s)
		log.Printf("Saving %s", name)
		r.SaveTo(name)

		r.Clear()

		r.RowNumber = 0
		r.Start = nil
	}
	return nil
}

func main() {
	afs := NewArchiveFileSet()
	afs.AddFrom("../23")

	log.Printf("Number of files: %v", len(afs.Files))
	log.Printf("Number of hours: %v", len(afs.Hours))
	log.Printf("Range: %v - %v", afs.Start, afs.End)

	hr := NewHourlyRendering(12)

	for _, h := range afs.Hours {
		files := afs.Hourly[h]

		log.Printf("hour = %v files = %v", time.Unix(h, 0).UTC(), len(files))

		hr.DrawHour(h, files)
	}

	hr.Save()
}

type ArchiveFileSet struct {
	Files  []*ArchiveFile
	Hours  []int64
	Hourly map[int64][]*ArchiveFile
	Start  *time.Time
	End    *time.Time
}

func NewArchiveFileSet() *ArchiveFileSet {
	return &ArchiveFileSet{
		Files:  make([]*ArchiveFile, 0),
		Hourly: make(map[int64][]*ArchiveFile),
	}
}

func (afs *ArchiveFileSet) Add(af *ArchiveFile) error {
	afs.Files = append(afs.Files, af)
	value := afs.Hourly[af.Hour.Unix()]
	if value == nil {
		value = make([]*ArchiveFile, 0)
		afs.Hours = append(afs.Hours, af.Hour.Unix())
		sort.Slice(afs.Hours, func(i, j int) bool {
			return afs.Hours[i] < afs.Hours[j]
		})
	}
	value = append(value, af)
	afs.Hourly[af.Hour.Unix()] = value

	if afs.Start == nil || afs.Start.After(*af.Time) {
		afs.Start = af.Time
	}
	if afs.End == nil || afs.End.Before(*af.Time) {
		afs.End = af.Time
	}

	return nil
}

func (afs *ArchiveFileSet) AddFrom(path string) error {
	log.Printf("Starting, reading %s...", path)
	return filepath.Walk(path, func(p string, f os.FileInfo, err error) error {
		if !f.IsDir() {
			if af, err := NewArchiveFile(p); err != nil {
				return err
			} else {
				return afs.Add(af)
			}
		}
		return nil
	})
}
