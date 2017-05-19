package main

import (
	"bytes"
	"encoding/binary"
	"flag"
	"github.com/fsnotify/fsnotify"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"path"
	"regexp"
	"strings"
	"time"
)

type Config struct {
	Url     string
	Token   string
	Pattern string
	Watch   bool
	Archive bool
}

func WriteSimplifiedBinary(filePath string, binaryPath string) (parsed *KinemetricsFile, err error) {
	fp, err := os.Open(filePath)
	if err != nil {
		return nil, err
	}

	defer fp.Close()

	parsed, err = ParseKinemetricsFile(fp)
	if err != nil {
		log.Fatal("Error parsing Kinemetrics file", err)
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
	re := regexp.MustCompile("(\\d{6}\\d{2}\\d{2}\\d{4})")

	matches := re.FindAllStringSubmatch(path.Base(filePath), -1)
	if len(matches) == 0 {
		return nil
	}

	log.Printf("%v\n", matches)

	startTime, err := time.Parse("20060102150405", matches[0][2])
	if err != nil {
		return nil
	}

	binary = new(WaveformBinary)
	binary.StartTime = startTime
	binary.SamplesPerSecond = 500
	binary.InputId = 0

	return
}

func UploadBinary(file *WaveformBinary, filePath string, config *Config) (err error) {
	data, err := ioutil.ReadFile(filePath)
	if err != nil {
		return err
	}

	log.Printf("Uploading %s to %s", filePath, config.Url)

	req, err := http.NewRequest("POST", config.Url, bytes.NewBuffer(data))
	if err != nil {
		return err
	}

	req.Header.Set("content-type", "application/octet-stream")
	req.Header.Set("x-token", config.Token)
	req.Header.Set("x-timestamp", string(file.StartTime.Unix()))
	req.Header.Set("x-frequency", string(file.SamplesPerSecond))
	req.Header.Set("x-input-id", string(file.InputId))
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

func ArchiveBinary(file *WaveformBinary, filePath string, config *Config) (err error) {
	directory := path.Dir(filePath)
	yearMonth := file.StartTime.Format("200601")
	day := file.StartTime.Format("02")
	hour := file.StartTime.Format("15")
	newPath := path.Join(directory, "archive", yearMonth, day, hour)

	if os.MkdirAll(newPath, 0777) == nil {
		err := os.Rename(filePath, path.Join(newPath, path.Base(filePath)))
		if err == nil {
			log.Printf("Archived %s.\n", filePath)
		} else {
			log.Printf("Unable to archive %s: %s\n", filePath, err)
		}
	} else {
		log.Printf("Error creating directory %s, failed to archive %s\n", newPath, filePath)
	}

	return
}

func ScanDirectories(paths []string, config *Config) {
	re := regexp.MustCompile(config.Pattern)

	for _, filePath := range paths {
		fi, err := os.Stat(filePath)
		if err != nil {
			log.Fatal("Error", err)
		}
		switch mode := fi.Mode(); {
		case mode.IsDir():
			log.Printf("Scanning %s..", filePath)

			files, err := ioutil.ReadDir(filePath)
			if err != nil {
				log.Fatal(err)
			}

			for _, child := range files {
				if !child.IsDir() {
					childPath := path.Join(filePath, child.Name())
					matches := re.FindAllStringSubmatch(child.Name(), -1)
					if len(matches) > 0 {
						if strings.EqualFold(path.Ext(child.Name()), ".evt") {
							binaryPath := childPath + ".bin"

							if _, err := os.Stat(binaryPath); os.IsNotExist(err) {
								log.Printf("Processing %s...", childPath)

								parsed, err := WriteSimplifiedBinary(childPath, binaryPath)
								if err != nil {
									log.Printf("Error writing binary %s", err)
								}

								binary := NewKinemetricsBinary(parsed)
								if err := UploadBinary(binary, childPath, config); err != nil {
									log.Printf("Error uploading %s", err)
								}
							}
						} else {
							binary := NewGeophoneBinary(childPath)
							if err := UploadBinary(binary, childPath, config); err != nil {
								log.Printf("Error uploading %s", err)
							}

							if err := ArchiveBinary(binary, childPath, config); err != nil {
								log.Printf("Error archiving %s", err)
							}
						}
					}
				}
			}
		}
	}
}

func main() {
	var config Config

	flag.StringVar(&config.Url, "url", "", "url to upload data to")
	flag.StringVar(&config.Token, "token", "", "upload token")
	flag.StringVar(&config.Pattern, "pattern", "", "upload pattern")
	flag.BoolVar(&config.Watch, "watch", false, "watch directory for changes")
	flag.BoolVar(&config.Archive, "archive", false, "archive files after upload")

	flag.Parse()

	if flag.NArg() == 0 || config.Token == "" || config.Url == "" || config.Pattern == "" {
		flag.Usage()
		os.Exit(1)
	}

	ScanDirectories(flag.Args(), &config)

	if config.Watch {
		watcher, err := fsnotify.NewWatcher()
		if err != nil {
			log.Fatal(err)
		}
		defer watcher.Close()

		done := make(chan bool)
		go func() {
			for {
				select {
				case _ = <-watcher.Events:
					ScanDirectories(flag.Args(), &config)
				case err := <-watcher.Errors:
					log.Println("Error:", err)
				}
			}
		}()

		for _, filePath := range flag.Args() {
			fi, err := os.Stat(filePath)
			if err != nil {
				log.Fatal("Error", err)
			}
			switch mode := fi.Mode(); {
			case mode.IsDir():
				err = watcher.Add(filePath)
			}
		}
		if err != nil {
			log.Fatal(err)
		}

		<-done
	}
}
