package main

import (
	"encoding/binary"
	"io"
	"os"
	"time"
)

type TagHeader struct {
	Sync           byte
	ByteOrder      byte
	Version        byte
	InstrumentType byte
	Type           uint32
	Length         uint16
	DataLength     uint16
	Id             uint16
	Crc            uint16
}

type MiscRoParms struct {
	A2dBits        byte
	SampleBytes    byte
	RestartSource  byte
	BytePad        [3]byte
	InstalledChan  uint16
	MaxChannels    uint16
	SysBlkVersion  uint16
	BootBlkVersion uint16
	AppBlkVersion  uint16
	DspBlkVersion  uint16
	BatteryVoltage int16
	Crc            uint16
	Flags          uint16
	Temperature    int16
	WordPad        [3]int16
	DwordPad       [4]int32
}

type SingleChannelRoParms struct {
	MaxPeak       int32
	MaxPeakOffset uint32
	MinPeak       int32
	MinPeakOffset uint32
	Mean          int32
	AqOffset      int32
	DwordPad      [3]int32
}

type ChannelRoParms struct {
	Channels [12]SingleChannelRoParms
}

type TimingRoParms struct {
	ClockSource     byte
	GpsStatus       byte
	GpsHealth       byte
	BytePad2        [5]byte // Rename
	GpsLockFailures uint16
	GpsUpdateRtc    uint16

	AcqDelay     int16
	GpsLatitude  int16
	GpsLongitude int16
	GpsAltitude  int16
	DacCount     uint16
	WordPad2     int16
	GpsLastDrift [2]int16

	GpsLastTurnOnTime [2]uint32
	GpsLastUpdateTime [2]uint32
	GpsLastLockTime   [2]uint32
	DwordPad2         [4]int32
}

type StreamRoParms struct {
	StartTime       uint32
	TriggerTime     uint32
	Duration        uint32
	Errors          uint16
	Flags           uint16
	StartTimeMsec   uint16
	TriggerTimeMsec uint16
	NumberScans     uint32
	TriggerBitMap   uint32
	Pad             [1]uint32
}

type RoParms struct {
	Id             [3]byte
	InstrumentCode byte
	HeaderVersion  uint16
	HeaderBytes    uint16

	Misc     MiscRoParms
	Timing   TimingRoParms
	Channels ChannelRoParms
	Stream   StreamRoParms
}

type MiscRwParms struct {
	SerialNumber      uint16
	NumberChannels    uint16
	StationId         [5]byte
	Comment           [33]byte
	Elevation         int16
	Latitude          float32
	Longitude         float32
	UserCodes         [4]int16
	CrLfCode          byte
	MinBatteryVoltage byte
	CrLfDecimation    byte
	CrLfIrigType      byte
	CrLfBitMap        uint32
	ChannelBitMap     uint32
	CrLfProtocol      byte
	SiteId            [17]byte
	ExternalTrigger   byte
	NetworkFlag       byte
}

type TimingRwParms struct {
	GpsTurnOnInterval byte
	GpsMaxTurnOnTime  byte
	BytePad           [6]byte
	LocalOffset       int16
	WordPad           [3]int16
	DwordPad          [4]int32
}

type SingleChannelRwParms struct {
	Id                    [5]byte
	Channel               byte
	SensorSerialNumberExt uint16
	North                 int16
	East                  int16
	Up                    int16
	Altitude              int16
	Azimuth               int16
	SesnorType            uint16
	SensorSerialNumber    uint16
	Gain                  uint16
	TriggerType           byte
	IirTrigFilter         byte
	StaSecondsTten        byte
	LtaSeconds            byte
	StaLtaRatio           uint16
	StaLtaPercent         byte
	BytePad               [1]byte
	FullScale             float32
	Sensitivity           float32
	Damping               float32
	NaturalFrequency      float32
	TriggerThreshold      float32
	DetriggerThreashold   float32
	AlarmTriggerThreshold float32
	CalCoil               float32
	Range                 byte
	SensorGain            byte
	BytePad2              [10]byte
}

type ChannelRwParms struct {
	Channels [12]SingleChannelRwParms
}

type StreamRwParms struct {
	FilterFlag       byte
	PrimaryStorage   byte
	SecondaryStorage byte
	BytePad          [5]byte
	EventNumber      uint16
	Sps              uint16
	Apw              uint16
	PreEvent         uint16
	PostEvent        uint16
	MinRunTime       uint16
	VotesToTrigger   int16
	VotesToDetrigger int16
	BytePadA         byte
	FilterType       byte
	DataFormat       byte
	Reserved         byte
	Timeout          int16
	TxBlkSize        uint16
	BufferSize       uint16
	SampleRate       uint16
	TxChanMap        int32
	DwordPad         [2]int32
}

