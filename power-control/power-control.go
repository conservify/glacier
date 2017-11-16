package main

import (
	"bufio"
	"flag"
	"fmt"
	"github.com/beevik/ntp"
	"log"
	"log/syslog"
	"os"
	"strings"
	"time"
)

type Options struct {
	LogFile       string
	Syslog        string
	NtpServer     string
	TimeZone      string
	HoldingPeriod int
	Pin           int
	TurnOn        bool
	TurnOff       bool
}

const (
	OffHour = 22
	OnHour  = 5
)

func holdingPeriod(o *Options) {
	log.Printf("Entering %d minute holding period...", o.HoldingPeriod)

	period := time.Duration(o.HoldingPeriod) * time.Minute
	start := time.Now()
	for {
		uptime := time.Now().Sub(start)
		if o.HoldingPeriod == 0 || uptime > period {
			break
		}
		time.Sleep(1 * time.Minute)
	}

	log.Printf("Holding period done.")
}

func echoFile(file string, text string) (err error) {
	f, err := os.OpenFile(file, os.O_WRONLY, 0666)
	if err != nil {
		log.Printf("Error: echo %s > %s: %s", strings.Replace(strings.Replace(text, "\n", "", -1), "\r", "", -1), file, err)
		return err
	}

	defer f.Close()

	w := bufio.NewWriter(f)

	defer w.Flush()

	_, err = w.WriteString(text)
	if err != nil {
		log.Printf("Error: echo %s > %s: %s", strings.Replace(strings.Replace(text, "\n", "", -1), "\r", "", -1), file, err)
		return err
	}

	return
}

func digitalWrite(pin int, high bool) error {
	echoFile("/sys/class/gpio/export", fmt.Sprintf("%d\n", pin))
	echoFile(fmt.Sprintf("/sys/class/gpio/gpio%d/direction", pin), "out\n")
	if high {
		echoFile(fmt.Sprintf("/sys/class/gpio/gpio%d/value", pin), "1\n")
	} else {
		echoFile(fmt.Sprintf("/sys/class/gpio/gpio%d/value", pin), "0\n")
		echoFile("/sys/class/gpio/unexport", fmt.Sprintf("%d\n", pin))
	}
	return nil
}

func turnEverythingOff(o *Options) (on bool) {
	log.Printf("TurnEverythingOff(%d)", o.Pin)
	digitalWrite(o.Pin, true)
	return false
}

func turnEverythingOn(o *Options) (on bool) {
	log.Printf("TurnEverythingOn(%d)", o.Pin)
	digitalWrite(o.Pin, false)
	return true
}

func shouldBeOn(now time.Time, nextOn time.Time) bool {
	if now.After(nextOn) {
		return true
	}
	return false
}

func shouldBeOff(now time.Time, nextOff time.Time) bool {
	if now.After(nextOff) {
		return true
	}
	return false
}

func nextOffTime(loc *time.Location) time.Time {
	n := time.Now().In(loc)
	t := time.Date(n.Year(), n.Month(), n.Day(), OffHour, 0, 0, 0, loc)
	if t.Before(n) {
		return t.AddDate(0, 0, 1)
	}
	return t
}

func nextOnTime(loc *time.Location) time.Time {
	n := time.Now().In(loc)
	t := time.Date(n.Year(), n.Month(), n.Day(), OnHour, 0, 0, 0, loc)
	if t.Before(n) {
		return t.AddDate(0, 0, 1)
	}
	return t
}

func isSystemTimeGood(o *Options) bool {
	actualTime, err := ntp.Time(o.NtpServer)
	if err != nil {
		log.Printf("Unable to get time: %v", err)
		return false
	}

	systemTime := time.Now()
	diff := systemTime.Sub(actualTime)
	log.Printf("Checking system time: %v vs %v = %v", actualTime, systemTime, diff)

	// Eh, 1 minute is close enough.
	return diff.Minutes() < 1
}

func main() {
	var o Options

	flag.IntVar(&o.HoldingPeriod, "holding-period", 30, "holding period in minutes")
	flag.IntVar(&o.Pin, "pin", 17, "pin to control")
	flag.BoolVar(&o.TurnOn, "on", false, "turn on (testing)")
	flag.BoolVar(&o.TurnOff, "off", false, "turn off (testing)")
	flag.StringVar(&o.LogFile, "log", "", "log file")
	flag.StringVar(&o.Syslog, "syslog", "", "enable syslog and name the ap")
	flag.StringVar(&o.NtpServer, "ntp-server", "0.beevik-ntp.pool.ntp.org", "ntp server")
	flag.StringVar(&o.TimeZone, "time-zone", "America/Edmonton", "time zone")

	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage:\n  %s [options]\n\nOptions:\n", os.Args[0])
		flag.PrintDefaults()
	}
	flag.Parse()

	if o.Syslog == "" {
		if o.LogFile != "" {
			f, err := os.OpenFile(o.LogFile, os.O_RDWR|os.O_CREATE|os.O_APPEND, 0666)
			if err != nil {
				log.Fatalf("Error opening file: %v", err)
			}
			defer f.Close()

			log.SetOutput(f)
		}
	} else {
		syslog, err := syslog.New(syslog.LOG_NOTICE, o.Syslog)
		if err == nil {
			log.SetOutput(syslog)
		}
	}

	if o.TurnOn {
		turnEverythingOn(&o)
		return
	}

	if o.TurnOff {
		turnEverythingOff(&o)
		return
	}

	everythingOn := turnEverythingOn(&o)

	// With everything on we should be connected. In fact,
	// refuse to turn things off unless we're doing so aware of
	// the actual time.
	for {
		if isSystemTimeGood(&o) {
			log.Printf("System time is good...")
			break
		}

		time.Sleep(5 * time.Minute)
	}

	// lastTurnedOff := time.Now()
	location, err := time.LoadLocation(o.TimeZone)
	if err != nil {
		log.Printf("Unable to get time: %v", err)
	}

	// Outer loop ensures we always perform a holding period after something on or off.
	for {
		nextOff := nextOffTime(location)
		nextOn := nextOnTime(location)

		log.Printf("UTC:")
		log.Printf("Now: %v", time.Now().UTC())
		log.Printf("Off: %v (in %v)", nextOff.UTC(), nextOff.Sub(time.Now()))
		log.Printf(" On: %v (in %v)", nextOn.UTC(), nextOn.Sub(time.Now()))

		log.Printf("%s:", location)
		log.Printf("Now: %v", time.Now().In(location))
		log.Printf("Off: %v (in %v)", nextOff, nextOff.Sub(time.Now()))
		log.Printf(" On: %v (in %v)", nextOn, nextOn.Sub(time.Now()))

		holdingPeriod(&o)

		for {
			localTime := time.Now().In(location)

			if everythingOn {
				if shouldBeOff(localTime, nextOff) {
					n := nextOnTime(location)
					log.Printf("Turning off, will turn on at %v (in %v)", n, n.Sub(time.Now()))
					time.Sleep(10 * time.Second)
					everythingOn = turnEverythingOff(&o)
					break
				}
			} else {
				if shouldBeOn(localTime, nextOn) {
					n := nextOffTime(location)
					log.Printf("Turning on, will turn off at %v (in %v)", n, n.Sub(time.Now()))
					everythingOn = turnEverythingOn(&o)
					break
				}
			}

			time.Sleep(1 * time.Minute)
		}
	}
}
