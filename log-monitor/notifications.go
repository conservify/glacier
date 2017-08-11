package main

import (
	"fmt"
	"github.com/sfreiberg/gotwilio"
	"log"
	"time"
)

type StatusUpdate struct {
	Name   string
	Status StatusType
}

type NotificationStatus struct {
	PreviousStatus StatusType
	LastStatus     time.Time
}

func (notifs *NotificationStatus) sendSingleStatus(ni *NetworkInfo) {
	notify := false
	interval := 24 * time.Hour
	newStatus := Good
	ns, _ := ToNetworkStatus(ni)

	for _, m := range ns.Machines {
		all := []StatusUpdate{
			StatusUpdate{Status: m.Health.Status, Name: "Health"},
			StatusUpdate{Status: m.Mounts.Status, Name: "Mounts"},
			StatusUpdate{Status: m.LocalBackup.Status, Name: "LocalBackup"},
		}

		if m.OffsiteBackup != nil {
			all = append(all, StatusUpdate{Status: m.OffsiteBackup.Status, Name: "OffsiteBackup"})
		}
		if m.Geophone != nil {
			all = append(all, StatusUpdate{Status: m.Geophone.Status, Name: "Geophone"})
		}
		if m.Uploader != nil {
			all = append(all, StatusUpdate{Status: m.Uploader.Status, Name: "Uploader"})
		}

		machineStatus := Good
		for _, su := range all {
			machineStatus = machineStatus.Prioritize(su.Status)
		}
		newStatus = newStatus.Prioritize(machineStatus)

		log.Printf("%s: %v", m.Hostname, machineStatus)
	}

	log.Printf("Global: %v (was %v)", newStatus, notifs.PreviousStatus)

	if notifs.PreviousStatus != newStatus {
		log.Printf("State change %v -> %v", notifs.PreviousStatus, newStatus)
		notifs.PreviousStatus = newStatus
		notify = true
	}
	if newStatus != Good {
		interval = 6 * time.Hour
	}
	if notifs.LastStatus.Add(interval).Before(time.Now()) {
		log.Printf("Notifying (reminder)")
		notify = true
	}

	if notify {
		log.Printf("Notifying")
		notifs.LastStatus = time.Now()
		twilio := gotwilio.NewTwilioClient(twilioSid, twilioToken)

		from := "+12132617278"
		to := "+19515438308"
		message := fmt.Sprintf("Status: %s", newStatus)
		twilio.SendSMS(from, to, message, "", "")
	}
}

func SendStatus(ni *NetworkInfo) {
	time.Sleep(2 * time.Second)

	notifs := &NotificationStatus{
		PreviousStatus: Unknown,
	}
	for {
		notifs.sendSingleStatus(ni)

		time.Sleep(5 * time.Second)
	}
}
