package main

import (
	"bytes"
	"context"
	"encoding/binary"
	"flag"
	"fmt"
	"io"
	"log"
	"math"
	"net/http"
	"os"
	"path"
	"regexp"
	"sort"
	"strconv"
	"strings"
	"time"

	"github.com/fsnotify/fsnotify"
	"github.com/go-audio/audio"
	"github.com/go-audio/wav"
	"github.com/jpillora/backoff"
)

type Config struct {
	Url     string
	Token   string
	Pattern string
	File    string
	Watch   bool
}

type WavBinary struct {
	Path             string
	SamplesPerSecond int
	InputId          int
	StartTime        time.Time
}

func NewWavBinary(filePath string) (*WavBinary, error) {
	re := regexp.MustCompile(`.*(\d{14}).*`)

	matches := re.FindAllStringSubmatch(strings.Replace(path.Base(filePath), "_", "", -1), -1)
	if len(matches) == 0 {
		return nil, fmt.Errorf("error finding timestamp")
	}

	startTime, err := time.Parse("20060102150405", matches[0][1])
	if err != nil {
		return nil, fmt.Errorf("error parsing timestamp")
	}

	return &WavBinary{
		Path:             filePath,
		StartTime:        startTime,
		SamplesPerSecond: 500,
		InputId:          0,
	}, nil
}

func (b *WavBinary) upload(ctx context.Context, config *Config) error {
	f, err := os.OpenFile(b.Path, os.O_RDONLY, 0666)
	if err != nil {
		return fmt.Errorf("error opening file: %w", err)
	}

	defer f.Close()

	decoder := wav.NewDecoder(f)

	decoder.ReadInfo()
	if decoder.Err() != nil {
		return fmt.Errorf("error opening file: %w", err)
	}

	numberOfChannels := decoder.Format().NumChannels
	sampleRate := decoder.Format().SampleRate
	duration, err := decoder.Duration()
	if err != nil {
		return fmt.Errorf("error opening file: %w", err)
	}

	log.Printf("number-of-channels = %v\n", numberOfChannels)
	log.Printf("sample-rate = %v\n", sampleRate)
	log.Printf("bit-depth = %v\n", decoder.SampleBitDepth())
	log.Printf("duration = %v\n", duration)

	data := bytes.NewBuffer(make([]byte, 0))
	total := 0
	bufferSize := 10000 * 3
	buf := &audio.IntBuffer{Data: make([]int, bufferSize), Format: decoder.Format()}
	for err == nil {
		n, err := decoder.PCMBuffer(buf)
		if err != nil {
			return fmt.Errorf("error reading PCM: %w", err)
		}
		if n == 0 {
			break
		}

		temp := buf.AsFloat32Buffer()
		for i := 0; i < n; i += 1 {
			bits := math.Float32bits(temp.Data[i])
			err := binary.Write(data, binary.LittleEndian, bits)
			if err != nil {
				return fmt.Errorf("error writing to buffer: %w", err)
			}
		}
		total += n
	}

	log.Printf("%d samples (%d bytes)\n", total, data.Len())

	log.Printf("uploading %s to %s (%d)", b.Path, config.Url, b.InputId)

	req, err := http.NewRequest("POST", config.Url, data)
	if err != nil {
		return err
	}

	req.Header.Set("content-type", "application/octet-stream")
	req.Header.Set("x-token", config.Token)
	req.Header.Set("x-timestamp", strconv.FormatInt(b.StartTime.Unix(), 10))
	req.Header.Set("x-frequency", strconv.Itoa(b.SamplesPerSecond))
	req.Header.Set("x-input-id", strconv.Itoa(b.InputId))
	req.Header.Set("x-format", "float32,float32,float32")
	req.Header.Set("x-filename", path.Base(b.Path))

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return err
	}

	defer resp.Body.Close()
	_, err = io.ReadAll(resp.Body)
	if err != nil {
		log.Printf("Error reading HTTP response: %v\n", err)
	}

	if resp.Status != "200" {
		log.Printf("%s:\n", resp.Status)
	}

	return nil
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
	files, err := os.ReadDir(directory)
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

					_ = child
					/*
						if !uploadSingleFile(child, config, b) {
							d := b.Duration()
							log.Printf("Sleeping for %v", d)
							time.Sleep(d)
						}
					*/
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
	flag.StringVar(&config.File, "file", "", "upload a single file")
	flag.BoolVar(&config.Watch, "watch", false, "watch directory for changes")

	flag.Parse()

	if config.Token == "" || config.Url == "" || (config.Pattern == "" && config.File == "") {
		flag.Usage()
		os.Exit(1)
	}

	ctx := context.Background()

	if config.File != "" {
		file, err := NewWavBinary(config.File)
		if err != nil {
			log.Printf("Error opening wav file: %v", err)
		} else {
			if err := file.upload(ctx, &config); err != nil {
				log.Printf("Upload error: %v", err)
			}
		}
	} else if config.Pattern != "" {
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
					case <-watcher.Events:
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
					log.Printf("Error %v", err)
				}
			}
			if err != nil {
				log.Fatalf("Error %v", err)
			}

			<-done
		}
	}
}
