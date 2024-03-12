package main

import (
	"fmt"
	"io/ioutil"
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
	Minute   uint32
	Second   uint32
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
		Minute:   uint32(t.Minute()),
		Second:   uint32(t.Second()),
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

func (afs *ArchiveFileSet) Sort() error {
	return nil
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

func isDir(p string) (bool, error) {
	i, err := os.Stat(p)
	if err != nil {
		return false, err
	}
	return i.Mode().IsDir(), nil
}

func FindArchiveFiles(dir string) (*ArchiveFileSet, error) {
	afs := NewArchiveFileSet()
	err := filepath.Walk(dir, func(p string, f os.FileInfo, err error) error {
		if f != nil && !f.IsDir() {
			if filepath.Ext(p) == ".bin" {
				if af, err := NewArchiveFile(p); err == nil {
					return afs.Add(af)
				}
			}
		}
		return nil
	})
	if err != nil {
		return nil, err
	}

	return afs, nil
}

type CheckDirectory func(dir string) (bool, error)

func FindDirectoryOnOrBefore(t time.Time, source string, check CheckDirectory) (string, error) {
	dir, err := filepath.Abs(source)
	if err != nil {
		return "", err
	}

	for i := 0; i < 365; i += 1 {
		day := time.Date(t.Year(), t.Month(), t.Day(), 0, 0, 0, 0, t.Location()).UTC()
		relative := path.Join(dir, day.Format("200601"), day.Format("02"))
		if b, _ := isDir(relative); !b {
			log.Printf("Not found: '%s'", relative)
		} else {
			files, err := ioutil.ReadDir(relative)
			if err != nil {
				return "", err
			}
			if len(files) > 2 {
				good, err := check(relative)
				if err != nil {
					return "", err
				}
				if good {
					dir = relative
					break
				}
			} else {
				log.Printf("Empty: '%s' (%v)", relative, len(files))
			}
		}

		// TOOD: This could be faster if we checked to see if months are missing.
		t = t.Add(time.Hour * -24)
	}

	return dir, nil
}

func (afs *ArchiveFileSet) Merge(other *ArchiveFileSet) error {
	for _, af := range other.Files {
		afs.Add(af)
	}
	return nil
}

func (afs *ArchiveFileSet) AddFrom(source string, recurse bool) error {
	dir, err := filepath.Abs(source)
	if err != nil {
		return err
	}

	if !recurse {
		dir, err = FindDirectoryOnOrBefore(time.Now(), source, func(adding string) (bool, error) {
			found, err := FindArchiveFiles(adding)
			if err != nil {
				return false, err
			}
			if len(found.Files) == 0 {
				return false, err
			}
			log.Printf("Adding from '%s' (%d files)", dir, len(found.Files))
			afs.Merge(found)
			return true, nil
		})
		if err != nil {
			return err
		}
	} else {
		found, err := FindArchiveFiles(dir)
		if err != nil {
			return err
		}

		log.Printf("Added from '%s' (%d files)", dir, len(found.Files))
		afs.Merge(found)
	}

	log.Printf("Found %d files", len(afs.Files))

	return nil
}

func (afs *ArchiveFileSet) FilterPreviousHour() (newAfs *ArchiveFileSet) {
	if len(afs.Hours) < 2 {
		return NewArchiveFileSet()
	}

	hour := afs.Hours[len(afs.Hours)-2]

	return afs.FilterByHour(time.Unix(hour, 0))
}

func (afs *ArchiveFileSet) FilterCurrentHour() (newAfs *ArchiveFileSet) {
	if len(afs.Hours) < 1 {
		return NewArchiveFileSet()
	}

	hour := afs.Hours[len(afs.Hours)-1]

	return afs.FilterByHour(time.Unix(hour, 0))
}

func (afs *ArchiveFileSet) FilterByHour(h time.Time) (newAfs *ArchiveFileSet) {
	newAfs = NewArchiveFileSet()

	all := afs.Hourly[h.Unix()]

	for _, af := range all {
		newAfs.Add(af)
	}

	return
}
