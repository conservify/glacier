package main

import (
	"flag"
	"log"
	"net/http"
)

func watchAndServe(o *options) error {
	if false {
		dw, err := NewDataWatcher(o.Watch)
		if err != nil {
			return err
		}

		defer dw.Close()

		dw.Watch()
	}

	log.Printf("Listening on :9090")

	ws, err := NewWebServer(o, nil)
	if err != nil {
		return err
	}

	ws.Register()

	err = http.ListenAndServe(":9090", nil)
	if err != nil {
		return err
	}

	return nil
}

func renderFiles(o *options) error {
	afs := NewArchiveFileSet()

	for _, arg := range flag.Args() {
		if err := afs.AddFrom(arg); err != nil {
			panic(err)
		}
	}

	if len(afs.Files) == 0 {
		return nil
	}

	log.Printf("Number of files: %v", len(afs.Files))
	log.Printf("Number of hours: %v", len(afs.Hours))
	log.Printf("Range: %v - %v", afs.Start, afs.End)

	hr := NewHourlyRendering(o.Axis, 12, o.Cx, o.Cy, o.StrictScaling, true)
	hr.DrawAll(afs, o.Overwrite)

	hr.Save()

	return nil
}

type options struct {
	Overwrite     bool
	StrictScaling bool
	Axis          string
	Watch         string
	Web           string
	Cx            int
	Cy            int
}

func main() {
	o := options{}

	flag.StringVar(&o.Axis, "axis", "x", "axis")
	flag.IntVar(&o.Cx, "cx", 60*60*2, "width")
	flag.IntVar(&o.Cy, "cy", 2000, "height")
	flag.BoolVar(&o.StrictScaling, "strict-scaling", false, "strict scaling")
	flag.BoolVar(&o.Overwrite, "overwrite", false, "overwite existing frames")
	flag.StringVar(&o.Web, "web", "./", "web")
	flag.StringVar(&o.Watch, "watch", "", "watch")

	flag.Parse()

	if o.Watch != "" {
		err := watchAndServe(&o)
		if err != nil {
			panic(err)
		}
	} else {
		err := renderFiles(&o)
		if err != nil {
			panic(err)
		}
	}
}
