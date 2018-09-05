package main

import (
	"encoding/json"
	"net/http"
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
	Start         *time.Time
	End           *time.Time
	NumberOfFiles int
}

func (ws *WebServer) ServeStatus() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		w.Header().Set("Cache-Control", "no-cache, no-store, must-revalidate")
		w.Header().Set("Pragma", "no-cache")
		w.Header().Set("Expires", "0")

		afs := NewArchiveFileSet()
		afs.AddFrom(ws.o.Watch)

		filtered := afs.FilterLatestHour()

		status := StatusResponse{
			Start:         filtered.Start,
			End:           filtered.End,
			NumberOfFiles: len(filtered.Files),
		}
		b, _ := json.Marshal(status)
		w.Write(b)
	}
}

func (ws *WebServer) ServeRendering() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "image/png")
		w.Header().Set("Cache-Control", "no-cache, no-store, must-revalidate")
		w.Header().Set("Pragma", "no-cache")
		w.Header().Set("Expires", "0")

		afs := NewArchiveFileSet()
		afs.AddFrom(ws.o.Watch)

		filtered := afs.FilterLatestHour()

		hr := NewHourlyRendering("x", 1, 60*60*2, 250, false, false)
		hr.DrawAll(filtered, false)
		hr.EncodeTo(w)
	}
}

func (ws *WebServer) Register() {
	http.Handle("/", http.FileServer(http.Dir(ws.o.Web)))
	http.HandleFunc("/status.json", ws.ServeStatus())
	http.HandleFunc("/rendering.png", ws.ServeRendering())
}
