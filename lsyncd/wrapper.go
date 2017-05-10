package main

import (
	"log"
	"os"
	"os/exec"
	"syscall"
	"io/ioutil"
	"regexp"
	"path"
)

func archive(directory string) {
	re := regexp.MustCompile("(?i)(.+)_(\\d{6})(\\d{2}_\\d{2})(\\d{4}).bin")
	files, _ := ioutil.ReadDir(directory)
	for _, f := range files {
		matches := re.FindAllStringSubmatch(f.Name(), -1)
		if len(matches) > 0 {
			newPath := path.Join(directory, matches[0][2], matches[0][3])

			if os.MkdirAll(newPath, 0777) == nil {
				err := os.Rename(path.Join(directory, f.Name()), path.Join(newPath, f.Name()))
				if err == nil {
					log.Printf("Archived %s.\n", f.Name())
				} else {
					log.Printf("Unable to archive %s: %s\n", f.Name(), err)
				}
			} else {
				log.Printf("Error creating directory %s, failed to archive %s\n", newPath, f.Name())
			}
		}
	}
}

func main() {
	f, err := os.OpenFile("wrapper.log", os.O_RDWR | os.O_CREATE | os.O_APPEND, 0666)
	if err != nil {
		log.SetFlags(0)
		log.SetOutput(ioutil.Discard)
	} else {
		defer f.Close()
		log.SetOutput(f)
	}

	args := os.Args[1:]

	cmd := exec.Command("/usr/bin/rsync", args...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Run()

	waitStatus := cmd.ProcessState.Sys().(syscall.WaitStatus)

	if waitStatus.ExitStatus() == 0 {
		archive("/data")
	}

	log.Printf("Exiting %d", waitStatus.ExitStatus())

	os.Exit(waitStatus.ExitStatus())
}
