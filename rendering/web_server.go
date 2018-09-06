package main

import (
	"encoding/json"
	"flag"
	"log"
	"net/http"
	"strconv"
	"time"
)

type WebServer struct {
	o  *options
	dw *DataWatcher
}

func NewWebServer(o *options, dw *DataWatcher) (ws *WebServer, err error) {
	ws = &WebServer{
		o:  o,
		dw: dw,
	}

	return
}

type StatusResponse struct {
	AvailableHours []int64
	CurrentHour    HourStatus
	PreviousHour   HourStatus
}

type HourStatus struct {
	Hour          int64
	Start         *time.Time
	End           *time.Time
	NumberOfFiles int
}

func (ws *WebServer) ServeStatus() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		log.Printf("/status.json")

		w.Header().Set("Content-Type", "application/json")
		w.Header().Set("Cache-Control", "no-cache, no-store, must-revalidate")
		w.Header().Set("Pragma", "no-cache")
		w.Header().Set("Expires", "0")

		afs := NewArchiveFileSet()
		for _, arg := range flag.Args() {
			if err := afs.AddFrom(arg); err != nil {
				panic(err)
			}
		}

		currentHour := afs.FilterCurrentHour()
		previousHour := afs.FilterPreviousHour()

		status := StatusResponse{
			AvailableHours: afs.Hours,
		}

		if len(currentHour.Files) > 0 && len(previousHour.Files) > 0 {
			status = StatusResponse{
				AvailableHours: afs.Hours,
				CurrentHour: HourStatus{
					Hour:          currentHour.Files[0].Hour.Unix(),
					Start:         currentHour.Start,
					End:           currentHour.End,
					NumberOfFiles: len(currentHour.Files),
				},
				PreviousHour: HourStatus{
					Hour:          previousHour.Files[0].Hour.Unix(),
					Start:         previousHour.Start,
					End:           previousHour.End,
					NumberOfFiles: len(previousHour.Files),
				},
			}
		}

		log.Printf("/status.json, done")

		b, _ := json.Marshal(status)

		w.Write(b)
	}
}

func (ws *WebServer) ServeRendering() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		log.Printf("/rendering.png")

		afs := NewArchiveFileSet()
		for _, arg := range flag.Args() {
			if err := afs.AddFrom(arg); err != nil {
				panic(err)
			}
		}

		var filtered *ArchiveFileSet

		axis := r.URL.Query().Get("axis")
		if axis == "" {
			axis = "x"
		}
		hourParam := r.URL.Query().Get("hour")
		if hourParam != "" {
			hour, err := strconv.Atoi(hourParam)
			if err != nil {
				return
			}

			t := time.Unix(int64(hour), 0)

			log.Printf("Rendering (hour = %v) (axis = %v)", hour, axis)
			filtered = afs.FilterByHour(t)
		} else {

			log.Printf("Rendering (CURRENT) (axis = %v)", axis)
			filtered = afs.FilterCurrentHour()
		}

		w.Header().Set("Content-Type", "image/png")
		w.Header().Set("Cache-Control", "no-cache, no-store, must-revalidate")
		w.Header().Set("Pragma", "no-cache")
		w.Header().Set("Expires", "0")

		hr := NewHourlyRendering(axis, 1, 60*60*2, 250, false, false)
		hr.DrawAll(filtered, false)
		hr.EncodeTo(w)
	}
}

func (ws *WebServer) Register() {
	http.Handle("/", http.FileServer(http.Dir(ws.o.Web)))
	http.HandleFunc("/status.json", ws.ServeStatus())
	http.HandleFunc("/rendering.png", ws.ServeRendering())
}
