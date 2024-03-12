/* AdcNew.h - Header File for the newer ADC boards. This includes Version 2, Version 3, 
              VolksMeter and SDR24 Based boards */

#define MIN_ADJ_TIME				300
#define PICC_ADC_FLAG				0x80		// This bit will be set if Picc based A/D board
#define PACKETS_PER_SEC				10
#define PACKETS_PER_SEC_SDR24_10	10
#define PACKETS_PER_SEC_SDR24_5		5
#define PACKETS_PER_SEC_VM			5

#define TIME_REFV2_GARMIN			0		// Garmin 16/18
#define TIME_REFV2_MOT_NMEA			1		// Motorola NMEA
#define TIME_REFV2_MOT_BIN			2		// Motorola Binary
#define TIME_REFV2_USEPC			3		// PC's clock as reference
#define TIME_REFV2_WWV				4		// WWV Mode
#define TIME_REFV2_WWVB				5		// WWVB Mode
#define TIME_REFV2_SKG				6		// Sure Elect. GPS board
#define TIME_REFV2_4800				7		// OEM GPS @ 4800 Baud
#define TIME_REFV2_9600				8		// OEM GPS @ 9600 Baud

#define TIME_SDR24_USEPC			0		// PC's clock as reference
#define TIME_SDR24_GARMIN			1		// Garmin 16/18
#define TIME_SDR24_SKG				2		// SKG/Sure Elec. Board
#define TIME_SDR24_4800				3		// OEM GPS @ 4800 Baud
#define TIME_SDR24_9600				4		// OEM GPS @ 9600 Baud

#define GPS_NOT_LOCKED				0
#define GPS_WAS_LOCKED				1
#define GPS_LOCKED					2

#define PC_LOCK_STATUS				1

#define WWV_NOT_LOCKED				0
#define WWV_WAS_LOCKED				1
#define WWV_LOCKED					2

#define	HIGH_TO_LOW_PPS_FLAG        0x01
#define	NO_LED_PPS_FLAG        		0x02
#define	SEND_NEW_HEADER        		0x04
	
#define MAX_LOC_AVG					5
#define MAX_AVG_DIFF				50

#define MAX_WWV_DIFF_TBL			6
#define NUM_FRAMES					2

// for sdr24 board
#define FG_PPS_HIGH_TO_LOW			0x0001
#define FG_DISABLE_LED				0x0002
#define FG_WATCHDOG_CHECK			0x0004
#define FG_WAAS_ON					0x0008
#define FG_2D_ONLY					0x0010

#define CFG_GF_UNIPOLAR				0x80
#define CFG_GF_MASK					0x7f

#define GF_GAIN_MASK				0x07
#define GF_UNIPOLAR_INPUT			0x08
#define GF_VREF_GT25				0x10

#define _isleap(year)  		((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))

#define PTIME_1MIN					600
#define PTIME_2MIN					1200
#define PTIME_5MIN					3000
#define PTIME_10MIN					6000
#define PTIME_15MIN					9000
#define PTIME_30MIN					18000
#define PTIME_1HOUR					36000
#define PTIME_2HOUR					72000
#define PTIME_3HOUR					108000
#define PTIME_4HOUR					144000
#define PTIME_6HOUR					216000
#define PTIME_8HOUR					(PTIME_1HOUR*8)
#define PTIME_12HOUR				(PTIME_1HOUR*12)
#define PTIME_24HOUR				(PTIME_1HOUR*24)
#define PTIME_48HOUR				(PTIME_24HOUR*2)

#pragma pack(1)

typedef struct  {						// A/D packet header
	ULONG packetID, timeTick, ppsTick, gpsTOD;
	BYTE  loopError, gpsLockSts, gpsSatNum, gpsMonth, gpsDay, gpsYear;
} DataHdrV2;

typedef struct  {						// A/D packet header
	ULONG packetID, timeTick, ppsTick, gpsTOD, notUsed;
	BYTE  loopError, gpsLockSts, gpsSatNum, gpsMonth, gpsDay, gpsYear;
} DataHdrSdr24;

typedef struct  {
	ULONG addDropTimer;
	BYTE addTimeFlag;
} AdjTimeInfo;
	
typedef struct  {
	BYTE modeNumConverters;
	BYTE goodConverterFlags;
	BYTE unused1;
	BYTE unused2;
} BoardInfoSdr24;

typedef struct  {
	ULONG notUsed;
	BYTE numConverters;
} BoardInfoVM;
	
typedef struct  {
	ULONG timeTick;
	BYTE reset;
} TimeInfoV2;
	
typedef struct  {
	LONG adjustTime;
	BYTE reset;
} TimeDiffInfo;

typedef struct  {
	BYTE boardType, majorVersion, minorVersion, numChannels; 
	short sps;
} StatusInfoV2;

