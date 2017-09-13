package main

import (
	"bufio"
	"encoding/binary"
	"flag"
	"fmt"
	"github.com/goburrow/modbus"
	"log"
	"log/syslog"
	"math"
	"os"
	"path/filepath"
	"strings"
	"time"
)

// taken from OGRE 3D rendering engine
func float16toUint32(yy uint16) (d uint32) {
	y := uint32(yy)
	s := (y >> 15) & 0x00000001
	e := (y >> 10) & 0x0000001f
	m := y & 0x000003ff

	if e == 0 {
		if m == 0 { // Plus or minus zero
			return s << 31
		} else { // Denormalized number -- renormalize it
			for (m & 0x00000400) == 0 {
				m <<= 1
				e -= 1
			}
			e += 1
			m &= ^uint32(0x00000400)
		}
	} else if e == 31 {
		if m == 0 { // Inf
			return (s << 31) | 0x7f800000
		} else { // NaN
			return (s << 31) | 0x7f800000 | (m << 13)
		}
	}
	e = e + (127 - 15)
	m = m << 13
	return (s << 31) | (e << 23) | m
}

type ProStarMpptData struct {
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

type ProStarMppt struct {
	handler *modbus.RTUClientHandler
	mc      modbus.Client
	data    ProStarMpptData
}

func NewProStarMppt() (controller *ProStarMppt) {
	controller = new(ProStarMppt)

	return
}

func (c *ProStarMppt) Connect(address string) (err error) {
	c.handler = modbus.NewRTUClientHandler(address)
	c.handler.BaudRate = 9600
	c.handler.DataBits = 8
	c.handler.Parity = "N"
	c.handler.StopBits = 1
	c.handler.SlaveId = 1
	c.handler.Timeout = 10 * time.Second

	err = c.handler.Connect()
	if err != nil {
		return
	}

	c.mc = modbus.NewClient(c.handler)

	return
}

func (c *ProStarMppt) ReadHoldingRegisters(address uint16) (data []byte, err error) {
	time.Sleep(1 * time.Second)

	for i := 0; ; i++ {
		data, err = c.mc.ReadHoldingRegisters(address, 1)
		if err != nil {
			if i == 3 {
				err = fmt.Errorf("Error reading '%v' (%v)", address, err)
				return
			}
		} else {
			return
		}
	}

	return
}

func (c *ProStarMppt) ReadFloat16(address uint16) (value float32, err error) {
	data, err := c.ReadHoldingRegisters(address)
	if err != nil {
		return
	}

	value = math.Float32frombits(float16toUint32(binary.BigEndian.Uint16(data)))
	return
}

func (c *ProStarMppt) ReadFloat32(address uint16) (value float32, err error) {
	data, err := c.ReadHoldingRegisters(address)
	if err != nil {
		return
	}

	value = float32(binary.BigEndian.Uint16(data)) * 0.1
	return
}

func (c *ProStarMppt) ReadUint16(address uint16) (value uint16, err error) {
	data, err := c.ReadHoldingRegisters(address)
	if err != nil {
		return
	}

	value = binary.BigEndian.Uint16(data)
	return
}

func (c *ProStarMppt) Close() {
	if c.handler != nil {
		c.handler.Close()
	}
}

func (c *ProStarMppt) ReadChargeState() (value string, err error) {
	data, err := c.ReadHoldingRegisters(0x21)
	if err != nil {
		return
	}

	switch binary.BigEndian.Uint16(data) {
	case 0:
		value = "Start"
	case 1:
		value = "NightCheck"
	case 2:
		value = "Disconnect"
	case 3:
		value = "Night"
	case 4:
		value = "Fault"
	case 5:
		value = "MPPT"
	case 6:
		value = "Absorption"
	case 7:
		value = "Float"
	case 8:
		value = "Equalize"
	case 9:
		value = "Slave"
	case 10:
		value = "Fixed"
	default:
		value = "Unknown"
	}

	return
}

func (c *ProStarMppt) ReadLoadState() (value string, err error) {
	data, err := c.ReadHoldingRegisters(0x2E)
	if err != nil {
		return
	}

	switch binary.BigEndian.Uint16(data) {
	case 0:
		value = "Start"
	case 1:
		value = "LoadOn"
	case 2:
		value = "LowVoltageWarning"
	case 3:
		value = "LowVoltage"
	case 4:
		value = "Fault"
	case 5:
		value = "Disconnect"
	case 6:
		value = "LoadOff"
	case 7:
		value = "Override"
	default:
		value = "Unknown"
	}

	return
}

func (c *ProStarMppt) ReadLedState() (value string, err error) {
	data, err := c.ReadHoldingRegisters(0x3B)
	if err != nil {
		return
	}

	switch binary.BigEndian.Uint16(data) {
	case 0:
		value = "LedStart"
	case 1:
		value = "LedStart2"
	case 2:
		value = "LedBranch"
	case 3:
		value = "Equalize"
	case 4:
		value = "Float"
	case 5:
		value = "Absorption"
	case 6:
		value = "Green"
	case 7:
		value = "GreenYellow"
	case 8:
		value = "Yellow"
	case 9:
		value = "YellowRed"
	case 10:
		value = "BlinkRed"
	case 11:
		value = "Red"
	case 12:
		value = "RygError1"
	case 13:
		value = "RygError2"
	case 14:
		value = "RgyError"
	case 15:
		value = "RyError"
	case 16:
		value = "RgError"
	case 17:
		value = "RyGyError"
	case 18:
		value = "GyrError"
	case 19:
		value = "GyrTimesTwo"
	case 20:
		value = "Off"
	case 21:
		value = "GyRTimesTwoGreenTimesTwo"
	case 22:
		value = "GyrTimesTwoRedTimesTwo"
	default:
		value = "Unknown"
	}

	return
}

func (controller *ProStarMppt) Refresh() (err error) {
	cd := &controller.data

	cd.ChargeCurrent, err = controller.ReadFloat16(0x10)
	if err != nil {
		return err
	}
	cd.ArrayCurrent, err = controller.ReadFloat16(0x11)
	if err != nil {
		return err
	}
	cd.BatteryTerminalVoltage, err = controller.ReadFloat16(0x12)
	if err != nil {
		return err
	}
	cd.ArrayVoltage, err = controller.ReadFloat16(0x13)
	if err != nil {
		return err
	}
	cd.LoadVoltage, err = controller.ReadFloat16(0x14)
	if err != nil {
		return err
	}
	cd.BatteryCurrentNet, err = controller.ReadFloat16(0x15)
	if err != nil {
		return err
	}
	cd.LoadCurrent, err = controller.ReadFloat16(0x16)
	if err != nil {
		return err
	}
	cd.BatterySenseVoltage, err = controller.ReadFloat16(0x17)
	if err != nil {
		return err
	}
	cd.BatteryVoltageSlowFilter, err = controller.ReadFloat16(0x18)
	if err != nil {
		return err
	}
	cd.BatteryCurrentNetSlowFIlter, err = controller.ReadFloat16(0x19)
	if err != nil {
		return err
	}

	cd.HeatsinkTemperature, err = controller.ReadFloat16(0x1A)
	if err != nil {
		return err
	}
	cd.BatteryTemperature, err = controller.ReadFloat16(0x1B)
	if err != nil {
		return err
	}
	cd.AmbientTemperature, err = controller.ReadFloat16(0x1C)
	if err != nil {
		return err
	}

	cd.ChargeState, err = controller.ReadChargeState()
	if err != nil {
		return err
	}
	cd.LoadState, err = controller.ReadLoadState()
	if err != nil {
		return err
	}
	cd.LedState, err = controller.ReadLedState()
	if err != nil {
		return err
	}

	cd.LoadCurrentCompensated, err = controller.ReadFloat16(0x30)
	if err != nil {
		return err
	}
	cd.LoadHvdVoltage, err = controller.ReadFloat16(0x31)
	if err != nil {
		return err
	}

	cd.PowerOut, err = controller.ReadFloat16(0x3C)
	if err != nil {
		return err
	}
	cd.ArrayTargetVoltage, err = controller.ReadFloat16(0x40)
	if err != nil {
		return err
	}
	cd.MaximumBatteryVoltage, err = controller.ReadFloat16(0x41)
	if err != nil {
		return err
	}
	cd.MinimumBatteryVoltage, err = controller.ReadFloat16(0x42)
	if err != nil {
		return err
	}

	cd.AmpHourCharge, err = controller.ReadFloat32(0x43)
	if err != nil {
		return err
	}

	cd.AmpHourLoad, err = controller.ReadFloat32(0x44)
	if err != nil {
		return err
	}

	cd.TimeInAbsorption, err = controller.ReadUint16(0x49)
	if err != nil {
		return err
	}

	cd.TimeInEqualization, err = controller.ReadUint16(0x4E)
	if err != nil {
		return err
	}

	cd.TimeInFloat, err = controller.ReadUint16(0x4F)
	if err != nil {
		return err
	}

	return nil
}

type Options struct {
	Device  string
	CsvFile string
	Echo    bool
	Syslog  string
	Driver  string
}

func ReadDevice(device string, options *Options) error {
	log.Printf("Opening %v", device)

	proStar := NewProStarMppt()
	err := proStar.Connect(device)
	if err != nil {
		return fmt.Errorf("Unable to open device: %v", err)
	}

	defer proStar.Close()

	worked := false
	tries := 5
	for tries > 0 {
		err := proStar.Refresh()
		if err != nil {
			log.Printf("Error getting data: %v", err)
		} else {
			worked = true
			break
		}

		tries--
	}

	if !worked {
		return fmt.Errorf("Unable to get data")
	}

	values := []interface{}{
		proStar.data.ChargeCurrent,
		proStar.data.ArrayCurrent,
		proStar.data.BatteryTerminalVoltage,
		proStar.data.ArrayVoltage,
		proStar.data.LoadVoltage,
		proStar.data.BatteryCurrentNet,
		proStar.data.LoadCurrent,
		proStar.data.BatterySenseVoltage,
		proStar.data.BatteryVoltageSlowFilter,
		proStar.data.BatteryCurrentNetSlowFIlter,

		proStar.data.HeatsinkTemperature,
		proStar.data.BatteryTemperature,
		proStar.data.AmbientTemperature,

		proStar.data.ChargeState,
		proStar.data.LoadState,
		proStar.data.LedState,

		proStar.data.LoadCurrentCompensated,
		proStar.data.LoadHvdVoltage,

		proStar.data.PowerOut,
		proStar.data.ArrayTargetVoltage,
		proStar.data.MaximumBatteryVoltage,
		proStar.data.MinimumBatteryVoltage,

		proStar.data.AmpHourCharge,
		proStar.data.AmpHourLoad,

		proStar.data.TimeInAbsorption,
		proStar.data.TimeInEqualization,
		proStar.data.TimeInFloat,
	}

	strs := make([]string, 0)
	strs = append(strs, fmt.Sprintf("READING,%v", time.Now()))
	for _, value := range values {
		strs = append(strs, fmt.Sprintf("%v", value))
	}
	line := strings.Join(strs, ",")

	if options.CsvFile != "" {
		log.Printf("Opening %v", options.CsvFile)

		file, err := os.OpenFile(options.CsvFile, os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0600)
		if err != nil {
			log.Fatalf("Unable to open CSV %v", err)
		}

		defer file.Close()

		w := bufio.NewWriter(file)

		defer w.Flush()

		fmt.Fprintf(w, line)
		fmt.Fprintf(w, "\n")
	}

	if options.Syslog != "" {
		log.Printf("%s", line)
	}

	if options.Echo {
		fmt.Printf("%s\n", line)
	}

	return nil
}

func FindDevicesByDriver(driver string) []string {
	devices := make([]string, 0)
	pattern := fmt.Sprintf("/sys/bus/usb-serial/drivers/%s/tty*", driver)
	m, err := filepath.Glob(pattern)
	if err != nil {
		return devices
	}

	for _, file := range m {
		devices = append(devices, fmt.Sprintf("/dev/%s", filepath.Base(file)))
	}

	log.Printf("Trying: %v", devices)

	return devices
}

func main() {
	options := Options{}

	flag.StringVar(&options.Device, "device", "", "usb device")
	flag.StringVar(&options.CsvFile, "csv", "", "csv file")
	flag.StringVar(&options.Syslog, "syslog", "", "enable syslog and name the ap")
	flag.StringVar(&options.Driver, "driver", "", "check devices by the usb serial driver (experimental)")
	flag.BoolVar(&options.Echo, "echo", false, "echo to te user")

	flag.Parse()

	if options.Driver == "" && options.Device == "" {
		flag.PrintDefaults()
		os.Exit(2)
	}

	if options.Syslog != "" {
		syslog, err := syslog.New(syslog.LOG_NOTICE, options.Syslog)
		if err == nil {
			log.SetOutput(syslog)
		}
	}

	if options.Driver != "" {
		for _, device := range FindDevicesByDriver(options.Driver) {
			if ReadDevice(device, &options) == nil {
				break
			}
		}
	} else {
		ReadDevice(options.Device, &options)
	}
}
