package main

import (
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"golang.org/x/crypto/ssh"
	"io"
	"io/ioutil"
	"log"
	"os"
	"regexp"
	"strconv"
	"strings"
	"time"
)

type SSHCommand struct {
	Path   string
	Env    []string
	Stdin  io.Reader
	Stdout io.Writer
	Stderr io.Writer
}

type SSHClient struct {
	Config *ssh.ClientConfig
	Host   string
	Port   int
}

func (client *SSHClient) RunCommand(cmd *SSHCommand) (string, string, error) {
	var (
		session *ssh.Session
		err     error
	)

	if session, err = client.newSession(); err != nil {
		return "", "", err
	}
	defer session.Close()

	if err = client.prepareCommand(session, cmd); err != nil {
		return "", "", err
	}

	var stdoutBuf bytes.Buffer
	var stderrBuf bytes.Buffer
	session.Stdout = &stdoutBuf
	session.Stderr = &stderrBuf
	err = session.Run(cmd.Path)

	return stdoutBuf.String(), stderrBuf.String(), err
}

func (client *SSHClient) prepareCommand(session *ssh.Session, cmd *SSHCommand) error {
	for _, env := range cmd.Env {
		variable := strings.Split(env, "=")
		if len(variable) == 2 {
			if err := session.Setenv(variable[0], variable[1]); err != nil {
				return err
			}
		}
	}

	return nil
}

func (client *SSHClient) newSession() (*ssh.Session, error) {
	connection, err := ssh.Dial("tcp", fmt.Sprintf("%s:%d", client.Host, client.Port), client.Config)
	if err != nil {
		return nil, fmt.Errorf("Failed to dial: %s", err)
	}

	session, err := connection.NewSession()
	if err != nil {
		return nil, fmt.Errorf("Failed to create session: %s", err)
	}

	return session, nil
}

func PublicKeyFile(file string) ssh.AuthMethod {
	buffer, err := ioutil.ReadFile(file)
	if err != nil {
		return nil
	}

	key, err := ssh.ParsePrivateKey(buffer)
	if err != nil {
		return nil
	}
	return ssh.PublicKeys(key)
}

type MountInfo struct {
	Name      string
	Available int64
	Total     int64
}

type DiskSpaceCommand struct {
	Mounts []MountInfo
}

func (c *DiskSpaceCommand) Execute(cl *SSHClient) error {
	cmd := &SSHCommand{
		Path: "df",
	}

	o, _, err := cl.RunCommand(cmd)
	if err != nil {
		return err
	}

	p := regexp.MustCompile("\\s+")
	lines := strings.Split(o, "\n")
	for _, line := range lines[1:] {
		parts := p.Split(line, -1)
		if len(parts) == 6 {
			total, _ := strconv.ParseInt(parts[1], 10, 64)
			available, _ := strconv.ParseInt(parts[3], 10, 64)

			c.Mounts = append(c.Mounts, MountInfo{
				Name:      parts[0],
				Total:     total,
				Available: available,
			})
		}
	}

	return nil
}

type UptimeCommand struct {
	Uptime float64
}

func (c *UptimeCommand) Execute(cl *SSHClient) error {
	cmd := &SSHCommand{
		Path: "head /proc/uptime",
	}

	o, _, err := cl.RunCommand(cmd)
	if err != nil {
		return err
	}

	parts := strings.Split(strings.TrimSpace(o), " ")

	c.Uptime, _ = strconv.ParseFloat(parts[0], 32)

	return nil
}

type PingCommand struct {
	Available   bool
	Transmitted int64
	Received    int64
	Loss        int64
	Latency     int64
}

func (c *PingCommand) Execute(cl *SSHClient, who string) error {
	cmd := &SSHCommand{
		Path: fmt.Sprintf("ping -nq -c 1 %s", who),
	}

	o, _, err := cl.RunCommand(cmd)
	if err != nil {
		fmt.Printf("%s\n", err)
	}

	p := regexp.MustCompile("(\\d+) packets transmitted, (\\d+) received, (\\d+)% packet loss, time (\\d+)ms")
	m := p.FindAllStringSubmatch(o, 1)
	if len(m) > 0 {
		c.Transmitted, _ = strconv.ParseInt(m[0][1], 10, 32)
		c.Received, _ = strconv.ParseInt(m[0][2], 10, 32)
		c.Loss, _ = strconv.ParseInt(m[0][3], 10, 32)
		c.Latency, _ = strconv.ParseInt(m[0][4], 10, 32)
		c.Available = c.Received > 0
	}

	return nil
}

