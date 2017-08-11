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
	}
	if newStatus != Good {
		interval = 6 * time.Hour
	}
	if notifs.LastStatus.Add(interval).Before(time.Now()) {
		log.Printf("Notifying (reminder)")
		notify = true
	}

	upFor := time.Now().Sub(notifs.StartTime)
	gracePeriod := (20 * time.Minute) > upFor
	if notify {
		log.Printf("Notifying %v", problems)
		notifs.LastStatus = time.Now()
		twilio := gotwilio.NewTwilioClient(twilioSid, twilioToken)

		from := "+12132617278"
		to := "+19515438308"
		message := fmt.Sprintf("%s https://code.conservify.org/glacier", newStatus)
		if gracePeriod {
			message = "(Grace) " + message
		}
		if len(problems) > 0 {
			message = message + fmt.Sprintf(" %v", problems)
		}
		twilio.SendSMS(from, to, message, "", "")
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
