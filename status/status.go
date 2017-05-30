package main

import (
	"context"
	"fmt"
	"bytes"
	"syscall"
	"github.com/docker/docker/api/types"
	"github.com/docker/docker/client"
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
	Good HealthStatus = "Good"
	Warning HealthStatus = "Warning"
	Fatal HealthStatus = "Fatal"
)

type HealthCheck struct {
	Name string
	Status HealthStatus
	Notes string
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

	// fmt.Printf("All: %.2f GB\n", float64(disk.All)/float64(GB))
	// freeGb := float64(disk.Free)/float64(GB)
	// usedGb := float64(disk.Used)/float64(GB)

	percentaage := float64(disk.Used)/float64(disk.All)*100.0

	health = new(HealthCheck)
	health.Name = fmt.Sprintf("Disk: %s", path)
	health.Notes = fmt.Sprintf("%.2f % used", percentaage)
	health.Status = DiskPercentageToStatus(percentaage)
	return
}

func CheckDiskHealth() (health []*HealthCheck) {
	health = make([]*HealthCheck, 2)
	health = append(health, CheckDisk("/"))
	health = append(health, CheckDisk("/data"))
	return
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

		health := new(HealthCheck)
		health.Name = fmt.Sprintf("Container: %s", container.Names)

		buf := new(bytes.Buffer)
		buf.ReadFrom(logs)
		health.Notes = buf.String() 

		if details.State.Running {
			health.Status = Good
		} else {
			health.Status = Fatal
		}
	}
	return
}

func main() {
	healths := make([]*HealthCheck, 0)
	healths = append(healths, CheckDiskHealth()...)
	healths = append(healths, CheckDockerHealth()...)
}
