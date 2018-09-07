package main

import (
	"bufio"
	"encoding/csv"
	"flag"
	"fmt"
	"io"
	"log"
	"os"
	"strings"
	"time"
)

func main() {
	timeColumn := 1
	daysToKeep := 14

	flag.IntVar(&timeColumn, "time-column", 1, "index of the time column")
	flag.IntVar(&daysToKeep, "days-to-keep", 7*4, "days to keep")

	flag.Parse()

	removeBefore := time.Now().Add(time.Duration(-daysToKeep*24) * time.Hour)

	for _, filename := range flag.Args() {
		file, err := os.Open(filename)
		if err != nil {
			log.Fatal(err)
		}

		defer file.Close()

		r := csv.NewReader(bufio.NewReader(file))
		for {
			record, err := r.Read()
			if err == io.EOF {
				break
			}

			t, err := time.Parse("2006-01-02 15:04:05", strings.Replace(record[timeColumn], " +0000 UTC", "", 1))
			if err != nil {
				log.Fatal(err)
			}

			if t.After(removeBefore) {
				fmt.Printf("%d", t.Unix())

				for _, field := range record {
					fmt.Printf(",")
					fmt.Printf("%s", field)
				}

				fmt.Println()
			}
		}
	}
}