type VoterInfo struct {
	Type   byte
	Number byte
	Weight int16
}

type ModemRwParms struct {
	InitCmd           [64]byte
	DialingPrefix     [16]byte
	DialingSuffix     [16]byte
	HangupCmd         [16]byte
	AutoAnswerOnCmd   [16]byte
	AutoAnswerOffCmd  [16]byte
	PhoneNumber0      [24]byte
	PhoneNumber1      [24]byte
	PhoneNumber2      [24]byte
	PhoneNumber3      [24]byte
	WaitForConnection byte
	PauseBetweenCalls byte
	MaxDialAttempts   byte
	CellShare         byte
	CellOnTime        byte
	CellWarmupTime    byte
	CellStartTime     [5]int16
	BytePad           [4]byte
	Flags             uint16
	CallOutMsg        [46]byte
}

type RwParms struct {
	Misc     MiscRwParms
	Timing   TimingRwParms
	Channels ChannelRwParms
	Stream   StreamRwParms
	Voter    [15]VoterInfo
	Modem    ModemRwParms
}

type FileHeader struct {
	Ro RoParms
	Rw RwParms
}

type FrameHeader struct {
	FrameType      byte
	InstrumentCode byte
	RecorderId     uint16
	FrameSize      uint16
	BlockTime      [4]byte // Alignment work around.
	ChannelBitMap  uint16
	StreamPar      uint16
	FrameStatuses  [2]byte
	Msec           uint16
	ChannelBitMap1 byte
	TimeCode       [13]byte
}

type KinemetricsFileStream struct {
	Data []float32
}

type KinemetricsFile struct {
	StartTime        time.Time
	TagHeader        TagHeader
	FileHeader       FileHeader
	NumberOfSamples  int
	SamplesPerSecond int
	Streams          []KinemetricsFileStream
}

func ParseKinemetricsFile(fp *os.File) (file *KinemetricsFile, err error) {
	file = new(KinemetricsFile)

	file.TagHeader = TagHeader{}
	err = binary.Read(fp, binary.BigEndian, &file.TagHeader)
	if err == io.EOF {
		return nil, err
	}

	file.FileHeader = FileHeader{}
	err = binary.Read(fp, binary.BigEndian, &file.FileHeader)
	if err == io.EOF {
		return nil, err
	}

	file.StartTime = time.Unix(int64(file.FileHeader.Ro.Stream.StartTime), 0)
	file.Streams = make([]KinemetricsFileStream, file.FileHeader.Ro.Misc.MaxChannels)
	file.NumberOfSamples = int(file.FileHeader.Ro.Stream.NumberScans)
	file.SamplesPerSecond = int(file.FileHeader.Rw.Stream.Sps)

	scan := 0

	for i := 0; i < int(file.FileHeader.Ro.Misc.MaxChannels); i++ {
		file.Streams[i].Data = make([]float32, file.FileHeader.Ro.Stream.NumberScans)
	}

	bytesPerSample := int(file.FileHeader.Ro.Misc.SampleBytes)
	if bytesPerSample != 3 {
		panic("Expected 3 bytes per sample.")
	}

	for {
		tag := TagHeader{}
		err := binary.Read(fp, binary.BigEndian, &tag)
		if err == io.EOF {
			break
		}

		frame := FrameHeader{}
		err = binary.Read(fp, binary.BigEndian, &frame)
		if err == io.EOF {
			return nil, err
		}

		data := make([]byte, tag.DataLength)
		err = binary.Read(fp, binary.BigEndian, &data)
		if err == io.EOF {
			return nil, err
		}

		numberOfChannels := int(file.FileHeader.Ro.Misc.MaxChannels)
		numberOfSamples := int(tag.DataLength) / int(bytesPerSample) / numberOfChannels

		var expanded [4]byte

		for sample := 0; sample < numberOfSamples; sample++ {
			row := sample * numberOfChannels
			for channel := 0; channel < numberOfChannels; channel++ {
				channelInfo := file.FileHeader.Rw.Channels.Channels[channel]

				offset := row + channel
				copy(expanded[0:], data[offset*bytesPerSample:(offset*bytesPerSample)+bytesPerSample])

				value := int32(binary.BigEndian.Uint32(expanded[:])) >> 8
				// 8338608 = 2**23 (24 - 3 bytes)
				calibrationVolts := 8388608.0 / channelInfo.FullScale
				calibrationMks := (calibrationVolts * channelInfo.Sensitivity) / 9.81 // Earth's gravity mean.
				calibrated := float32(value) / calibrationMks
				file.Streams[channel].Data[scan] = calibrated
			}

			scan++
		}
	}

	return
}
