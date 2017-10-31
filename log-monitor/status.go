package main

import (
	"fmt"
	_ "log"
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

type NetworkStatus struct {
	Machines map[string]*MachineStatus `json:"machines"`
}

type GeophoneStatus struct {
	Info   GeophoneInfo `json:"info"`
	Status StatusType   `json:"status"`
}

type UploaderStatus struct {
	Info   UploaderInfo `json:"info"`
	Status StatusType   `json:"status"`
}

type MountPointsStatus struct {
	Info   MountPointsInfo `json:"info"`
	Status StatusType      `json:"status"`
}

type HealthStatus struct {
	Info   HealthInfo `json:"info"`
	Status StatusType `json:"status"`
}

type BackupStatus struct {
	Info   BackupInfo `json:"info"`
	Status StatusType `json:"status"`
}

type ResilienceCheckStatus struct {
	Info   ResilienceCheckInfo `json:"info"`
	Status StatusType          `json:"status"`
}

type CronCheckStatus struct {
	Info   CronCheckInfo `json:"info"`
	Status StatusType    `json:"status"`
}

type MorningStarCheckStatus struct {
	Info   MorningStarCheckInfo `json:"info"`
	Status StatusType           `json:"status"`
}

type MachineStatus struct {
	Hostname      string                  `json:"hostname"`
	Health        *HealthStatus           `json:"health"`
	Mounts        *MountPointsStatus      `json:"mounts"`
	LocalBackup   *BackupStatus           `json:"localBackup"`
	OffsiteBackup *BackupStatus           `json:"offsiteBackup"`
	Geophone      *GeophoneStatus         `json:"geophone"`
	Uploader      *UploaderStatus         `json:"uploader"`
	Resilience    *ResilienceCheckStatus  `json:"resilience"`
	Cron          *CronCheckStatus        `json:"cron"`
	MorningStar   *MorningStarCheckStatus `json:"morningStar"`
}

const offlineWarningAfter = -6 * time.Minute
const diskUnknownAfter = -10 * time.Minute
const geophoneInterval = -20 * time.Second
const uploaderInterval = -2 * time.Minute
const localBackupInterval = -10 * time.Minute
const offsiteBackupInterval = -10 * time.Minute
const resilienceCheckInterval = -32 * time.Minute
const cronCheckInterval = -6 * time.Minute
const morningStarCheckInterval = -70 * time.Minute

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

func checkDisk(m *MachineInfo) *MountPointsStatus {
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
	return &MountPointsStatus{
		Info:   m.Mounts,
		Status: status,
	}
}

func checkHealth(m *MachineInfo) *HealthStatus {
	status := Good
	offlineWarningThreshold := time.Now().Add(offlineWarningAfter)
	if m.LastMessageAt.Before(offlineWarningThreshold) {
		status = Fatal
	}
	return &HealthStatus{
		Info:   m.Health,
		Status: status,
	}
}

func checkUploader(m *MachineInfo) *UploaderStatus {
	status := Good
	if m.Uploader.LastUpdatedAt.Before(time.Now().Add(uploaderInterval)) {
		status = Unknown
	}
	return &UploaderStatus{
		Info:   m.Uploader,
		Status: status,
	}
}

func checkGeophone(m *MachineInfo) *GeophoneStatus {
	status := Good
	if m.Geophone.LastUpdatedAt.Before(time.Now().Add(geophoneInterval)) {
		status = Unknown
	}
	return &GeophoneStatus{
		Info:   m.Geophone,
		Status: status,
	}
}

func boolToStatus(b bool) StatusType {
	if b {
		return Good
	}
	return Fatal
}

func checkLocalBackup(m *MachineInfo) *BackupStatus {
	status := m.LocalBackup.Status
	if m.LocalBackup.LastUpdatedAt.Before(time.Now().Add(localBackupInterval)) {
		status = Unknown
	}
	return &BackupStatus{
		Info:   m.LocalBackup,
		Status: status,
	}
}

func checkOffsiteBackup(m *MachineInfo) *BackupStatus {
	status := m.OffsiteBackup.Status
	if m.OffsiteBackup.LastUpdatedAt.Before(time.Now().Add(offsiteBackupInterval)) {
		status = Unknown
	}
	return &BackupStatus{
		Info:   m.OffsiteBackup,
		Status: status,
	}
}

func checkResilienceCheck(m *MachineInfo) *ResilienceCheckStatus {
	status := m.Resilience.Status
	if m.Resilience.LastUpdatedAt.Before(time.Now().Add(resilienceCheckInterval)) {
		status = Unknown
	}
	return &ResilienceCheckStatus{
		Info:   m.Resilience,
		Status: status,
	}
}

func checkCronCheck(m *MachineInfo) *CronCheckStatus {
	status := m.Cron.Status
	if m.Cron.LastUpdatedAt.Before(time.Now().Add(cronCheckInterval)) {
		status = Unknown
	}
	return &CronCheckStatus{
		Info:   m.Cron,
		Status: status,
	}
}

func checkMorningStarCheck(m *MachineInfo) *MorningStarCheckStatus {
	status := m.MorningStar.Status
	if m.MorningStar.LastUpdatedAt.Before(time.Now().Add(morningStarCheckInterval)) {
		status = Unknown
	}
	return &MorningStarCheckStatus{
		Info:   m.MorningStar,
		Status: status,
	}
}

func ToNetworkStatus(ni *NetworkInfo) (ns *NetworkStatus, err error) {
	ni.Lock.Lock()
	defer ni.Lock.Unlock()

	lodge := ni.Machines["lodge"]
	glacier := ni.Machines["glacier"]

	ns = &NetworkStatus{
		Machines: make(map[string]*MachineStatus),
	}

	ns.Machines["lodge"] = &MachineStatus{
		Hostname:      lodge.Hostname,
		Health:        checkHealth(lodge),
		Mounts:        checkDisk(lodge),
		LocalBackup:   checkLocalBackup(lodge),
		OffsiteBackup: checkOffsiteBackup(lodge),
		Resilience:    checkResilienceCheck(lodge),
		Cron:          checkCronCheck(lodge),
		MorningStar:   checkMorningStarCheck(lodge),
	}

	ns.Machines["glacier"] = &MachineStatus{
		Hostname:      glacier.Hostname,
		Health:        checkHealth(glacier),
		Mounts:        checkDisk(glacier),
		// LocalBackup:   checkLocalBackup(glacier),
		OffsiteBackup: checkOffsiteBackup(glacier),
		Geophone:      checkGeophone(glacier),
		Uploader:      checkUploader(glacier),
		Resilience:    checkResilienceCheck(glacier),
		Cron:          checkCronCheck(glacier),
		MorningStar:   checkMorningStarCheck(glacier),
	}

	return
}
