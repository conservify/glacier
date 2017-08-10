package main

import (
	"net/http"
	"encoding/json"
)

func statusHandler(ns *NetworkStatus) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		ns.Lock.Lock()
		b, _ := json.Marshal(*ns)
		w.Write(b)
		ns.Lock.Unlock()
	}
}

func StartWebServer(ns *NetworkStatus) {
	http.HandleFunc("/status.json", statusHandler(ns))
	http.Handle("/", http.FileServer(http.Dir("./static")))
	http.ListenAndServe(":8000", nil)
}
