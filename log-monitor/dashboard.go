package main

import (
	"encoding/json"
	"net/http"
)

func statusHandler(ni *NetworkInfo) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		ns, _ := ToNetworkStatus(ni)
		b, _ := json.Marshal(*ns)
		w.Write(b)
	}
}

func StartWebServer(ni *NetworkInfo) {
	http.HandleFunc("/status.json", statusHandler(ni))
	http.Handle("/", http.FileServer(http.Dir("./static")))
	http.ListenAndServe(":8000", nil)
}
