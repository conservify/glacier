package main

import (
	"bufio"
	"flag"
	"fmt"
	"github.com/hpcloud/tail"
	"log"
	"os"
	"regexp"
	"strconv"
	"strings"
	"sync"
	"time"
)

type NetworkInfo struct {
	Lock     sync.Mutex              `json:"lock"`
	Machines map[string]*MachineInfo `json:"machines"`
}

type GeophoneInfo struct {
	Frequency     int        `json:"frequency"`
	LastUpdatedAt time.Time  `json:"lastUpdatedAt"`
	Status        StatusType `json:"status"`
	Log           []string   `json:"log"`
}

type UploaderInfo struct {
	Frequency     int        `json:"frequency"`
	LastUpdatedAt time.Time  `json:"lastUpdatedAt"`
	Status        StatusType `json:"status"`
	Log           []string   `json:"log"`
}

type MountPointInfo struct {
	MountPoint string  `json:"mountPoint"`
	Size       int64   `json:"size"`
	Available  int64   `json:"available"`
	Used       float64 `json:"used"`
}

type MountPointsInfo struct {
	Frequency     int                        `json:"frequency"`
	LastUpdatedAt time.Time                  `json:"lastUpdatedAt"`
	Mounts        map[string]*MountPointInfo `json:"mounts"`
}

type HealthInfo struct {
	Log           []string   `json:"log"`
	Frequency     int        `json:"frequency"`
	LastUpdatedAt time.Time  `json:"lastUpdatedAt"`
	Users         string     `json:"users"`
	Uptime        string     `json:"uptime"`
	LoadAverage   string     `json:"loadAverage"`
	Status        StatusType `json:"status"`
}

type BackupInfo struct {
	Frequency     int        `json:"frequency"`
	LastUpdatedAt time.Time  `json:"lastUpdatedAt"`
	Status        StatusType `json:"status"`
	Log           []string   `json:"log"`
}

type ResilienceCheckInfo struct {
	Frequency     int        `json:"frequency"`
	LastUpdatedAt time.Time  `json:"lastUpdatedAt"`
	Status        StatusType `json:"status"`
	Log           []string   `json:"log"`
}

type MachineInfo struct {
	Hostname      string              `json:"hostname"`
	LastMessageAt time.Time           `json:"lastMessageAt"`
	Health        HealthInfo          `json:"health"`
	Mounts        MountPointsInfo     `json:"mounts"`
	LocalBackup   BackupInfo          `json:"localBackup"`
	OffsiteBackup BackupInfo          `json:"offsiteBackup"`
	Geophone      GeophoneInfo        `json:"geophone"`
	Uploader      UploaderInfo        `json:"uploader"`
	Resilience    ResilienceCheckInfo `json:"resilience"`
}

func NewMachineInfo(name string) *MachineInfo {
	return &MachineInfo{
		Hostname: name,
		Mounts: MountPointsInfo{
			Mounts: make(map[string]*MountPointInfo),
		},
		Health: HealthInfo{},
	}
}

type SysLogLine struct {
	Raw      string
	StampRaw string
	Stamp    time.Time
	Hostname string
	Facility string
	Message  string
}

type LogFileParser struct {
}

var cleanupFacilityRe = regexp.MustCompile("\\[\\d+\\]")

func (l *LogFileParser) Parse(line string) *SysLogLine {
	timeLength := len("Aug  4 21:46:55")
	if len(line) < timeLength+1 {
		return nil
	}
	parts := strings.SplitN(line[timeLength+1:], " ", 3)
	wholeFacility := strings.TrimSpace(strings.Replace(parts[1], ":", "", -1))
	now := time.Now()
	stampRaw := line[0:timeLength]
	stamp, err := time.Parse("2006 Jan  2 15:04:05", fmt.Sprintf("%d %s", now.Year(), stampRaw))
	if err != nil {
		log.Printf("Error parsing time %s", err)
		return nil
	}
	facility := cleanupFacilityRe.ReplaceAllString(wholeFacility, "")
	return &SysLogLine{
		Raw:      line,
		StampRaw: stampRaw,
		Stamp:    stamp,
		Hostname: parts[0],
		Facility: facility,
		Message:  strings.TrimSpace(parts[2]),
	}
}

// Aug  4 21:45:01 lodge local-backup: Local backup done (LOCAL_BACKUP:GOOD)

