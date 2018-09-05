package main

import (
	"io/ioutil"
	"log"
	"path/filepath"

	"github.com/fsnotify/fsnotify"
)

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