/* Configuration information sent to the SDR24 board */
typedef struct  {	// main system config info packet
	WORD flags;
	WORD sps;
	ULONG timeTick, spare;
	BYTE numChannels;
	BYTE timeRefType;
} ConfigInfoV2;

/* Configuration information sent to the SDR24 ADC board */
typedef struct  {	// main system config info packet
	WORD flags;
	WORD sps;
	WORD vcoOnTime;
	WORD vcoOffTime;
	ULONG timeTick;
	BYTE numChannels;
	BYTE timeRefType;
	BYTE gainFlags[ MAX_SDR24_CHANNELS ];
} ConfigInfoSdr24;

/* Configuration information sent to the VolksMeter interface board */
typedef struct  {	// main system config info packet
	WORD flags;
	WORD sps;
	ULONG timeTick;
	BYTE numChannels;
	BYTE timeRefType;
	BYTE dacA;
	BYTE dacB;
} ConfigInfoVM;

typedef struct  {	// WWV time information
	LONG data;
	WORD msCount;
} WWVLocInfo;

typedef struct  {
	WORD onTime;
	WORD offTime;
} VcoInfo;

#pragma pack()

typedef struct  {
	LONG startTime, diff;
	time_t tod;
} FrameInfo;

class CAdcBoard;
class CAdcSdr24;
class CAdcVM;
class CAdcV2;

class CGpsRefVco
{
public:
	CAdcBoard *m_pAdcBoard;
	CAdcSdr24 *m_pOwner;

	CGpsRefVco();
	
	void NewPacket( DataHdrV2 *dataHdr );
	void Init();
	BOOL NewTimeSec();
	void ResetRef();
	void SetUnlockCounter();
	void CheckLockSts();
	void CheckUnlockSts();
	void MakeTimeInfo( TimeInfo *info );
	void MakeStatusStr( char *str );
	void TimeStrHMS( char *str, DWORD time );
	void TimeStrMin( char *str, DWORD time );
	void AdjustVco();
	void SetVcoOnOffPercent( int percent ) { m_vcoOnOffPercent = percent; }		
	
	BOOL m_debug;
	
	BOOL m_goodGpsData; 
	BOOL m_goodPPS;
	BOOL m_noDataFlag; 
	BOOL m_highNotLockedFlag;
	
	int m_timeState;
	int m_currPacketTime;
	int m_skipPackets;
	int m_notLockedTimer;
	int m_lockOffCount;
	int m_currOnCount;
	int m_gpsLockStatus;
	int m_currLockSts;
	int m_wasLocked;
	int m_currPPSDiff;
	int m_currPPSDiffAbs;
	int m_logStrCount;	
	int m_noGpsDataCount;
	int m_noGpsPPSCount;
	int m_currSecDiff;
	int m_waitSecTime;
	int m_badTimeCount;	
	int m_resetTimeFlag;
	int m_resetTimeStart;
	int m_waitTimeCount;
	int m_lockStartTime;
	
	int m_vcoOnOffPercent;
	int m_vcoSetTime;
	int m_vcoLastSetTime;
	int m_vcoDirection;
	int m_vcoHighError;
	int m_vco0to1Percent;
	
	ULONG m_lastGpsTime;
	ULONG m_lastPPSTime;
		
	DataHdrV2 m_currGpsHdr;
};

class CGpsRef
{
public:
	CAdcBoard *owner;
	CAdcV2 *ownerV2;
	CAdcVM *ownerVM;
	CAdcSdr24 *ownerSdr24;

	CGpsRef();
	
	void NewPacket( DataHdrV2 *dataHdr );
	void NewTimeAdj();
	void SendPacket( char chr );
	void CheckLockSts();
	void CheckUnlockSts();
	void RestartRef();
	void SetUnlockCounter();
	void Init();
	void SendAdjPacket( char chr );
	void ResetTime();
	void MakeStatusStr( char *str );
	void SaveTimeInfo( int mode, ULONG timer );
	void RecalcErrorRate();
	void MakeTimeInfo( TimeInfo * );	
	void ResetRef( BOOL clrAll );
	void ResetError( char );
	void CalcMsTimer();
	void TimeStr( char *str, DWORD time );
	void TimeStrHMS( char *str, DWORD time );
	void TimeStrMin( char *str, DWORD time );
	
	BOOL NewTimeSec();
	int CalcNewAdj();
	
	int boardType;
	
	BOOL debug;
	