// Aug  4 21:48:59 lodge status:  21:48:59 up 18 min,  2 users,  load average: 0.00, 0.01, 0.00

var uptimeRe = regexp.MustCompile("([\\d:]+) up (.+),\\s+(\\d+) users,\\s+load average: (.+)")

func updateLog(m string, log []string) []string {
	l := append(log, m)
	if len(l) > 10 {
		return l[1:len(l)]
	}
	return l
}

func (l *LogFileParser) TryParseHealth(sl *SysLogLine, m *MachineInfo) {
	if sl.Facility != "status" {
		return
	}

	v := uptimeRe.FindAllStringSubmatch(sl.Message, -1)
	if v == nil {
		return
	}

	m.Health = HealthInfo{
		LastUpdatedAt: sl.Stamp,
		Uptime:        v[0][2],
		Users:         v[0][3],
		LoadAverage:   v[0][4],
		Log:           updateLog(sl.Message, m.Health.Log),
	}
}

// Aug  5 00:13:03 glacier disk-space: Filesystem                Size      Used Available Use% Mounted on
// Aug  5 00:13:03 glacier disk-space: tmpfs                   831.0M     53.8M    777.2M   6% /
// Aug  5 00:13:03 glacier disk-space: tmpfs                   461.7M         0    461.7M   0% /dev/shm
// Aug  5 00:13:03 glacier disk-space: /dev/mmcblk0p2          234.5M     87.7M    130.4M  40% /mnt/mmcblk0p2
// Aug  5 00:13:03 glacier disk-space: /dev/sda1               343.7G     66.7M    326.1G   0% /backup

// Aug  5 00:13:03 lodge disk-space: Filesystem                Size      Used Available Use% Mounted on
// Aug  5 00:13:03 lodge disk-space: tmpfs                   831.0M     53.9M    777.1M   6% /
// Aug  5 00:13:03 lodge disk-space: tmpfs                   461.7M         0    461.7M   0% /dev/shm
// Aug  5 00:13:03 lodge disk-space: /dev/mmcblk0p2          234.5M     87.9M    130.3M  40% /mnt/mmcblk0p2
// Aug  5 00:13:03 lodge disk-space: /dev/sda1               343.7G      3.7G    322.5G   1% /backup

var spacesRe = regexp.MustCompile("\\s+")
var bytesRe = regexp.MustCompile("(.+)([kMG])")
var makeNumberRe = regexp.MustCompile("[MG%]")

func (l *LogFileParser) TryParseDisk(sl *SysLogLine, m *MachineInfo) {
	if sl.Facility != "disk-space" {
		return
	}

	parts := spacesRe.Split(sl.Message, -1)
	if parts[1] == "Size" {
		return
	}

	size, err := parseBytes(parts[1])
	if err != nil {
		log.Printf("Error parsing '%v'", parts[1])
		return
	}
	available, err := parseBytes(parts[3])
	if err != nil {
		log.Printf("Error parsing '%v'", parts[3])
		return
	}
	used, err := strconv.ParseFloat(makeNumberRe.ReplaceAllString(parts[4], ""), 32)
	if err != nil {
		log.Printf("Error parsing %v", makeNumberRe.ReplaceAllString(parts[4], ""))
		return
	}

	mp := &MountPointInfo{
		MountPoint: parts[0],
		Size:       size,
		Available:  available,
		Used:       used,
	}

	m.Mounts.LastUpdatedAt = sl.Stamp
	m.Mounts.Mounts[mp.MountPoint] = mp

	log.Printf("MountPoint: %v", mp)
}

var statusRe = regexp.MustCompile("\\((\\S+):(\\S+)\\)")

type SimpleInlineStatus struct {
	Which  string
	Status string
}

func toStatusType(s string) StatusType {
	if s == "GOOD" {
		return Good
	}
	if s == "FATAL" {
		return Fatal
	}
	return Unknown
}

func ParseSimpleInlineStatus(text string) *SimpleInlineStatus {
	v := statusRe.FindAllStringSubmatch(text, -1)
	if v == nil {
		return nil
	}

	ss := SimpleInlineStatus{
		Which:  v[0][1],
		Status: v[0][2],
	}

	return &ss
}

