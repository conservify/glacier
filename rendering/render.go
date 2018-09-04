package main

import (
	"encoding/binary"
	"flag"
	"fmt"
	"image"
	"image/png"
	"io"
	"io/ioutil"
	"log"
	"os"
	"path"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
	"time"

	"encoding/json"
	"net/http"

	"github.com/fsnotify/fsnotify"
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
		return nil, fmt.Errorf("Error finding time in filename: %s", fileName)
	}

	t, err := time.Parse("20060102150405", matches[0][1])
	if err != nil {
		return nil, fmt.Errorf("Error parsing time: %v", err)
	}

	hour := time.Date(t.Year(), t.Month(), t.Day(), t.Hour(), 0, 0, 0, t.Location()).UTC()

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
	selected := make([]float64, 0)
	minimum := float64(0.0)
	maximum := float64(0.0)
	for i, sample := range samples {
		value := 0.0
		switch axis {
		case "x":
			value = float64(sample.X)
		case "y":
			value = float64(sample.Y)
		case "z":
			value = float64(sample.Z)
		}
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

func (gr *Rendering) DrawSamples(axis string, samples []Sample, rowNumber, numberOfRows int, strictScaling bool) error {
	as := gr.Analyze(axis, samples)

	log.Printf("Analyzed [%f, %f]", as.Minimum, as.Maximum)

	dy := gr.Image.Bounds().Dy()
	rowCy := (dy / (numberOfRows + 1))
	offsetY := dy / 2
	if numberOfRows > 1 {
		offsetY = (rowNumber + 1) * rowCy
	}
	numberOfSamples := len(as.Samples)
	scale := 1.0 / float64(numberOfRows)

	log.Printf("Drawing (%v)...", len(as.Samples))
	for i, sample := range as.Samples {
		x := mapInt(i, 0, numberOfSamples, 0, gr.Image.Bounds().Dx())
		y := sample * scale
		if strictScaling {
			y = mapFloat(sample, as.Minimum, as.Maximum, float64(-rowCy/2), float64(rowCy/2))
		}

		clr := MapToColor(sample, as.Minimum, as.Maximum)
		drawColumn(gr.Image, x, offsetY, offsetY+int(y), clr)
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

	log.Printf("Draw samples...")
	r.DrawSamples(r.Axis, samples, r.RowNumber, r.NumberOfRows, r.StrictScaling)
	log.Printf("Done")

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

func (r *HourlyRendering) SaveFrame(w io.Writer) error {
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

func (r *HourlyRendering) DrawMostRecent(afs *ArchiveFileSet) error {
	selected := int64(0)
	for _, h := range afs.Hours {
		if selected == 0 || selected < h {
			selected = h
		}
	}

	if selected == 0 {
		log.Printf("No data yet.")
		return nil
	}

	files := afs.Hourly[selected]
	log.Printf("Adding hour = %v files = %v", time.Unix(selected, 0).UTC(), len(files))
	r.DrawHour(selected, files)

	return nil
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
		if r.Exists(start) {
			if !overwrite {
				log.Printf("Skipping %v", time.Unix(start, 0))
				continue
			}
		}
		for _, h := range hours {
			files := afs.Hourly[h]
			log.Printf("Adding hour = %v files = %v", time.Unix(h, 0).UTC(), len(files))
			r.DrawHour(h, files)
		}
	}

	return nil
}

type DataWatcher struct {
	Watcher *fsnotify.Watcher
}

func NewDataWatcher(dir string) (dw *DataWatcher, err error) {
	watcher, err := fsnotify.NewWatcher()
	if err != nil {
		return nil, err
	}

	dw = &DataWatcher{
		Watcher: watcher,
	}

	err = dw.WatchRecursively(dir)
	if err != nil {
		return nil, err
	}

	return
}

func (dw *DataWatcher) WatchRecursively(dir string) error {
	log.Printf("Watching '%v'", dir)

	err := dw.Watcher.Add(dir)
	if err != nil {
		return err
	}

	entries, err := ioutil.ReadDir(dir)
	if err != nil {
		return err
	}

	for _, e := range entries {
		if e.IsDir() {
			child := filepath.Join(dir, e.Name())

			err = dw.WatchRecursively(child)
			if err != nil {
				return err
			}
		}
	}

	return nil
}

func (dw *DataWatcher) Close() {
	dw.Watcher.Close()
}

func (dw *DataWatcher) Watch() {
	go func() {
		for {
			select {
			case event, ok := <-dw.Watcher.Events:
				if !ok {
					return
				}
				log.Println("Notify: ", event)
				if event.Op&fsnotify.Write == fsnotify.Write {
					log.Println("Modified file: ", event.Name)
				}
			case err, ok := <-dw.Watcher.Errors:
				if !ok {
					return
				}
				log.Println("Error: ", err)
			}
		}
	}()
}

func ServeData(dw *DataWatcher) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		state := make(map[string]string)

		b, _ := json.Marshal(state)
		w.Header().Set("Content-Type", "application/json")
		w.Write(b)
	}
}

func ServeRendering(dw *DataWatcher) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "image/png")

		afs := NewArchiveFileSet()
		afs.AddFrom("/Users/jlewallen/conservify/glacier/data/201808/01")

		hr := NewHourlyRendering("x", 1, 60*60*2, 512, false, false)
		hr.DrawMostRecent(afs)
		hr.SaveFrame(w)
	}
}

