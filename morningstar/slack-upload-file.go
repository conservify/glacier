package main

import (
	"flag"
	"github.com/nlopes/slack"
	"log"
	"os"
	"path/filepath"
)

type options struct {
	Token   string
	Channel string
	File    string
}

func main() {
	o := options{}

	flag.StringVar(&o.Token, "token", "", "token")
	flag.StringVar(&o.Channel, "channel", "", "channel")
	flag.StringVar(&o.File, "file", "", "file")

	flag.Parse()

	if o.Token == "" || o.Channel == "" || o.File == "" {
		flag.Usage()
		os.Exit(1)
	}

	api := slack.New(o.Token)

	name := filepath.Base(o.File)
	uploadParams := slack.FileUploadParameters{
		Title: name,
		File:  o.File,
		Channels: []string{
			o.Channel,
		},
	}
	_, err := api.UploadFile(uploadParams)
	if err != nil {
		log.Fatal("%v", err)
		return
	}
}
