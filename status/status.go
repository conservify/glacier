package main

import (
	"bytes"
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"github.com/docker/docker/api/types"
	"github.com/docker/docker/client"
	"html/template"
	"log"
	"net/http"
	"net/smtp"
	"os"
	"strconv"
	"strings"
	"syscall"
	"time"
)

type DiskStatus struct {
	All  uint64 `json:"all"`
	Used uint64 `json:"used"`
	Free uint64 `json:"free"`
}

func DiskUsage(path string) (disk DiskStatus) {
	fs := syscall.Statfs_t{}
	err := syscall.Statfs(path, &fs)
	if err != nil {
		return
	}
	disk.All = fs.Blocks * uint64(fs.Bsize)
	disk.Free = fs.Bavail * uint64(fs.Bsize)
	disk.Used = disk.All - disk.Free

	return
}

const (
	B  = 1
	KB = 1024 * B
	MB = 1024 * KB
	GB = 1024 * MB
)

type HealthStatus string

const (
	Good    HealthStatus = "Good"
	Warning HealthStatus = "Warning"
	Fatal   HealthStatus = "Fatal"
)

type HealthCheck struct {
	Name   string
	Status HealthStatus
	Notes  string
}

func DiskPercentageToStatus(percentage float64) HealthStatus {
	if percentage < 10 {
		return Fatal
	} else if percentage < 30 {
		return Warning
	} else {
		return Good
	}
}

func CheckDisk(path string) (health *HealthCheck) {
	disk := DiskUsage(path)

	freeGb := float64(disk.Free) / float64(GB)
	percentage := float64(disk.Used) / float64(disk.All) * 100.0

	health = new(HealthCheck)
	health.Name = fmt.Sprintf("Disk: %s", path)
	health.Notes = fmt.Sprintf("%.2f%% used (%.2f GB free)", percentage, freeGb)
	health.Status = DiskPercentageToStatus(percentage)
	return
}

func CheckDiskHealth() (healths []*HealthCheck) {
	healths = make([]*HealthCheck, 0)
	healths = append(healths, CheckDisk("/"))
	if _, err := os.Stat("/data"); !os.IsNotExist(err) {
		healths = append(healths, CheckDisk("/data"))
	}
	return
}

func StripNonPrintable(str string) string {
	return strings.Map(func(r rune) rune {
		if r >= 32 && r < 127 {
			return r
		}
		return -1
	}, str)
}

func CheckDockerHealth() (healths []*HealthCheck) {
	cli, err := client.NewEnvClient()
	if err != nil {
		panic(err)
	}

	ctx := context.Background()
	containers, err := cli.ContainerList(ctx, types.ContainerListOptions{All: true})
	if err != nil {
		panic(err)
	}

	for _, container := range containers {
		details, err := cli.ContainerInspect(ctx, container.ID)
		if err != nil {
			panic(err)
		}

		logsOptions := types.ContainerLogsOptions{ShowStdout: true, ShowStderr: true, Tail: "10"}
		logs, err := cli.ContainerLogs(ctx, container.ID, logsOptions)
		if err != nil {
			panic(err)
		}

		buf := new(bytes.Buffer)
		buf.ReadFrom(logs)

		health := new(HealthCheck)
		health.Name = fmt.Sprintf("Container: %s", container.Names[0])
		health.Notes = fmt.Sprintf("%s\n%s", container.Status, StripNonPrintable(buf.String()))

		if details.State.Running {
			health.Status = Good
		} else {
			health.Status = Fatal
		}

		healths = append(healths, health)
	}
	return
}

func Combine(statuses []*StatusCheck, globalStatus HealthStatus) (combined string) {
	data := &TemplateData{statuses, globalStatus}

	t, err := template.ParseFiles("status.html")
	if err != nil {
		panic(err)
	}
	buf := new(bytes.Buffer)
	if err = t.Execute(buf, data); err != nil {
		panic(err)
	}
	return buf.String()
}

type EmailUser struct {
	Username    string
	Password    string
	EmailServer string
	Port        int
}

type TemplateData struct {
	Checks       []*StatusCheck
	GlobalStatus HealthStatus
}

func SendEmail(globalStatus HealthStatus, to string, body string) {
	var err error

	mime := "MIME-version: 1.0;\nContent-Type: text/html; charset=\"UTF-8\";\n\n"
	subject := fmt.Sprintf("Subject: Glacier Health: %s\n", globalStatus)
	msg := []byte(subject + mime + "\n" + body)

	emailUser := &EmailUser{smtpUsername, smtpPassword, "smtp.gmail.com", 587}
	auth := smtp.PlainAuth("", emailUser.Username, emailUser.Password, emailUser.EmailServer)
	server := emailUser.EmailServer + ":" + strconv.Itoa(emailUser.Port)

	err = smtp.SendMail(server, auth, emailUser.Username, []string{"jlewalle@gmail.com"}, msg)
	if err != nil {
		log.Print("ERROR: Unable to send email", err)
	}
}

type StatusCheck struct {
	Name         string
	Healths      []*HealthCheck
	GlobalStatus HealthStatus
}

func Check() (check *StatusCheck) {
	name, _ := os.Hostname()
	check = new(StatusCheck)
	check.Name = name
	check.Healths = make([]*HealthCheck, 0)
	check.Healths = append(check.Healths, CheckDiskHealth()...)
	check.Healths = append(check.Healths, CheckDockerHealth()...)
	check.GlobalStatus = Good
	for _, health := range check.Healths {
		if health.Status == Warning && check.GlobalStatus == Good {
			check.GlobalStatus = Warning
		}
		if health.Status == Fatal {
			check.GlobalStatus = Fatal
		}
	}
	return
}

func StatusHandler(w http.ResponseWriter, r *http.Request) {
	b, _ := json.Marshal(Check())
	w.Write(b)
}

func QueryStatus(url string) (target *StatusCheck) {
	client := http.Client{Timeout: 10 * time.Second}
	r, err := client.Get(url)
	if err != nil {
		failure := new(HealthCheck)
		failure.Notes = fmt.Sprintf("Server unavailable.")
		failure.Name = url
		failure.Status = Fatal

		target = new(StatusCheck)
		target.GlobalStatus = Fatal
		target.Healths = append(target.Healths, failure)
	} else {
		defer r.Body.Close()

		target = new(StatusCheck)
		err = json.NewDecoder(r.Body).Decode(target)
		if err != nil {
			panic(err)
		}
	}
	return
}

func CombineHealthStatus(statuses []*StatusCheck) HealthStatus {
	combined := Good
	for _, s := range statuses {
		if s.GlobalStatus == Warning && combined == Good {
			combined = Warning
		}
		if s.GlobalStatus == Fatal {
			combined = Fatal
		}
	}
	return combined
}

func main() {
	var server bool
	var test bool

	flag.BoolVar(&server, "server", false, "run a server")
	flag.BoolVar(&test, "test", false, "run a test check")

	flag.Parse()

	if test {
		b, _ := json.Marshal(Check())
		os.Stdout.Write(b)
		fmt.Printf("\n")
	}

	if server {
		fmt.Printf("Serving!\n")
		http.HandleFunc("/status.json", StatusHandler)
		http.ListenAndServe(":8000", nil)
	} else {
		statuses := make([]*StatusCheck, 0)
		for _, arg := range flag.Args() {
			fmt.Printf("Querying %s...\n", arg)
			status := QueryStatus(arg)
			statuses = append(statuses, status)
		}

		globalStatus := CombineHealthStatus(statuses)
		SendEmail(globalStatus, "jlewalle@gmail.com", Combine(statuses, globalStatus))
	}

}