func watchAndServe(dir string, web string) error {
	dw, err := NewDataWatcher(dir)
	if err != nil {
		return err
	}

	defer dw.Close()

	dw.Watch()

	http.Handle("/", http.FileServer(http.Dir(web)))
	http.HandleFunc("/data.json", ServeData(dw))
	http.HandleFunc("/rendering.png", ServeRendering(dw))
	err = http.ListenAndServe(":9090", nil)
	if err != nil {
		return err
	}

	return nil
}

type options struct {
	Overwrite     bool
	StrictScaling bool
	Axis          string
	Watch         string
	Web           string
	Cx            int
	Cy            int
}

func main() {
	o := options{}

	flag.StringVar(&o.Web, "web", "./", "web")
	flag.StringVar(&o.Watch, "watch", "", "watch")
	flag.StringVar(&o.Axis, "axis", "x", "axis")
	flag.IntVar(&o.Cx, "cx", 60*60*2, "width")
	flag.IntVar(&o.Cy, "cy", 2000, "height")

	flag.BoolVar(&o.StrictScaling, "strict-scaling", false, "strict scaling")
	flag.BoolVar(&o.Overwrite, "overwrite", false, "overwite existing frames")

	flag.Parse()

	if o.Watch != "" {
		err := watchAndServe(o.Watch, o.Web)
		if err != nil {
			panic(err)
		}
		return
	}

	afs := NewArchiveFileSet()

	for _, arg := range flag.Args() {
		if err := afs.AddFrom(arg); err != nil {
			panic(err)
		}
	}

	if len(afs.Files) == 0 {
		return
	}

	log.Printf("Number of files: %v", len(afs.Files))
	log.Printf("Number of hours: %v", len(afs.Hours))
	log.Printf("Range: %v - %v", afs.Start, afs.End)

	hr := NewHourlyRendering(o.Axis, 12, o.Cx, o.Cy, o.StrictScaling, true)
	hr.DrawAll(afs, o.Overwrite)

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
	unix := af.Hour.Unix()
	afs.Files = append(afs.Files, af)
	value := afs.Hourly[unix]
	if value == nil {
		value = make([]*ArchiveFile, 0)
		afs.Hours = append(afs.Hours, unix)
		sort.Slice(afs.Hours, func(i, j int) bool {
			return afs.Hours[i] < afs.Hours[j]
		})
	}
	value = append(value, af)
	afs.Hourly[unix] = value

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
		if f != nil && !f.IsDir() {
			if filepath.Ext(p) == ".bin" {
				if af, err := NewArchiveFile(p); err != nil {
					return err
				} else {
					return afs.Add(af)
				}
			}
		}
		return nil
	})
}