type ChargeInformationCommand struct {
	Time                        time.Time
	ChargeCurrent               float32
	ArrayCurrent                float32
	BatteryTerminalVoltage      float32
	ArrayVoltage                float32
	LoadVoltage                 float32
	BatteryCurrentNet           float32
	LoadCurrent                 float32
	BatterySenseVoltage         float32
	BatteryVoltageSlowFilter    float32
	BatteryCurrentNetSlowFIlter float32

	HeatsinkTemperature float32
	BatteryTemperature  float32
	AmbientTemperature  float32

	ChargeState string
	LoadState   string
	LedState    string

	LoadCurrentCompensated float32
	LoadHvdVoltage         float32

	PowerOut              float32
	ArrayTargetVoltage    float32
	MaximumBatteryVoltage float32
	MinimumBatteryVoltage float32
	AmpHourCharge         float32
	AmpHourLoad           float32

	TimeInAbsorption   uint16
	TimeInEqualization uint16
	TimeInFloat        uint16
}

func parseFloat32(value string) float32 {
	f, _ := strconv.ParseFloat(value, 32)
	return float32(f)
}

func parseUint16(value string) uint16 {
	i, _ := strconv.ParseInt(value, 10, 16)
	return uint16(i)
}

func (c *ChargeInformationCommand) Execute(cl *SSHClient, path string) error {
	cmd := &SSHCommand{
		Path: fmt.Sprintf("tail -n 1 %s", path),
	}

	o, _, err := cl.RunCommand(cmd)
	if err != nil {
		return err
	}

	f := strings.Split(strings.TrimSpace(o), ",")

	c.Time, _ = time.Parse("2006-01-02 15:03:04.99999999 +0000 UTC", f[0])

	c.ChargeCurrent = parseFloat32(f[1])
	c.ArrayCurrent = parseFloat32(f[2])
	c.BatteryTerminalVoltage = parseFloat32(f[3])
	c.ArrayVoltage = parseFloat32(f[4])
	c.LoadVoltage = parseFloat32(f[5])
	c.BatteryCurrentNet = parseFloat32(f[6])
	c.LoadCurrent = parseFloat32(f[7])
	c.BatterySenseVoltage = parseFloat32(f[8])
	c.BatteryVoltageSlowFilter = parseFloat32(f[9])
	c.BatteryCurrentNetSlowFIlter = parseFloat32(f[10])

	c.HeatsinkTemperature = parseFloat32(f[11])
	c.BatteryTemperature = parseFloat32(f[12])
	c.AmbientTemperature = parseFloat32(f[13])

	c.ChargeState = f[14]
	c.LoadState = f[15]
	c.LedState = f[16]

	c.LoadCurrentCompensated = parseFloat32(f[17])
	c.LoadHvdVoltage = parseFloat32(f[18])

	c.PowerOut = parseFloat32(f[19])
	c.ArrayTargetVoltage = parseFloat32(f[20])
	c.MaximumBatteryVoltage = parseFloat32(f[21])
	c.MinimumBatteryVoltage = parseFloat32(f[22])
	c.AmpHourCharge = parseFloat32(f[23])
	c.AmpHourLoad = parseFloat32(f[24])

	c.TimeInAbsorption = parseUint16(f[25])
	c.TimeInEqualization = parseUint16(f[26])
	c.TimeInFloat = parseUint16(f[27])

	return nil
}

type ContainerInfo struct {
	Name       string
	Id         string
	Running    bool
	StartedAt  time.Time
	FinishedAt time.Time
}

type DockerStatusCommand struct {
	Containers []ContainerInfo
}

func (c *DockerStatusCommand) Execute(cl *SSHClient) error {
	cmd := &SSHCommand{
		Path: "sudo docker inspect -f '{{.Id}},{{.State.Running}},{{.State.StartedAt}},{{.State.FinishedAt}},{{.Name}}' $(sudo docker ps -q)",
	}

	o, _, err := cl.RunCommand(cmd)
	if err != nil {
		return err
	}

	lines := strings.Split(o, "\n")
	for _, line := range lines[1:] {
		parts := strings.Split(line, ",")
		if len(parts) == 5 {
			layout := "2006-01-02T15:04:05.999999999Z"
			startedAt, err := time.Parse(layout, parts[2])
			if err != nil {
				log.Printf("%s\n", err)
			}
			finishedAt, err := time.Parse(layout, parts[3])
			if err != nil {
				log.Printf("%s\n", err)
			}

			c.Containers = append(c.Containers, ContainerInfo{
				Id:         parts[0],
				Running:    parts[1] == "true",
				Name:       parts[4],
				StartedAt:  startedAt,
				FinishedAt: finishedAt,
			})
		}
	}

	return nil
}

