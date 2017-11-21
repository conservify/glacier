package main

import (
	"flag"
	"github.com/conservify/goridium"
	"log"
	"os"
)

type options struct {
	Device string
}

func main() {
	o := options{}

	flag.StringVar(&o.Device, "device", "", "device to use")
	flag.Parse()

	if len(o.Device) == 0 {
		flag.Usage()
		os.Exit(2)
	}

	rb, err := goridium.NewRockBlock(o.Device)
	if err != nil {
		log.Fatalf("Unable to open RockBlock: %v", err)
	}

	defer rb.Close()

	err = rb.Ping()
	if err != nil {
		log.Fatalf("Unable to ping RockBlock: %v", err)
	}

	rb.EnableEcho()

	rb.DisableRingAlerts()

	rb.DisableFlowControl()

	_, err = rb.GetSignalStrength()
	if err != nil {
		log.Fatalf("Unable to get signal strength: %v", err)
	}

	_, err = rb.GetNetworkTime()
	if err != nil {
		log.Printf("Unable to get network time: %v", err)
	}

	_, err = rb.GetSerialIdentifier()
	if err != nil {
		log.Fatalf("Unable to get serial id: %v", err)
	}

	outgoing := ""
	if outgoing != "" {
		err = rb.QueueMessage("Hello, World")
		if err != nil {
			log.Fatalf("Unable to queue message: %v", err)
		}

		err = rb.AttemptConnection()
		if err != nil {
			log.Fatalf("Unable to establish connection: %v", err)
		}
	}

	incoming, err := rb.AttemptSession()
	if err != nil {
		log.Fatalf("Unable to establish session: %v", err)
	}

	for _, m := range incoming {
		log.Printf("Received: %s", m)

		// Ping?

		// Turn everything on?

		// Restart?
	}
}
