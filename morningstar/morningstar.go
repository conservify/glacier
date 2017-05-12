package main

import (
	"flag"
	"log"
	"fmt"
	"github.com/goburrow/modbus"
	"time"
	"encoding/binary"
	"math"
	"os"
	"bufio"
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

type ProStarMppt struct {
	handler *modbus.RTUClientHandler 
	mc modbus.Client

	ChargeCurrent float32
	ArrayCurrent float32
	BatteryTerminalVoltage float32
	ArrayVoltage float32
	LoadVoltage float32
	BatteryCurrentNet float32
	LoadCurrent float32
	BatterySenseVoltage float32
	BatteryVoltageSlowFilter float32
	BatteryCurrentNetSlowFIlter float32

	HeatsinkTemperature float32
	BatteryTemperature float32
	AmbientTemperature float32
	
	ChargeState string
	LoadState string

	LoadCurrentCompensated float32
	LoadHvdVoltage float32

	PowerOut float32
	ArrayTargetVoltage float32
	MaximumBatteryVoltage float32
	MinimumBatteryVoltage float32
	AmpHourCharge float32
	AmpHourLoad float32

	TimeInAbsorption uint16
	TimeInEqualization uint16
	TimeInFloat uint16
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
	controller.handler.Timeout = 5 * time.Second

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
	case 0: value = "Start"
	case 1: value = "NightCheck"
	case 2: value = "Disconnect"
	case 3: value = "Night"
	case 4: value = "Fault"
	case 5: value = "MPPT"
	case 6: value = "Absorption"
	case 7: value = "Float"
	case 8: value = "Equalize"
	case 9: value = "Slave"
	case 10: value = "Fixed"
	default: value = "Unknown"
	}

	return 
}

func (controller *ProStarMppt) ReadLoadState() (value string, err error) {
	data, err := controller.mc.ReadHoldingRegisters(0x2E, 1)
	if err != nil {
		return
	}

	switch binary.BigEndian.Uint16(data) {
	case 0: value = "Start"
	case 1: value = "LoadOn"
	case 2: value = "LowVoltageWarning"
	case 3: value = "LowVoltage"
	case 4: value = "Fault"
	case 5: value = "Disconnect"
	case 6: value = "LoadOff"
	case 7: value = "Override"
	default: value = "Unknown"
	}

	return 
}

func (controller *ProStarMppt) Refresh() {
	controller.ChargeCurrent, _ = controller.ReadFloat16(0x10)
	controller.ArrayCurrent, _ = controller.ReadFloat16(0x11)
	controller.BatteryTerminalVoltage, _ = controller.ReadFloat16(0x12)
	controller.ArrayVoltage, _ = controller.ReadFloat16(0x13)
	controller.LoadVoltage, _ = controller.ReadFloat16(0x14)
	controller.BatteryCurrentNet, _ = controller.ReadFloat16(0x15)
	controller.LoadCurrent, _ = controller.ReadFloat16(0x16)
	controller.BatterySenseVoltage, _ = controller.ReadFloat16(0x17)
	controller.BatteryVoltageSlowFilter, _ = controller.ReadFloat16(0x18)
	controller.BatteryCurrentNetSlowFIlter, _ = controller.ReadFloat16(0x19)

	controller.HeatsinkTemperature, _ = controller.ReadFloat16(0x1A)
	controller.BatteryTemperature, _ = controller.ReadFloat16(0x1B)
	controller.AmbientTemperature, _ = controller.ReadFloat16(0x1C)
	
	controller.ChargeState, _ = controller.ReadChargeState()
	controller.LoadState, _ = controller.ReadLoadState()

	controller.LoadCurrentCompensated, _ = controller.ReadFloat16(0x30)
	controller.LoadHvdVoltage, _ = controller.ReadFloat16(0x31)

	controller.PowerOut, _ = controller.ReadFloat16(0x3C)
	controller.ArrayTargetVoltage, _ = controller.ReadFloat16(0x40)
	controller.MaximumBatteryVoltage, _ = controller.ReadFloat16(0x41)
	controller.MinimumBatteryVoltage, _ = controller.ReadFloat16(0x42)

	var data []byte

	data, _ = controller.mc.ReadHoldingRegisters(0x43, 1)
	controller.AmpHourCharge = float32(binary.BigEndian.Uint16(data)) * 0.1

	data, _ = controller.mc.ReadHoldingRegisters(0x44, 1)
	controller.AmpHourLoad = float32(binary.BigEndian.Uint16(data)) * 0.1

	data, _ = controller.mc.ReadHoldingRegisters(0x49, 1)
	controller.TimeInAbsorption = binary.BigEndian.Uint16(data)

	data, _ = controller.mc.ReadHoldingRegisters(0x4E, 1)
	controller.TimeInEqualization = binary.BigEndian.Uint16(data)

	data, _ = controller.mc.ReadHoldingRegisters(0x4F, 1)
	controller.TimeInFloat = binary.BigEndian.Uint16(data)
}

func FindDevice() (device *string) {
	return
}

func main() {
	device := flag.String("device", "/dev/ttyUSB0", "USB device")
	search := flag.Bool("search", false, "search all USB devices") 
	csvFile := flag.String("csv", "morningstar.csv", "CSV file")

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

	proStar.Refresh()

	file, err := os.OpenFile(*csvFile, os.O_CREATE | os.O_APPEND | os.O_WRONLY, 0600)
	if err != nil {
		log.Fatal(err)
	}

	w := bufio.NewWriter(file)

	defer file.Close()

	values := []interface{} {
		proStar.ChargeCurrent,
		proStar.ArrayCurrent,
		proStar.BatteryTerminalVoltage,
		proStar.ArrayVoltage,
		proStar.LoadVoltage,
		proStar.BatteryCurrentNet,
		proStar.LoadCurrent,
		proStar.BatterySenseVoltage,
		proStar.BatteryVoltageSlowFilter,
		proStar.BatteryCurrentNetSlowFIlter,

		proStar.HeatsinkTemperature,
		proStar.BatteryTemperature,
		proStar.AmbientTemperature,

		proStar.ChargeState,
		proStar.LoadState,

		proStar.LoadCurrentCompensated,
		proStar.LoadHvdVoltage,

		proStar.PowerOut,
		proStar.ArrayTargetVoltage,
		proStar.MaximumBatteryVoltage,
		proStar.MinimumBatteryVoltage,

		proStar.AmpHourCharge,
		proStar.AmpHourLoad,

		proStar.TimeInAbsorption,
		proStar.TimeInEqualization,
		proStar.TimeInFloat,
	}

	fmt.Fprintf(w, "%v", time.Now())

	for _, value := range values {
		fmt.Fprintf(w, ",%v", value)
	}

	fmt.Fprintf(w, "\n")

	w.Flush()
}