type Options struct {
	Daemon   bool
	Host     string
	Port     int
	User     string
	Key      string
	Interval int
}

type CheckConfig struct {
	Name   string
	Kind   string
	Params map[string]string
}

type Configuration struct {
	Checks []CheckConfig
}

func Run(cl *SSHClient, cfg *Configuration) {
	var err error

	for _, check := range cfg.Checks {
		switch check.Kind {
		case "DockerStatusCommand":
			log.Printf("Running Check: %s[%s]", check.Name, check.Kind)

			docker := &DockerStatusCommand{}
			err = docker.Execute(cl)
			if err != nil {
				log.Printf("%s", err)
			}
		case "UptimeCommand":
			log.Printf("Running Check: %s[%s]", check.Name, check.Kind)

			uptime := &UptimeCommand{}
			err = uptime.Execute(cl)
			if err != nil {
				log.Printf("%s", err)
			}
		case "DiskSpaceCommand":
			log.Printf("Running Check: %s[%s]", check.Name, check.Kind)

			disk := &DiskSpaceCommand{}
			err = disk.Execute(cl)
			if err != nil {
				log.Printf("%s", err)
			}
		case "PingCommand":
			log.Printf("Running Check: %s[%s]", check.Name, check.Kind)

			ping := &PingCommand{}
			err = ping.Execute(cl, check.Params["address"])
			if err != nil {
				log.Printf("%s", err)
			}
		case "ChargeInformationCommand":
			log.Printf("Running Check: %s[%s]", check.Name, check.Kind)

			charge := &ChargeInformationCommand{}
			err = charge.Execute(cl, check.Params["path"])
			if err != nil {
				log.Printf("%s", err)
			}
		default:
			log.Printf("Unknown Check: %s", check.Kind)
		}
	}
}

func main() {
	options := new(Options)

	flag.BoolVar(&options.Daemon, "daemon", false, "run as a daemon")
	flag.IntVar(&options.Interval, "interval", 60*10, "check interval")
	flag.StringVar(&options.Host, "host", "", "hostname")
	flag.IntVar(&options.Port, "port", 22, "port")
	flag.StringVar(&options.Key, "key", "", "key file")
	flag.StringVar(&options.User, "user", "", "user")

	flag.Parse()

	if options.Host == "" || options.User == "" || options.Key == "" {
		flag.PrintDefaults()
		os.Exit(1)
	}

	sshConfig := &ssh.ClientConfig{
		User: options.User,
		Auth: []ssh.AuthMethod{
			PublicKeyFile(options.Key),
		},
		HostKeyCallback: ssh.InsecureIgnoreHostKey(),
	}

	cl := &SSHClient{
		Config: sshConfig,
		Host:   options.Host,
		Port:   options.Port,
	}

	cfg := Configuration{
		Checks: []CheckConfig{
			CheckConfig{Name: "docker", Kind: "DockerStatusCommand", Params: make(map[string]string)},
			CheckConfig{Name: "uptime", Kind: "UptimeCommand", Params: make(map[string]string)},
			CheckConfig{Name: "disk", Kind: "DiskSpaceCommand", Params: make(map[string]string)},
			CheckConfig{Name: "charge", Kind: "ChargeInformationCommand", Params: map[string]string{
				"path": "morningstar.csv",
			},
			},
			CheckConfig{Name: "glacier", Kind: "PingCommand", Params: map[string]string{
				// "address": "8.8.8.8",
				"address": "glacier",
			},
			},
		},
	}

	if (false) {
		w, _ := json.MarshalIndent(cfg, "", "   ")
		fmt.Printf("%s\n", string(w))
	}

	for {
		Run(cl, &cfg)

		if !options.Daemon {
			break
		}

		time.Sleep(time.Duration(options.Interval) * time.Second)
	}
}
