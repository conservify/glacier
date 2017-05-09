package main

import (
	"log"
	"os"
	"os/exec"
	"syscall"
	"io/ioutil"
	"regexp"
)

func archive(directory string) {
	re := regexp.MustCompile("(?i)(.+)_(\\d{6})(\\d{2}_\\d{2})(\\d{4}).bin")
	files, _ := ioutil.ReadDir(directory)
	for _, f := range files {
		matches := re.FindAllStringSubmatch(f.Name(), -1)
		if len(matches) > 0 {
			newPath := matches[0][2] + "/" + matches[0][3]

			if os.MkdirAll(newPath, 0777) == nil {
				if os.Rename(directory + "/" + f.Name(), newPath + "/" + f.Name()) == nil {
					log.Printf("Archived %s.\n", f.Name())
				} else {
					log.Fatalf("Unable to archive %s.\n", f.Name())
				}
			} else {
				log.Fatalf("Error creating directory %s, failed to archive %s\n", newPath, f.Name())
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
		archive("./")
	}

	os.Exit(waitStatus.ExitStatus())
}
