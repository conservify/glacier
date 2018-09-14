package main

import (
	"bytes"
	"flag"
	"log"
	"log/syslog"
	"os"
	"os/exec"
	"time"

	"github.com/Conservify/goridium"
)

type options struct {
	Device string
	Syslog string
	DryRun bool
}

func main() {
	o := options{}
	flag.BoolVar(&o.DryRun, "dry", false, "dry run")
	flag.StringVar(&o.Syslog, "syslog", "", "enable syslog and name the ap")
	flag.StringVar(&o.Device, "device", "", "device to use")
	flag.Parse()

	if len(o.Device) == 0 {
		flag.Usage()
		os.Exit(2)
	}

	if o.Syslog != "" {
		syslog, err := syslog.New(syslog.LOG_NOTICE, o.Syslog)
		if err == nil {
			log.SetOutput(syslog)
		}
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
	}

	err = rb.AttemptConnection()
	if err != nil {
		log.Fatalf("Unable to establish connection: %v", err)
	}

	incoming, err := rb.AttemptSession()
	if err != nil {
		log.Fatalf("Unable to establish session: %v", err)
	}

	for _, m := range incoming {
		log.Printf("Received: %s", m)
		if m == "REBOOT" {
			message := "REBOOTING," + getUptime()
			err = rb.QueueMessage(message)
			if err == nil {
				_, err := rb.AttemptSession()
				if err != nil {
					log.Printf("Unable to establish session: %v", err)
				}
			} else {
				log.Printf("Unable to queue message: %v", err)
			}

			execute([]string{"/sbin/reboot"}, o.DryRun, 10)
		} else {
			message := "PONG," + getUptime()
			err = rb.QueueMessage(message)
			if err != nil {
				log.Fatalf("Unable to queue message: %v", err)
			}
			_, err := rb.AttemptSession()
			if err != nil {
				log.Fatalf("Unable to establish session: %v", err)
			}
		}
	}
}

func getUptime() string {
	if b, e := exec.Command("/usr/bin/uptime").Output(); e == nil {
		return string(b)
	}
	return "<unknown>"
}

func execute(l []string, dryRun bool, sleep int) error {
	log.Printf("Exec: %v", l)
	if !dryRun {
		so := bytes.Buffer{}
		se := bytes.Buffer{}
		time.Sleep(time.Duration(sleep) * time.Second)
		c := exec.Command(l[0], l[1:]...)
		c.Stdout = &so
		c.Stderr = &se
		err := c.Run()
		if err != nil {
			log.Printf("Error: %v", err)
		}

		if so.Len() > 0 {
			log.Printf("%s", so.String())
		}
		if se.Len() > 0 {
			log.Printf("%s", se.String())
		}
	}
	return nil
}
