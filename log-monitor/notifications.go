package main

import (
	"fmt"
	"github.com/sfreiberg/gotwilio"
	"log"
	"strconv"
	"time"
)

type StatusType string

const (
	Fatal   StatusType = "Fatal"
	Warning StatusType = "Warning"
	Good    StatusType = "Good"
	Unknown StatusType = "Unknown"
)

type StatusUpdate struct {
	Name    string
	Updated time.Time
	Body    string
	Status  StatusType
}

const offlineWarningAfter = -1 * time.Minute
const geophoneWarningAfter = -1 * time.Minute
const diskUnknownAfter = -10 * time.Minute
const geophoneInterval = -1 * time.Minute
const localBackupInterval = -8 * time.Minute
const offsiteBackupInterval = -20 * time.Minute

const GB = 1024 * 1024 * 1024
const MB = 1024 * 1024
const KB = 1024

func parseBytes(s string) (int64, error) {
	m := bytesRe.FindAllStringSubmatch(s, -1)
	if m == nil {
		return 0, nil
	}
	var mul = float64(1)
	if m[0][2] == "G" {
		mul = GB

	} else if m[0][2] == "M" {
		mul = MB

	} else if m[0][2] == "k" {
		mul = KB
	}
	base, err := strconv.ParseFloat(m[0][1], 32)
	if err != nil {
		return 0, err
	}
	return int64(base * mul), nil
}

func prettyBytes(b int64) string {
	if b > GB {
		return fmt.Sprintf("%.2fG", float32(b)/float32(GB))
	}
	if b > MB {
		return fmt.Sprintf("%.2fM", float32(b)/float32(MB))
	}
	return fmt.Sprintf("%.2fk", float32(b)/float32(KB))
}

func checkDisk(m *Machine) StatusUpdate {
	status := Good
	body := ""
	if m.Mounts.LastUpdatedAt.Before(time.Now().Add(diskUnknownAfter)) {
		status = Unknown
	} else {
		for _, mp := range m.Mounts.Mounts {
			if mp.Used > 90 {
				status = Warning
			} else if mp.Used > 95 {
				status = Fatal
			}
			body += fmt.Sprintf("%20s %8s %8s %2.0f%%\n",
				mp.MountPoint,
				prettyBytes(mp.Size),
				prettyBytes(mp.Available),
				mp.Used)
		}
	}
	return StatusUpdate{
		Name:    "Disk",
		Body:    body,
		Updated: m.Geophone.LastUpdatedAt,
		Status:  status,
	}
}

func checkOnline(m *Machine) StatusUpdate {
	status := Good
	offlineWarningThreshold := time.Now().Add(offlineWarningAfter)
	if m.LastMessageAt.Before(offlineWarningThreshold) {
		status = Fatal
	}
	return StatusUpdate{
		Name: "Online",
		Body: fmt.Sprintf("uptime: %s, users: %s, load average: %s",
			m.Status.Uptime,
			m.Status.Users,
			m.Status.LoadAverage),
		Updated: m.LastMessageAt,
		Status:  status,
	}
}

func checkGeophone(m *Machine) StatusUpdate {
	status := Good
	if !m.Geophone.GpsLock {
		status = Warning
	}
	if m.Geophone.LastUpdatedAt.Before(time.Now().Add(geophoneInterval)) {
		status = Unknown
	}
	return StatusUpdate{
		Name:    "Geophone",
		Body:    "",
		Updated: m.Geophone.LastUpdatedAt,
		Status:  status,
	}
}

func boolToStatus(b bool) StatusType {
	if b {
		return Good
	}
	return Fatal
}

func checkLocalBackup(m *Machine) StatusUpdate {
	status := boolToStatus(m.LocalBackup.Failed)
	if m.LocalBackup.LastUpdatedAt.Before(time.Now().Add(localBackupInterval)) {
		status = Unknown
	}
	return StatusUpdate{
		Name:    "Local Backup",
		Body:    "",
		Updated: m.LocalBackup.LastUpdatedAt,
		Status:  status,
	}
}

func checkOffsiteBackup(m *Machine) StatusUpdate {
	status := boolToStatus(m.OffsiteBackup.Failed)
	if m.OffsiteBackup.LastUpdatedAt.Before(time.Now().Add(offsiteBackupInterval)) {
		status = Unknown
	}
	return StatusUpdate{
		Name:    "Offsite Backup",
		Body:    "",
		Updated: m.OffsiteBackup.LastUpdatedAt,
		Status:  status,
	}
}

type NotificationStatus struct {
	PreviousStatus StatusType
	LastStatus     time.Time
}

func (s StatusType) Ordinal() int {
	switch s {
	case Good:
		return 3
	case Warning:
		return 2
	case Unknown:
		return 1
	case Fatal:
		return 0
	}
	return 0
}

func (s StatusType) Prioritize(o StatusType) StatusType {
	if o.Ordinal() < s.Ordinal() {
		return o
	}
	return s
}

func SendStatus(ns *NetworkStatus) {
	time.Sleep(2 * time.Second)

	notifs := NotificationStatus{PreviousStatus: Unknown}
	for {
		ns.Lock.Lock()

		notify := false
		interval := 24 * time.Hour
		newStatus := Good

		for _, m := range ns.Machines {
			offline := checkOnline(m)
			disk := checkDisk(m)
			geophone := checkGeophone(m)
			localBackup := checkLocalBackup(m)
			offsiteBackup := checkOffsiteBackup(m)
			all := []StatusUpdate{
				offline,
				disk,
				geophone,
				localBackup,
				offsiteBackup,
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

		ns.Lock.Unlock()

		time.Sleep(5 * time.Second)
	}

}
