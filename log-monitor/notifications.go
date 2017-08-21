package main

import (
	"fmt"
	"github.com/sfreiberg/gotwilio"
	"log"
	"time"
)

type StatusUpdate struct {
	Machine string
	Name    string
	Status  StatusType
}

type NotificationStatus struct {
	Attempt        int
	PreviousStatus StatusType
	StartTime      time.Time
	LastStatus     time.Time
}

func (notifs *NotificationStatus) sendSingleStatus(ni *NetworkInfo) {
	notify := false
	interval := 24 * time.Hour
	newStatus := Good
	ns, _ := ToNetworkStatus(ni)
	problems := make([]StatusUpdate, 0)

	for _, m := range ns.Machines {
		all := []StatusUpdate{
			StatusUpdate{Machine: m.Hostname, Status: m.Health.Status, Name: "Health"},
			StatusUpdate{Machine: m.Hostname, Status: m.Mounts.Status, Name: "Mounts"},
			StatusUpdate{Machine: m.Hostname, Status: m.LocalBackup.Status, Name: "LocalBackup"},
		}

		if m.OffsiteBackup != nil {
			all = append(all, StatusUpdate{Machine: m.Hostname, Name: "OffsiteBackup", Status: m.OffsiteBackup.Status})
		}
		if m.Geophone != nil {
			all = append(all, StatusUpdate{Machine: m.Hostname, Name: "Geophone", Status: m.Geophone.Status})
		}
		if m.Uploader != nil {
			all = append(all, StatusUpdate{Machine: m.Hostname, Name: "Uploader", Status: m.Uploader.Status})
		}

		machineStatus := Good
		for _, su := range all {
			machineStatus = machineStatus.Prioritize(su.Status)

			if su.Status != Good {
				problems = append(problems, su)
			}
		}
		newStatus = newStatus.Prioritize(machineStatus)
	}

	if notifs.PreviousStatus != newStatus {
		log.Printf("State change %v -> %v (%d)", notifs.PreviousStatus, newStatus, notifs.Attempt)
		notifs.Attempt += 1
		if notifs.Attempt == 3 {
			notifs.PreviousStatus = newStatus
			notifs.Attempt = 0
			notify = true
		}
	} else {
		notifs.Attempt = 0
	}
	if notifs.PreviousStatus != Good {
		interval = 6 * time.Hour
	}
	if notifs.LastStatus.Add(interval).Before(time.Now()) {
		log.Printf("Notifying (reminder)")
		notify = true
	}

	upFor := time.Now().Sub(notifs.StartTime)
	gracePeriod := (20 * time.Minute) > upFor
	if notify {
		notifs.LastStatus = time.Now()
		twilio := gotwilio.NewTwilioClient(twilioSid, twilioToken)

		message := fmt.Sprintf("%s https://code.conservify.org/glacier", newStatus)
		if gracePeriod {
			message = "(Grace) " + message
		}
		if len(problems) > 0 {
			message = message + fmt.Sprintf(" %v", problems)
		}
		for _, to := range toNumbers {
			log.Printf("Notifying %v %v", problems, to)
			twilio.SendSMS(fromNumber, to, message, "", "")
		}
	}
}

func SendStatus(ni *NetworkInfo) {
	time.Sleep(2 * time.Second)

	notifs := &NotificationStatus{
		PreviousStatus: Unknown,
		Attempt:        0,
		LastStatus:     time.Now(),
		StartTime:      time.Now(),
	}
	for {
		notifs.sendSingleStatus(ni)

		time.Sleep(5 * time.Second)
	}
}
