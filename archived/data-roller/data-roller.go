package main

import (
	"flag"
	"log"
	"log/syslog"
	"os"
	"path/filepath"
	"sort"
	_ "strings"
	"syscall"
	"time"
)

type Config struct {
	LogFile string
	Syslog  string
	From    string
	Require int
	FreeUp  int
	DryRun  bool
}

type DataFile struct {
	Path  string
	Ctime int64
	Size  int64
}

type ByCtime []DataFile

func (s ByCtime) Len() int {
	return len(s)
}

func (s ByCtime) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s ByCtime) Less(i, j int) bool {
	return s[i].Ctime > s[j].Ctime
}

type FileSet struct {
	Files []DataFile
}

func freeSpaceOn(d string) uint64 {
	var stat syscall.Statfs_t
	syscall.Statfs(d, &stat)
	return stat.Bavail * uint64(stat.Bsize)
}

func processFile(fs *FileSet, config *Config) filepath.WalkFunc {
	return func(path string, info os.FileInfo, err error) error {
		if err != nil {
			log.Print(err)
			return nil
		}
		fi, err := os.Stat(path)
		if err != nil {
			log.Print(err)
			return nil
		}
		stat := fi.Sys().(*syscall.Stat_t)
		ctime := time.Unix(int64(stat.Ctim.Sec), int64(stat.Ctim.Nsec))
		fs.Files = append(fs.Files, DataFile{
			Path:  path,
			Size:  stat.Size,
			Ctime: ctime.Unix(),
		})
		return nil
	}
}

func main() {
	var config Config

	flag.StringVar(&config.LogFile, "log", "", "log file")
	flag.StringVar(&config.Syslog, "syslog", "", "enable syslog and name the ap")
	flag.StringVar(&config.From, "from", "", "from directory")
	flag.IntVar(&config.Require, "require", 0, "free MB to require")
	flag.IntVar(&config.FreeUp, "free-up", 0, "free up so many MB")
	flag.BoolVar(&config.DryRun, "dry", false, "dry run mode")

	flag.Parse()

	if config.From == "" {
		flag.Usage()
		os.Exit(1)
	}

	if config.Syslog == "" {
		if config.LogFile != "" {
			f, err := os.OpenFile(config.LogFile, os.O_RDWR|os.O_CREATE|os.O_APPEND, 0666)
			if err != nil {
				log.Fatalf("Error opening file: %v", err)
			}
			defer f.Close()

			log.SetOutput(f)
		}
	} else {
		syslog, err := syslog.New(syslog.LOG_NOTICE, config.Syslog)
		if err == nil {
			log.SetOutput(syslog)
		}
	}

	freeUp := uint64(config.FreeUp) * uint64(1024*1024)
	requiredBytes := uint64(config.Require) * uint64(1024*1024)
	available := freeSpaceOn(config.From)
	if freeUp > 0 {
		log.Printf("Freeing up %d bytes", freeUp)
		requiredBytes = available + freeUp
	}

	fs := FileSet{}
	err := filepath.Walk(config.From, processFile(&fs, &config))
	if err != nil {
		log.Fatalf("Error %s", err)
	}

	sort.Sort(ByCtime(fs.Files))

	for _, file := range fs.Files {
		log.Printf("Processing (avail %d) (req %d) (file %v) (ctime %d)\n", available, requiredBytes, file.Path, file.Ctime)

		if requiredBytes > 0 && available < requiredBytes {
			log.Printf("Deleting %s to make room...", file)
			if !config.DryRun {
				os.Remove(file.Path)
			}
			available += uint64(file.Size)
		} else {
			log.Printf("Done making room")
			break
		}
	}
}
