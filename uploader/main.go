package main

import (
	"bytes"
	"encoding/binary"
	"flag"
	"github.com/fsnotify/fsnotify"
	"github.com/jpillora/backoff"
	"io/ioutil"
	"log"
	"log/syslog"
	"net/http"
	"os"
	"path"
	"regexp"
	"sort"
	"strconv"
	"strings"
	"time"
)

type Config struct {
	Url     string
	Token   string
	Pattern string
	LogFile string
	Syslog  string
	Watch   bool
	Archive bool
}

func writeSimplifiedBinary(filePath string, binaryPath string) (parsed *KinemetricsFile, err error) {
	fp, err := os.Open(filePath)
	if err != nil {
		return nil, err
	}

	defer fp.Close()

	parsed, err = ParseKinemetricsFile(fp)
	if err != nil {
		log.Fatalf("Error parsing Kinemetrics file %v", err)
	}

	binFile, err := os.OpenFile(binaryPath, os.O_CREATE|os.O_WRONLY, 0600)
	if err != nil {
		return nil, err
	}

	defer binFile.Close()

	for sample := 0; sample < parsed.NumberOfSamples; sample++ {
		for channel := 0; channel < 3; channel++ {
			binary.Write(binFile, binary.BigEndian, parsed.Streams[channel].Data[sample])
		}
	}

	return
}

type WaveformBinary struct {
	SamplesPerSecond int
	InputId          int
	StartTime        time.Time
}

func NewKinemetricsBinary(file *KinemetricsFile) (binary *WaveformBinary) {
	binary = new(WaveformBinary)
	binary.StartTime = file.StartTime
	binary.SamplesPerSecond = file.SamplesPerSecond
	binary.InputId = 1

	return
}

func NewGeophoneBinary(filePath string) (binary *WaveformBinary) {
	re := regexp.MustCompile(".*(\\d{14}).*")

	matches := re.FindAllStringSubmatch(strings.Replace(path.Base(filePath), "_", "", -1), -1)
	if len(matches) == 0 {
		return nil
	}

	startTime, err := time.Parse("20060102150405", matches[0][1])
	if err != nil {
		return nil
	}

	binary = new(WaveformBinary)
	binary.StartTime = startTime
	binary.SamplesPerSecond = 500
	binary.InputId = 0

	return
}

func uploadBinary(file *WaveformBinary, filePath string, config *Config) (err error) {
	data, err := ioutil.ReadFile(filePath)
	if err != nil {
		return err
	}

	log.Printf("Uploading %s to %s (%d)", filePath, config.Url, file.InputId)

	req, err := http.NewRequest("POST", config.Url, bytes.NewBuffer(data))
	if err != nil {
		return err
	}

	req.Header.Set("content-type", "application/octet-stream")
	req.Header.Set("x-token", config.Token)
	req.Header.Set("x-timestamp", strconv.FormatInt(file.StartTime.Unix(), 10))
	req.Header.Set("x-frequency", strconv.Itoa(file.SamplesPerSecond))
	req.Header.Set("x-input-id", strconv.Itoa(file.InputId))
	req.Header.Set("x-format", "float32,float32,float32")
	req.Header.Set("x-filename", path.Base(filePath))

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return err
	}

	defer resp.Body.Close()
	_, err = ioutil.ReadAll(resp.Body)
	log.Printf("%s:\n", resp.Status)

	return
}

func getArchivePath(file *WaveformBinary, baseDirectory string) string {
	yearMonth := file.StartTime.Format("200601")
	day := file.StartTime.Format("02")
	hour := file.StartTime.Format("15")
	base := "archive"
	return path.Join(baseDirectory, base, yearMonth, day, hour)
}

func archiveBinary(file *WaveformBinary, filePath string, config *Config) (err error) {
	newPath := getArchivePath(file, path.Dir(filePath))
	if os.MkdirAll(newPath, 0777) == nil {
		err := os.Rename(filePath, path.Join(newPath, path.Base(filePath)))
		if err == nil {
			log.Printf("Archived %s.", filePath)
		} else {
			log.Printf("Unable to archive %s: %s", filePath, err)
		}
	} else {
		log.Printf("Error creating directory %s, failed to archive %s", newPath, filePath)
	}

	return
}

type PendingFile struct {
	Name  string
	Path  string
	Stamp time.Time
}

type ByStamp []PendingFile

func (s ByStamp) Len() int {
	return len(s)
}

func (s ByStamp) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s ByStamp) Less(i, j int) bool {
	return s[i].Stamp.Unix() > s[j].Stamp.Unix()
}