func (l *LogFileParser) TryParseLocalBackup(sl *SysLogLine, m *MachineInfo) {
	s := ParseSimpleInlineStatus(sl.Message)
	if s != nil {
		if s.Which == "LOCAL_BACKUP" {
			log.Printf("Local backup %v (%s) %v", sl.Stamp, m.Hostname, s)
			m.LocalBackup = BackupInfo{
				Frequency:     5,
				LastUpdatedAt: sl.Stamp,
				Status:        toStatusType(s.Status),
				Log:           updateLog(sl.Message, m.LocalBackup.Log),
			}
		}
	} else {
		if sl.Facility == "local-backup" {
			m.LocalBackup.Log = updateLog(sl.Message, m.LocalBackup.Log)
		}
	}
}

func (l *LogFileParser) TryParseOffsiteBackup(sl *SysLogLine, m *MachineInfo) {
	s := ParseSimpleInlineStatus(sl.Message)
	if s != nil {
		if s.Which == "OFFSITE_BACKUP" {
			log.Printf("Offsite backup %v (%s) %v", sl.Stamp, m.Hostname, s)
			m.OffsiteBackup = BackupInfo{
				Frequency:     20,
				LastUpdatedAt: sl.Stamp,
				Status:        toStatusType(s.Status),
				Log:           updateLog(sl.Message, m.OffsiteBackup.Log),
			}
		}
	} else {
		if sl.Facility == "offsite-backup" {
			m.OffsiteBackup.Log = updateLog(sl.Message, m.OffsiteBackup.Log)
		}
	}
}

func (l *LogFileParser) TryParseGeophone(sl *SysLogLine, m *MachineInfo) {
	if sl.Facility == "adc" {
		m.Geophone = GeophoneInfo{
			LastUpdatedAt: sl.Stamp,
			Log:           updateLog(sl.Message, m.Geophone.Log),
		}
	}
}

func (l *LogFileParser) TryParseUploader(sl *SysLogLine, m *MachineInfo) {
	if sl.Facility == "uploader-geophone" {
		m.Uploader = UploaderInfo{
			LastUpdatedAt: sl.Stamp,
			Log:           updateLog(sl.Message, m.Uploader.Log),
		}
	}
}

func (l *LogFileParser) TryParseResilienceCheck(sl *SysLogLine, m *MachineInfo) {
	if sl.Facility == "resilience" {
		m.Resilience = ResilienceCheckInfo{
			Frequency:     32,
			LastUpdatedAt: sl.Stamp,
			Log:           updateLog(sl.Message, m.Resilience.Log),
			Status:        Good,
		}
	}
}

func (parser *LogFileParser) ProcessLine(ni *NetworkInfo, line string) {
	sl := parser.Parse(line)
	if sl == nil {
		return
	}

	ni.Lock.Lock()

	defer ni.Lock.Unlock()

	if ni.Machines[sl.Hostname] == nil {
		if false {
			ni.Machines[sl.Hostname] = NewMachineInfo(sl.Hostname)
		} else {
			return
		}
	}

	m := ni.Machines[sl.Hostname]
	m.LastMessageAt = sl.Stamp

	parser.TryParseHealth(sl, m)
	parser.TryParseDisk(sl, m)
	parser.TryParseLocalBackup(sl, m)
	parser.TryParseOffsiteBackup(sl, m)
	parser.TryParseGeophone(sl, m)
	parser.TryParseUploader(sl, m)
	parser.TryParseResilienceCheck(sl, m)
}

func main() {
	flag.Parse()

	ni := NetworkInfo{
		Machines: make(map[string]*MachineInfo),
	}

	go StartWebServer(&ni)

	go SendStatus(&ni)

	ni.Machines["lodge"] = NewMachineInfo("lodge")
	ni.Machines["glacier"] = NewMachineInfo("glacier")

	parser := &LogFileParser{}
	args := flag.Args()
	if len(args) > 0 {
		for {
			t, err := tail.TailFile(args[0], tail.Config{Follow: true})
			if err == nil {
				for line := range t.Lines {
					parser.ProcessLine(&ni, line.Text)
				}
			}

			time.Sleep(1 * time.Second)
		}
	} else {
		r := bufio.NewScanner(os.Stdin)
		for r.Scan() {
			l := r.Text()
			parser.ProcessLine(&ni, l)
		}
	}
}
