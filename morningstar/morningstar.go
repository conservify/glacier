package main

import (
	"bufio"
	"encoding/binary"
	"flag"
	"fmt"
	"github.com/goburrow/modbus"
	"log"
	"math"
	"os"
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

func (controller *ProStarMppt) Connect(address string) (err error) {
	controller.handler = modbus.NewRTUClientHandler(address)
	controller.handler.BaudRate = 9600
	controller.handler.DataBits = 8
	controller.handler.Parity = "N"
	controller.handler.StopBits = 1
	controller.handler.SlaveId = 1
	controller.handler.Timeout = 10 * time.Second

	err = controller.handler.Connect()
	if err != nil {
		return
	}

	controller.mc = modbus.NewClient(controller.handler)

	return
}

func (controller *ProStarMppt) ReadFloat16(address uint16) (value float32, err error) {
	data, err := controller.mc.ReadHoldingRegisters(address, 1)
	if err != nil {
		return
	}

	value = math.Float32frombits(float16toUint32(binary.BigEndian.Uint16(data)))

	return
}

func (controller *ProStarMppt) Close() {
	if controller.handler != nil {
		controller.handler.Close()
	}
}

func (controller *ProStarMppt) ReadChargeState() (value string, err error) {
	data, err := controller.mc.ReadHoldingRegisters(0x21, 1)
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

func (controller *ProStarMppt) ReadLoadState() (value string, err error) {
	data, err := controller.mc.ReadHoldingRegisters(0x2E, 1)
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

func (controller *ProStarMppt) ReadLedState() (value string, err error) {
	data, err := controller.mc.ReadHoldingRegisters(0x3B, 1)
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
	var chargeData *ProStarMpptData

	chargeData.ChargeCurrent, err = controller.ReadFloat16(0x10)
	if err != nil {
		return
	}
	chargeData.ArrayCurrent, err = controller.ReadFloat16(0x11)
	if err != nil {
		return
	}
	chargeData.BatteryTerminalVoltage, err = controller.ReadFloat16(0x12)
	if err != nil {
		return
	}
	chargeData.ArrayVoltage, err = controller.ReadFloat16(0x13)
	if err != nil {
		return
	}
	chargeData.LoadVoltage, err = controller.ReadFloat16(0x14)
	if err != nil {
		return
	}
	chargeData.BatteryCurrentNet, err = controller.ReadFloat16(0x15)
	if err != nil {
		return
	}
	chargeData.LoadCurrent, err = controller.ReadFloat16(0x16)
	if err != nil {
		return
	}
	chargeData.BatterySenseVoltage, err = controller.ReadFloat16(0x17)
	if err != nil {
		return
	}
	chargeData.BatteryVoltageSlowFilter, err = controller.ReadFloat16(0x18)
	if err != nil {
		return
	}
	chargeData.BatteryCurrentNetSlowFIlter, err = controller.ReadFloat16(0x19)
	if err != nil {
		return
	}

	chargeData.HeatsinkTemperature, err = controller.ReadFloat16(0x1A)
	if err != nil {
		return
	}
	chargeData.BatteryTemperature, err = controller.ReadFloat16(0x1B)
	if err != nil {
		return
	}
	chargeData.AmbientTemperature, err = controller.ReadFloat16(0x1C)
	if err != nil {
		return
	}

	chargeData.ChargeState, err = controller.ReadChargeState()
	if err != nil {
		return
	}
	chargeData.LoadState, err = controller.ReadLoadState()
	if err != nil {
		return
	}
	chargeData.LedState, err = controller.ReadLedState()
	if err != nil {
		return
	}

	chargeData.LoadCurrentCompensated, err = controller.ReadFloat16(0x30)
	if err != nil {
		return
	}
	chargeData.LoadHvdVoltage, err = controller.ReadFloat16(0x31)
	if err != nil {
		return
	}

	chargeData.PowerOut, err = controller.ReadFloat16(0x3C)
	if err != nil {
		return
	}
	chargeData.ArrayTargetVoltage, err = controller.ReadFloat16(0x40)
	if err != nil {
		return
	}
	chargeData.MaximumBatteryVoltage, err = controller.ReadFloat16(0x41)
	if err != nil {
		return
	}
	chargeData.MinimumBatteryVoltage, err = controller.ReadFloat16(0x42)
	if err != nil {
		return
	}

	var data []byte

	data, err = controller.mc.ReadHoldingRegisters(0x43, 1)
	if err != nil {
		return
	}
	chargeData.AmpHourCharge = float32(binary.BigEndian.Uint16(data)) * 0.1

	data, err = controller.mc.ReadHoldingRegisters(0x44, 1)
	if err != nil {
		return
	}
	chargeData.AmpHourLoad = float32(binary.BigEndian.Uint16(data)) * 0.1

	data, err = controller.mc.ReadHoldingRegisters(0x49, 1)
	if err != nil {
		return
	}
	chargeData.TimeInAbsorption = binary.BigEndian.Uint16(data)

	data, err = controller.mc.ReadHoldingRegisters(0x4E, 1)
	if err != nil {
		return
	}
	chargeData.TimeInEqualization = binary.BigEndian.Uint16(data)

	data, err = controller.mc.ReadHoldingRegisters(0x4F, 1)
	if err != nil {
		return
	}
	chargeData.TimeInFloat = binary.BigEndian.Uint16(data)

	return
}

func FindDevice() (device *string) {
	return
}

func main() {
	device := flag.String("device", "/dev/ttyUSB0", "usb device")
	search := flag.Bool("search", false, "search all usb devices")
	csvFile := flag.String("csv", "morningstar.csv", "csv file")
	echo := flag.Bool("echo", false, "echo to te user")

	flag.Parse()

	if *search {
		device = FindDevice()
	}

	proStar := NewProStarMppt()
	err := proStar.Connect(*device)
	if err != nil {
		log.Fatal("Unable to open device", err)
	}

	defer proStar.Close()

	worked := false
	tries := 5
	for tries > 0 {
		err := proStar.Refresh()
		if err != nil {
			log.Printf("Error getting data", err)
		} else {
			worked = true
			break
		}

		tries--
	}

	if !worked {
		log.Fatal("Unable to get data")
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

	w := bufio.NewWriter(os.Stdout)

	if !*echo {
		log.Printf("Opening %v", *csvFile)
		file, err := os.OpenFile(*csvFile, os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0600)
		if err != nil {
			log.Fatal(err)
		}

		w = bufio.NewWriter(file)

		defer file.Close()
	}

	fmt.Fprintf(w, "%v", time.Now())

	for _, value := range values {
		fmt.Fprintf(w, ",%v", value)
	}

	fmt.Fprintf(w, "\n")

	w.Flush()
}