	BOOL goodGpsData, goodPPS, invertAdj, gpsTest, noDataFlag;
	ULONG lastGpsTime, adjTimer, msAdjTimerValue, recalcAdjTimer;
	ULONG currPacketTime, addSubTimer, msAdjPackets, msAdjTimer;
	BOOL resetTimeFlag, adjModeAdd, wasLocked;
	LONG currSecDiff, currPPSDiff, currPPSDiffAbs, msAdjCount;
	int waitSecTime, noGpsDataCount, noGpsPPSCount, extraAdjTimer;
	int timeState, calcState, badTimeCount, calcTimer;
	int notZeroCounter, zeroCounter, logStrCount, skipPackets; 
	LONG resetTimeStart, lockStartTime, lastPPSTime;
	ULONG notLockedTimer;	
	BOOL highNotLockedFlag;
	int currLockSts, currOnCount, gpsLockStatus, lockOffCount, firstMsgFlag;
	
	DataHdrV2 currGpsHdr;
};

class CPcRef
{
public:
	CPcRef();
	
	CAdcBoard *owner;
	CAdcV2 *ownerV2;
	CAdcVM *ownerVM;
	CAdcSdr24 *ownerSdr24;

	void NewPacket();
	void NewTimeInfo( LONG diff );
	void SendPacket( char chr );
	int AddDiff( int newDiff );
	void AddAvg( int diff );
	void LogStatus();
	void MakeTimeInfo( TimeInfo * );	
	void Init();
	
	int boardType;
	
	int pcLockStatus;
	ULONG packetCounter, currPacketTime, adjTimer, addSubTimer, checkTime;
	int timeAvg[ MAX_LOC_AVG ];
	int avgIdx, avgNum, currDiff, absDiff;
	BOOL adjModeAdd, firstTimeInfo;
	time_t lockStartTime, lastLockTime;
};

class CWWVRef
{
public:
	CWWVRef();

	CAdcBoard *owner;
	CAdcV2 *ownerV2;
	
	int wwvLockStatus;
	int pulseAvg, pulseAvgCount, pulseWidth, calcWidth;
	int currPerLockCnt, lastPerLockCnt, periodLockTime;
	int debug, timeState, goodLock, badCount, timeOffset;
	int diffTimeTbl[ MAX_WWV_DIFF_TBL ], curDiff, diffCnt;
	ULONG currPacket, lastGoodPacket, lockStartPacket, startNotLockPacket;
	ULONG addSubTimer, adjTimer, msAdjCount, calcStartPacket, periodStartTimer;
	BOOL adjModeAdd, firstCalc, firstLock, firstTime, wasLocked;
	
	void NewPacket();
	void NewTimeInfo( int tickTime, int msCount );
	void CheckLockStatus();
	void ProcNotLock( int diff, int tickTime, int msCount );
	void ProcLock(int diff, int tickTime, int msCount );
	void ProcCalc( int diff, int tickTime, int msCount );
	void MakeTimeInfo( TimeInfo * );	
	void StrTimeSec(char *to, LONG tm );
	void ResetNotLock();
	void SendAdjPacket( char chr );
	void SaveTimeInfo( int mode, ULONG timer, WORD pulseWidth );
	void CalcPulseAvg( int msCount );
	void UnLock( int dsp );
	void Init();
	int AddAvg( int diff );
	int CalcAvg();
	int CalcTimeDiff(int *diff, LONG tm, int msCount );
};

class CWWVBRef
{
public:
	CWWVBRef();

	CAdcBoard *owner;
	CAdcV2 *ownerV2;
	
	WWVLocInfo ticks[60];
	int tickCount;
	
	FrameInfo frames[ NUM_FRAMES ];
	int frameCount;

	int finishTimer;
	
	ULONG lastTime, recalcAdjTimer;
	int lastMsCount;
		
	int currYear, currYearDay, currMon, currDay, currHour, currMin;
	LONG currDiff, absDiff, avgDiff, absAvgDiff;
	
	int lockStatus;
	int debug, goodLock, badCount, timeOffset;
	ULONG currPacket, lastGoodPacket, lockStartPacket, startNotLockPacket;
	ULONG addSubTimer, adjTimer, msAdjCount;
	BOOL adjModeAdd, firstCalc, firstLock, firstTime, wasLocked;
	
	void NewPacket();
	void NewTimeInfo( int tickTime, int msCount );
	void NewFrame();
	BOOL GetTime();
	BOOL GoodFrames();
	BOOL GoodTicks();
	
	BYTE GetNible( int start, int len );
	int PulseType( int msLen );
	void GetMonthDay( int *month, int *day, int yDay, int year );
	LONG CalcTimeDiff();
	void TimeStrHMS( char *str, DWORD time );
	void CheckAdjTimer();
	void TimeStr( char *str, DWORD time );
	void CheckLockStatus();
	void MakeTimeInfo( TimeInfo * );	
	void StrTimeSec(char *to, LONG tm );
	void ResetNotLock();
	void SendAdjPacket( char chr );
	void SaveTimeInfo( int mode, ULONG timer );
	void UnLock( int dsp );
	void Init();
};