func readFiles(directory string, config *Config) (pending []PendingFile, err error) {
	re := regexp.MustCompile(config.Pattern)
	files, err := ioutil.ReadDir(directory)
	if err != nil {
		log.Fatalf("Error listing directory %v", err)
	}

	pending = make([]PendingFile, 0)

	for _, child := range files {
		if !child.IsDir() {
			childPath := path.Join(directory, child.Name())
			matches := re.FindAllStringSubmatch(child.Name(), -1)
			if len(matches) > 0 {
				stamp := time.Now()
				pending = append(pending, PendingFile{
					Name:  child.Name(),
					Path:  childPath,
					Stamp: stamp,
				})
			}
		}
	}

	sort.Sort(ByStamp(pending))

	return
}

func uploadSingleFile(child PendingFile, config *Config, b *backoff.Backoff) bool {
	if strings.EqualFold(path.Ext(child.Name), ".evt") {
		binaryPath := child.Path + ".bin"

		if _, err := os.Stat(binaryPath); os.IsNotExist(err) {
			log.Printf("Processing %s...", child.Path)

			parsed, err := writeSimplifiedBinary(child.Path, binaryPath)
			if err != nil {
				log.Printf("Error writing binary %s", err)
			}

			binary := NewKinemetricsBinary(parsed)
			if binary != nil {
				if err := uploadBinary(binary, binaryPath, config); err != nil {
					log.Printf("Error uploading %s", err)
				} else {
					b.Reset()
					return true
				}
			}
		}
	} else if config.Archive {
		binary := NewGeophoneBinary(child.Path)
		if binary != nil {
			if err := uploadBinary(binary, child.Path, config); err != nil {
				log.Printf("Error uploading %s", err)
			} else {
				b.Reset()
				if err := archiveBinary(binary, child.Path, config); err != nil {
					log.Printf("Error archiving %s", err)
				}
				return true
			}
		} else {
			log.Printf("Unable to parse Geophone file name: %s", child.Path)
		}
	}
	return false
}

func scanDirectories(paths []string, config *Config, b *backoff.Backoff) {
	for {
		anyFiles := false

		for _, filePath := range paths {
			fi, err := os.Stat(filePath)
			if err != nil {
				log.Fatalf("Error %v", err)
			}
			switch mode := fi.Mode(); {
			case mode.IsDir():
				files, err := readFiles(filePath, config)
				if err != nil {
					log.Fatalf("Error listing directory %v", err)
				}

				for _, child := range files {
					anyFiles = true
					if !uploadSingleFile(child, config, b) {
						d := b.Duration()
						log.Printf("Sleeping for %v", d)
						time.Sleep(d)
					}
				}
			}
		}

		if !anyFiles {
			break
		}
	}
}

func main() {
	var config Config

	flag.StringVar(&config.Url, "url", "", "url to upload data to")
	flag.StringVar(&config.Token, "token", "", "upload token")
	flag.StringVar(&config.Pattern, "pattern", "", "upload pattern")
	flag.StringVar(&config.LogFile, "log", "uploader.log", "log file")
	flag.StringVar(&config.Syslog, "syslog", "", "enable syslog and name the ap")

	flag.BoolVar(&config.Watch, "watch", false, "watch directory for changes")
	flag.BoolVar(&config.Archive, "archive", false, "archive files after upload")

	flag.Parse()

	if flag.NArg() == 0 || config.Token == "" || config.Url == "" || config.Pattern == "" {
		flag.Usage()
		os.Exit(1)
	}

	if config.Syslog == "" {
		f, err := os.OpenFile(config.LogFile, os.O_RDWR|os.O_CREATE|os.O_APPEND, 0666)
		if err != nil {
			log.Fatalf("Error opening file: %v", err)
		}
		defer f.Close()

		log.SetOutput(f)
	} else {
		syslog, err := syslog.New(syslog.LOG_NOTICE, config.Syslog)
		if err == nil {
			log.SetOutput(syslog)
		}
	}

	b := &backoff.Backoff{
		Min:    100 * time.Millisecond,
		Max:    5 * 60 * time.Second,
		Factor: 2,
		Jitter: false,
	}

	scanDirectories(flag.Args(), &config, b)

	if config.Watch {
		watcher, err := fsnotify.NewWatcher()
		if err != nil {
			log.Fatalf("Error creating watcher %v", err)
		}
		defer watcher.Close()

		done := make(chan bool)
		go func() {
			for {
				select {
				case _ = <-watcher.Events:
					scanDirectories(flag.Args(), &config, b)
				case err := <-watcher.Errors:
					log.Println("Error:", err)
				}
			}
		}()

		for _, filePath := range flag.Args() {
			fi, err := os.Stat(filePath)
			if err != nil {
				log.Fatalf("Error %v", err)
			}
			switch mode := fi.Mode(); {
			case mode.IsDir():
				err = watcher.Add(filePath)
			}
		}
		if err != nil {
			log.Fatalf("Error %v", err)
		}

		<-done
	}
}
