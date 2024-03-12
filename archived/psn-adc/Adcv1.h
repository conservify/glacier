/* AdcV1.h - Header file for the older Version 1 ADC board based on the Rabbit Core CPU */

#pragma pack(1)
	
typedef struct  {
	ULONG packetID, timeTick;
	short year;
	BYTE  month, day, lockSts, flags;
} DataHdrV1;
	
typedef struct  {
	char addDropMode, flags;
	WORD pulseWidth;
	LONG addDropTimer, timeLocked, adjustNumber;
	short timeDiff, timeOffset;
} TimeInfo;
	
typedef struct  {
	short year;
	char month, day, hour, min, sec, dayOfWeek;
} DayTime;	
	
/* Flags for flag field below */
#define CF_CHECK_HEART		0x0001
#define CF_TIME_ONLY		0x0002
#define CF_ADD_DROP_ONLY	0x0004
#define CF_12BIT_ONLY		0x0008
#define CF_GPS_GARMIN		0x0010
#define CF_NO_ONE_PPS		0x0020
#define CF_GPS_MOT_NMEA		0x0080

/* Time modes for timeMode below */
#define WWV_MODE 			0
#define WWVB_MODE 			1
#define GPS_MODE 			2
#define PORT_MODE			3
#define LOCAL_MODE			4
	
typedef struct  {
	WORD flags;
	TimeInfo timeInfo;
	DayTime dayTime;
	short sps, dummy;
	LONG baud;
	BYTE numChannels, exitFlag, timeMode, dummy1, mode12Bit[MAX_ADC_CHANNELS];
} ConfigInfo;
	
typedef struct  {
	BYTE sysType, majVer, minVer, lockSts, numChannels;
	BYTE goodConfig, sdrExit, atod16;
	short sps;
	ULONG crcErrs, numProcessed, numRetran, numRetranErr, packetsRcvd;
	TimeInfo timeInfo;
} StatusInfoV1;
	
typedef struct  {
	ULONG packetID;
} RetranNum;

#pragma pack()
