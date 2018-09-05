package main

import (
	"fmt"
	"log"
	"os"
	"path"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
	"time"
)

type ArchiveFile struct {
	Time     *time.Time
	Hour     *time.Time
	FileName string
}

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
	log.Printf("Adding from '%s'...", path)

	err := filepath.Walk(path, func(p string, f os.FileInfo, err error) error {
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

	log.Printf("Found %d files", len(afs.Files))

	return err
}

func (afs *ArchiveFileSet) FilterLatestHour() (newAfs *ArchiveFileSet) {
	newAfs = NewArchiveFileSet()

	sort.Slice(afs.Files, func(i, j int) bool {
		return afs.Files[i].Time.After(*afs.Files[j].Time)
	})

	hour := -1

	for _, af := range afs.Files {
		if hour == -1 {
			hour = af.Time.Hour()
			log.Printf("Filtering: %v", af.Time)
		}

		if hour != af.Time.Hour() {
			break
		}

		newAfs.Add(af)
	}

	return
}
