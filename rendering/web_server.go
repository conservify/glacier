package main

import (
	"encoding/json"
	"net/http"
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

func (ws *WebServer) ServeData() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		state := make(map[string]string)

		b, _ := json.Marshal(state)
		w.Header().Set("Content-Type", "application/json")
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
	http.HandleFunc("/data.json", ws.ServeData())
	http.HandleFunc("/rendering.png", ws.ServeRendering())
}
