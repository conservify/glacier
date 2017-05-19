package main

import (
	"bytes"
	"encoding/binary"
	"flag"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"path"
	"strings"

	"github.com/fsnotify/fsnotify"
)

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

func UploadBinary(file *KinemetricsFile, filePath string, url string, token string) (err error) {
	data, err := ioutil.ReadFile(filePath)
	if err != nil {
		return err
	}

	log.Printf("Uploading %s to %s", filePath, url)

	req, err := http.NewRequest("POST", url, bytes.NewBuffer(data))
	if err != nil {
		return err
	}

	req.Header.Set("content-type", "application/octet-stream")
	req.Header.Set("x-token", token)
	req.Header.Set("x-timestamp", string(file.StartTime.Unix()))
	req.Header.Set("x-frequency", string(file.SamplesPerSecond))
	req.Header.Set("x-input-id", "0")
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

func ScanDirectories(paths []string, url string, token string) {
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
				if !child.IsDir() && strings.EqualFold(path.Ext(child.Name()), ".evt") {
					childPath := path.Join(filePath, child.Name())
					binaryPath := childPath + ".bin"

					if _, err := os.Stat(binaryPath); os.IsNotExist(err) {
						log.Printf("Processing %s...", childPath)

						parsed, err := WriteSimplifiedBinary(childPath, binaryPath)
						if err != nil {
							log.Printf("Error writing binary %s", err)
						}

						if err := UploadBinary(parsed, childPath, url, token); err != nil {
							log.Printf("Error uploading %s", err)
						}
					}
				}
			}
		}
	}
}

func main() {
	var url string
	var token string
	var watch bool

	flag.StringVar(&url, "url", "", "url to upload data to")
	flag.StringVar(&token, "token", "", "upload token")
	flag.BoolVar(&watch, "watch", false, "watch directory for changes")

	flag.Parse()

	if flag.NArg() == 0 || token == "" || url == "" {
		flag.Usage()
		os.Exit(1)
	}

	ScanDirectories(flag.Args(), url, token)

	if watch {
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
					ScanDirectories(flag.Args(), url, token)
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
