package main

import (
	"flag"
	"fmt"
	fastping "github.com/tatsushid/go-fastping"
	"log"
	"log/syslog"
	"net"
	"os"
	"os/exec"
	"os/signal"
	"syscall"
	"time"
)

type response struct {
	addr *net.IPAddr
	rtt  time.Duration
}

func testNetworking(hostname string) (anySuccess bool, stopped bool) {
	p := fastping.NewPinger()
	ra, err := net.ResolveIPAddr("ip4:icmp", hostname)
	if err != nil {
		log.Fatal(err)
	}

	responses := make(map[string]*response)
	responses[ra.String()] = nil

	p.AddIPAddr(ra)

	onRecv, onIdle := make(chan *response), make(chan bool)
	p.OnRecv = func(addr *net.IPAddr, t time.Duration) {
		onRecv <- &response{addr: addr, rtt: t}
	}
	p.OnIdle = func() {
		onIdle <- true
	}

	p.MaxRTT = time.Second
	p.RunLoop()

	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt)
	signal.Notify(c, syscall.SIGTERM)

	startAt := time.Now()
	running := true
	stopped = false
	for running && time.Now().Sub(startAt).Seconds() < 10.0 {
		select {
		case <-c:
			running = false
			stopped = true
		case res := <-onRecv:
			if _, ok := responses[res.addr.String()]; ok {
				responses[res.addr.String()] = res
			}
		case <-onIdle:
			for host, r := range responses {
				if r == nil {
					log.Printf("%s : unreachable %v\n", host, time.Now())
				} else {
					log.Printf("%s : %v %v\n", host, r.rtt, time.Now())
					anySuccess = true
					running = false
				}
				responses[host] = nil
			}
		case <-p.Done():
			if err = p.Err(); err != nil {
				fmt.Println("Ping failed:", err)
				os.Exit(2)
			}
		}
	}

	signal.Stop(c)

	// This hangs for me for some reason?
	// p.Stop()

	return
}

func Execute(l []string, dryRun bool) error {
	log.Printf("Exec: %v", l)
	if !dryRun {
		c := exec.Command(l[0], l[1:]...)
		err := c.Run()
		if err != nil {
			log.Printf("Error: %v", err)
		}
	}
	return nil
}

type Options struct {
	DryRun        bool
	DisableReboot bool
	LogFile       string
	Syslog        string
}

func main() {
	var o Options

	flag.BoolVar(&o.DryRun, "dry", false, "dry run")
	flag.BoolVar(&o.DisableReboot, "disable-reboot", false, "disable reboot")
	flag.StringVar(&o.LogFile, "log", "resilience.log", "log file")
	flag.StringVar(&o.Syslog, "syslog", "", "enable syslog and name the ap")

	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage:\n  %s [options] hostname [source]\n\nOptions:\n", os.Args[0])
		flag.PrintDefaults()
	}
	flag.Parse()

	if o.Syslog == "" {
		f, err := os.OpenFile(o.LogFile, os.O_RDWR|os.O_CREATE|os.O_APPEND, 0666)
		if err != nil {
			log.Fatalf("Error opening file: %v", err)
		}
		defer f.Close()

		log.SetOutput(f)
	} else {
		syslog, err := syslog.New(syslog.LOG_NOTICE, o.Syslog)
		if err == nil {
			log.SetOutput(syslog)
		}
	}

	hostname := flag.Arg(0)
	if len(hostname) == 0 {
		flag.Usage()
		os.Exit(1)
	}

	if o.DryRun {
		log.Printf("Dry run enabled.")
	}

	good, stopped := testNetworking(hostname)
	if !stopped && !good {
		log.Printf("Unreachable, restarting networking...")

		Execute([]string{"/usr/sbin/service", "networking", "restart"}, o.DryRun)

		time.Sleep(2 * time.Second)

		Execute([]string{"/usr/sbin/service", "logmein-hamachi", "restart"}, o.DryRun)

		time.Sleep(2 * time.Second)

		good, stopped := testNetworking(hostname)
		if !stopped && !good {
			if !o.DisableReboot {
				log.Printf("Unreachable, restarting computer...")
				Execute([]string{"/sbin/reboot"}, o.DryRun)
			}
		}
	} else if good {
		log.Printf("Network is good.")
	}

}
