/* PSNADBoard  */

#ifdef WIN32_64
#define	WINVER					0x500
#define VC_EXTRALEAN					// Exclude rarely-used stuff from Windows headers
#include <afxdisp.h>        			// MFC Automation classes
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

#ifdef WIN32_64
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h> 
#endif

#define DLL_MAJOR_VERSION		5
#define DLL_MINOR_VERSION		6

#define BYTE					unsigned char
#define WORD					unsigned short
#define UINT					unsigned int
#define ULONG					unsigned int
#define SLONG					int
#define BOOL					int
#define TRUE					1
#define FALSE					0

#define E_OK					0		// No Error
#define E_OPEN_PORT_ERROR		1		// Error opening comm port
#define E_NO_CONFIG_INFO		2		// No configure information
#define E_THREAD_START_ERROR	3		// Receive thread startup error
#define E_NO_HANDLES			4		// Too many used handles
#define E_BAD_HANDLE			5		// Bad Handle ID
#define E_NO_ADC_CONTROL		6		// Internal Error
#define E_CONFIG_ERROR			7		// Configuration Error

#ifdef WIN32_64
#include <process.h>
#define PSNADBOARD_API 			__declspec(dllexport) 
#else
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#define PSNADBOARD_API
#define __stdcall
#define TIMEVAL					struct timeval
#define HANDLE					int
#define DWORD					unsigned int
#define SOCKET					int	
#define INVALID_SOCKET			-1	
#define SOCKET_ERROR			-1	
#define WSAEWOULDBLOCK			EWOULDBLOCK

typedef struct  {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME;
#endif

#ifdef ANDROID_LIB
#include <time64.h>				// needed for timegm64()
#endif

#define LONG					int

#define BOARD_UNKNOWN			0
#define BOARD_V1				1
#define BOARD_V2				2
#define BOARD_VM				3
#define BOARD_V3				4
#define BOARD_SDR24				5

#define ADC_BOARD_ERROR			0
#define ADC_NO_DATA				1
#define ADC_GOOD_DATA			2

#define NO_DATA_WAIT			1000		// about 20 seconds at 20ms loop time

#define ADC_CMD_EXIT			0
#define ADC_CMD_SEND_STATUS		1
#define ADC_CMD_RESET_GPS		2
#define ADC_CMD_FORCE_TIME_TEST	3
#define ADC_CMD_CLEAR_COUNTERS	4
#define ADC_CMD_GPS_DATA_ON_OFF	5
#define ADC_CMD_GPS_ECHO_MODE	6
#define ADC_CMD_RESET_BOARD		7
#define ADC_CMD_GOTO_BOOTLOADER	8
#define ADC_CMD_SEND_TIME_INFO	9
#define ADC_CMD_RESTART_BOARD	10
#define ADC_CMD_GPS_CONFIG		11

/* VM Sensor Commands */
#define ADC_CMD_SET_DAC_A		64
#define ADC_CMD_SET_DAC_B		65
#define ADC_CMD_OFF_CAL			66

/* SDR24 Commands */
#define ADC_CMD_SET_GAIN_REF	70
#define ADC_CMD_SET_VCO			71

/* Debug Commands */
#define ADC_CMD_DEBUG_REF		128
#define ADC_CMD_DEBUG_SAVE_ALL	129

/* Get Data from Host Commands */
#define ADC_GET_BOARD_TYPE		0
#define ADC_GET_DLL_VERSION		1
#define ADC_GET_DLL_INFO		2
#define ADC_GET_NUM_CHANNELS	3
#define ADC_GET_LAST_ERR_NUM	4

/* Send Data to Host Commands */
#define ADC_MSG					0
#define ADC_ERROR				1
#define ADC_AD_MSG				2
#define ADC_AD_DATA				3
#define ADC_GPS_DATA			4
#define ADC_STATUS				5
#define ADC_SAVE_TIME_INFO		6

#define MAX_ADC_HANDLES			8		// Maximum number of boards
#define	MAX_ADC_CHANNELS		8		// Maximum channels except SDR24 board
#define MAX_SDR24_CHANNELS		4		// Maximum channels on the SDR24 board

/* Time reference types */
#define TIME_REF_USEPC			0		// Use PC Time
#define TIME_REF_GARMIN			1		// Garmin 16/18
#define TIME_REF_MOT_NMEA		2		// Motorola NMEA
#define TIME_REF_MOT_BIN		3		// Motorola Binary
#define TIME_REF_WWV			4		// WWV Mode
#define TIME_REF_WWVB			5		// WWVB Mode
#define TIME_REF_SKG			6		// SKG GPS Receiver
#define TIME_REF_4800			7		// OEM GPS @ 4800 Baud
#define TIME_REF_9600			8		// OEM GPS @ 9600 Baud

#define SET_PC_TIME_NORMAL		1		// Do not set time if large error between PC and A/D board
#define SET_PC_TIME_LARGE_OK	2		// Always set the time even if there is a large error

#define ROLL_OVER_SEC			90000	// 25 Hours

#include "Adcv1.h"
#include "AdcNew.h"

#pragma pack(1)
	
typedef struct {
	BYTE saveType;				// 0 = send to A/D board, 1 = read from A/D Board;
	SYSTEMTIME tm;
	WORD len;
} SaveAllHdr;

typedef struct  {
	DWORD maxInQueue;
	DWORD maxUserQueue;
	DWORD maxOutQueue;
	DWORD crcErrors;
	DWORD userQueueFullCount;
	DWORD xmitQueueFullCount;
	DWORD cpuLoopErrors;
} DLLInfo;

typedef struct  {
	SYSTEMTIME packetTime;
	ULONG packetID;
	BYTE timeRefStatus;
	BYTE flags;
} DataHeader;
	
/* This structure is sent to the DLL to configure the DLL and ADC board. NOTE This structure is for DLL version 2.0 and higher */
typedef struct  {
	ULONG commPort;			/* Windows Comm port number - can be 1 to 255 */
	ULONG commSpeed;		/* Comm baud rate - can be 4800 (V1 only), 9600, 19200, 38400 or 57600 (V2 only) */
	ULONG numberChannels;	/* Number of channels to record - can be 1 to 8 ( 4 max V1 board at 200 SPS ) */
	ULONG sampleRate;		/* Sample rate - can be 10, 20, 25 (V1 board only), 50, 100 or 200 */
	ULONG timeRefType;		/* Time reference type - can be one of the TIME_REF_* types defined above */
	ULONG addDropTimer;		/* Time information used by the ADC board ( V1 board ) or DLL ( V2 boards ) */
	ULONG pulseWidth;		/* Time information used by the ADC board ( V1 board ) or DLL ( V2 boards ) */
	ULONG mode12BitFlags;	/* Used to place a ADC channel in the 12 Bit output mode */
	ULONG highToLowPPS;		/* If TRUE, the ADC board (V2 only ) will use a high to low top of the second pulse */
	ULONG noPPSLedStatus;	/* If TRUE, disables the LED on the ADC board from blinking at a 1 second rate */
	ULONG checkPCTime;		/* IF TRUE, the DLL will check the computer time using the ADC board type, but not set it */
	ULONG setPCTime;		/* If TRUE, the DLL will set the computers time using the ADC board time */
	ULONG tcpPort;			/* The port to use when connecting to the ADC board through a TCP/IP connection */
	LONG addDropMode;		/* Time information used by the ADC board ( V1 board ) or DLL ( V2 boards ) */
	LONG timeOffset;		/* Time offset in milliseconds. Should be set to ~30 ms for WWV ref. */
	char commPortTcpHost[256]; /* Linux/Unix Comm port string or TCP Host info if TCP/IP connection */
} AdcBoardConfig2;

/* Note: This is the old Configration structure that is no longer used by the 4.x DLL/Library */
typedef struct  {
	ULONG commPort;			/* Windows Comm port number - can be 1 to 255 */
	ULONG commSpeed;		/* Comm baud rate - can be 4800 (V1 only), 9600, 19200, 38400 or 57600 (V2 only) */
	ULONG numberChannels;	/* Number of channels to record - can be 1 to 8 ( 4 max V1 board at 200 SPS ) */
	ULONG sampleRate;		/* Sample rate - can be 10, 20, 25 (V1 board only), 50, 100 or 200 */
	ULONG timeRefType;		/* Time reference type - can be one of the TIME_REF_* types defined above */
	ULONG addDropTimer;		/* Time information used by the ADC board ( V1 board ) or DLL ( V2 boards ) */
	ULONG pulseWidth;		/* Time information used by the ADC board ( V1 board ) or DLL ( V2 boards ) */
	ULONG mode12BitFlags;	/* Used to place a ADC channel in the 12 Bit output mode */
	ULONG highToLowPPS;		/* If TRUE, the ADC board (V2 only ) will use a high to low top of the second pulse */
	ULONG noPPSLedStatus;	/* If TRUE, disables the LED on the ADC board from blinking at a 1 second rate */
	ULONG checkPCTime;		/* IF TRUE, the DLL will check the computer time using the ADC board type, but not set it */
	ULONG setPCTime;		/* If TRUE, the DLL will set the computers time using the ADC board time */
	SLONG addDropMode;		/* Time information used by the ADC board ( V1 board ) or DLL ( V2 boards ) */
	SLONG timeOffset;		/* Time offset in milliseconds. Should be set to ~30 ms for WWV ref. */
	char commPortStr[32];	/* Linux / Unix Comm port string */
} AdcBoardConfigOld;

typedef struct  {
	BYTE hdr[4];
	WORD len;
	BYTE type, flags;
} PreHdr;

#define BOARD_TYPE_UNKNOWN			0		
#define BOARD_TYPE_RABBIT_CPU		1		// V1.x board
#define BOARD_TYPE_PICC_CPU			2		// V2.x board
#define BOARD_TYPE_VM_SENSOR		3		// VolksMeter board
#define BOARD_TYPE_DSPIC_CPU		4		// V3.x board
#define BOARD_TYPE_DSPIC_SDR24		5		// V4.x 24 Bit board
	
/* This structure  is used to pass status information back to the host application */
typedef struct  {
	BYTE boardType;
	BYTE majorVersion;
	BYTE minorVersion;
	BYTE lockStatus;
	BYTE numChannels;
	WORD spsRate;
	ULONG crcErrors;
	ULONG numProcessed;
	ULONG numRetran;
	ULONG numRetranErr;
	ULONG packetsRcvd;
	TimeInfo timeInfo;
} StatusInfo;
	
/* This structure is used to pass additional configuration information from the host application to the 
   PSN-ADC24 and PSN-ACCEL boards. Currently referenceVolts member is not used. RefV is always 2.5.
   The adcGainFlags byte array controls the gain of each channel on the 24-Bit ADC board. The two upper 
   bits of each byte is used as flags to control the input mode and ref voltage mode of the ADC chip.
   Host Command: ADC_CMD_SET_GAIN_REF
*/
typedef struct {
	double referenceVolts;						
	BYTE  adcGainFlags[ MAX_SDR24_CHANNELS ];	
} AdcConfig;

/* This structure is used to pass additional GPS configuration information from the host application to the 
   the ADC boards. Currenly this information is used by the following boards: 
        16-Bit Version 3 both USB and Serial
   		24-Bit PSN-ADC24
		PSN-ACCEL 
   Host Command: ADC_CMD_GPS_CONFIG
*/
typedef struct {
	ULONG only2DMode;	// If TRUE tell the Garmin GPS receiver to only do 2D processing.
	ULONG enableWAAS;	// If TRUE tell the Garmin GPS receiver to also do WAAS processing if available.
	ULONG unused[14];	// Unused flags.
} GpsConfig;

#pragma pack()

class CAdcBoard;

typedef struct  {
	BYTE *data;
	DWORD dataLen;
} OutQueueInfo;
	
typedef struct  {
	BYTE *data;
	DWORD dataLen;
	BYTE type;
} UserQueueInfo;

class CQueue
{
public:
	CQueue();
	~CQueue();
	
	void Init( DWORD size );
	BOOL Add( void * );
	void *Remove();
	void *Peek();
	BOOL IsEmpty();
	DWORD GetFullCounter()  { return fullCounter; }		
	DWORD GetSize() { return count; };	
	
#ifndef WIN32_64
	pthread_mutex_t mutex;
#else	
	CRITICAL_SECTION lock;
#endif
	
	DWORD count, maxCount;
	DWORD fullCounter;
	DWORD size;
	void **array;
};

class COutQueue
{
public:
	COutQueue() { queue.Init( 4 ); };
	~COutQueue() { RemoveAll(); };
	
	DWORD GetFullCounter() { return queue.GetFullCounter(); }	
	BOOL Add( BYTE *data, DWORD len );
	BOOL PeekHead( BYTE *data, DWORD *len );
	BOOL Remove( BYTE *data, DWORD *len );
	BOOL AnyData();
	void RemoveAll();
	int GetSize()  { return queue.GetSize(); }

	CQueue queue;
};

class CUserQueue
{
public:
	CUserQueue() { queue.Init( 60 ); };
	~CUserQueue() { RemoveAll(); };
	
	DWORD GetFullCounter() { return queue.GetFullCounter(); }	
	BOOL Add( DWORD type, void *data, void *data1, DWORD dataLen );
	BOOL Remove( DWORD *type, void *data, void *data1, DWORD *dataLen );
	BOOL AnyData();
	void RemoveAll();

	CQueue queue;
};

#define MAX_RETRAN			20
#define RETRAN_NOT_SENT		0	
#define RETRAN_NEW			1	
#define RETRAN_SENT			2	
typedef struct  {
	int state;
	ULONG id;
} Retran;

class CQueue;

class CRetran
{
public:
	CRetran() { Init(); };
	void Init();
	void Reset();
	BOOL Start( DWORD startId, DWORD number );
	BOOL SavePacket( BYTE *packet );
	BOOL Check();
	BOOL Done();
	DWORD GetCurrentID();
	BYTE *GetQueuedPacket();
	void SetNextID();
	
	Retran list[ MAX_RETRAN ];
	int number, currIdx;
	time_t startTime;
	
	CQueue queue;
};

class CAdcV1
{
public:
	CAdcV1();
	
	CAdcBoard *owner;

	void ProcessNewPacket( BYTE *packet );
	void ProcessDataPacket( BYTE *packet );
	void ProcessRetranPacket( BYTE *packet );
	void ProcessStatusPacket( BYTE *packet );
	void ProcessAdcMessage( BYTE *packet );
	void ProcessSaveTimeInfo( BYTE *packet );
	void ProcessGpsData( BYTE *packet );
	void SendExitCommand();
	void SendTestCheckTime();
	void SendRetranPacket( ULONG packetId );
	void UnpackRawData( BYTE *packet, short *data );
	void SendCommand( char );
	void SendCommand( char, BYTE );
	void InitAdc( int, int );
	void SendCurrentTime();
	void CheckTimePacket( BYTE *packet );
	void MakeHeader( DataHeader *, DataHdrV1 * );
	void RetranQueueToUser();
	void SendNextRetranID();
	BOOL SendConfigPacket();
	BOOL GoodConfig( AdcBoardConfig2 *, char *errMsg );
	BOOL SendBoardCommand( DWORD cmd, void * );

	BOOL IsValidSpsRate( WORD sps, WORD numChannels, char *errMsg );
	
	BOOL retranMode;
	CRetran retran;
	
	BYTE hdrStr[4];
	ULONG currentPacketID;
	ULONG numberOfSamples, sampleLen, coiLen;
	WORD noPacketCount;

	short adcData[4096];
	short adcDataRetran[4096];
	
	BOOL setPCTime, checkPCTime;
	ULONG checkPCCount;
	
	ULONG start, number;
	LONG startTime;
};

class CAdcV2
{
public:
	CAdcV2();
	
	CAdcBoard *owner;
	
	void InitAdc( int sps, int numChans );
	void ProcessConfig( BYTE *packet );
	void ProcessNewPacket( BYTE *packet );
	void ProcessAdcMessage( BYTE *packet );
	void ProcessDataPacket( BYTE *packet );
	void ProcessTimeInfo( BYTE *packet );
	void ProcessGpsData( BYTE *packet );
	void ProcessStatusPacket( BYTE *packet );
	void ProcessAck();
	void SendCurrentTime( BOOL reset );
	void SendPacket( char chr );
	void UpdateMake12( DWORD flags );
	void MakeHeader( DataHdrV2 * );
	void MakeHeaderHole( ULONG *timeTick );
	void SendTimeDiffRequest( BOOL reset );	
	void NewCheckTimeInfo( LONG offset );
	void SetTimeDiff( LONG adjust, BOOL reset );
	void CheckUTCTime( DataHdrV2 * );
	void GetTickHMS( ULONG tick, int *hour, int *min, int *sec );
	void SendNewAdjInfo( ULONG addDrop, BYTE flag );
	void FillDataHole( DWORD diff );
	void AddTimeMs( ULONG *, SYSTEMTIME *, DWORD );	
	void SetRefBoardType( int type );

	BOOL SendConfigPacket();
	BOOL GoodConfig( AdcBoardConfig2 *, char *errMsg );
	BOOL SendBoardCommand( DWORD cmd, void * );
	BOOL IsGPSRef();
	
	BYTE GetLockStatus();
	BOOL IsValidSpsRate( WORD sps, WORD numChannels, char *errMsg );

	BOOL setPCTime, checkPCTime, checkUTCFlag;
	ULONG checkPCCount;
	
	int firstPacketCount, noPacketCount, sendStatusCount, blockConfigCount;
	
	BOOL make12Flag;
	BYTE make12Bit[ MAX_ADC_CHANNELS ];

	DataHdrV2 lastHdr;
	
	ULONG currUTCTick;
	SYSTEMTIME currUTCTime;
	DataHeader dataHeader;
	AdjTimeInfo adjTimeInfo;
		
	LONG sps5Accum[MAX_ADC_CHANNELS];
	int sps5Count, numChannels, timeRefType;
	
	time_t startRunTime;
	
	int packetNum, adDataLen, adSecDataLen, spsRate;
	
	BOOL noNewDataFlag, firstStatPacket;
	
	BYTE currSecBuffer[8192], *currSecPtr;
	DataHdrV2 currHdr;
	ULONG currPacketID, testPacketID, packetsReceived;	

	LONG checkPCAvg, checkPCAvgCount;
	
	BYTE currLoopErrors;
	double rmcTimeAvg;
	int rmcTimeCount;
		
	DWORD ackTimer, ackErrors;
	char ackTimerCommand;
		
	int rmcLocked;
	
	CGpsRef gpsRef;
	CPcRef pcRef;
	CWWVRef wwvRef;
	CWWVBRef wwvbRef;
	StatusInfoV2 m_statusInfo;
	GpsConfig gpsConfig;
};

class CAdcVM
{
public:
	CAdcVM();
	
	CAdcBoard *owner;
	
	void InitAdc( int sps, int numChans );
	void ProcessConfig( BYTE *packet );
	void ProcessNewPacket( BYTE *packet );
	void ProcessAdcMessage( BYTE *packet );
	void ProcessDataPacket( BYTE *packet );
	void ProcessTimeInfo( BYTE *packet );
	void ProcessGpsData( BYTE *packet );
	void ProcessStatusPacket( BYTE *packet );
	void ProcessAck();
	void SendCurrentTime( BOOL reset );
	void SendPacket( char chr );
	void MakeHeader( DataHdrV2 * );
	void MakeHeaderHole( ULONG *timeTick );
	void SendTimeDiffRequest( BOOL reset );	
	void NewCheckTimeInfo( LONG offset );
	void SetTimeDiff( LONG adjust, BOOL reset );
	void CheckUTCTime( DataHdrV2 * );
	void GetTickHMS( ULONG tick, int *hour, int *min, int *sec );
	void SendNewAdjInfo( ULONG addDrop, BYTE flag );
	void FillDataHole( DWORD diff );
	void AddTimeMs( ULONG *, SYSTEMTIME *, DWORD );	
	void SetRefBoardType();
	BOOL SendConfigPacket();
	BOOL GoodConfig( AdcBoardConfig2 *, char *errMsg );
	BOOL SendBoardCommand( DWORD cmd, void * );
	BOOL IsGPSRef();
	
	BYTE GetLockStatus();

	BOOL setPCTime, checkPCTime, checkUTCFlag;
	ULONG checkPCCount;
	
	int firstPacketCount, noPacketCount, sendStatusCount, blockConfigCount;
	
	DataHdrV2 lastHdr;
	
	ULONG currUTCTick;
	SYSTEMTIME currUTCTime;
	DataHeader dataHeader;
	BoardInfoVM boardInfo;
	AdjTimeInfo adjTimeInfo;
	
	LONG sps5Accum[MAX_ADC_CHANNELS];
	int sps5Count, numChannels, timeRefType, sampleLoop;
	
	time_t startRunTime;
	
	int packetNum, adDataLen, adSecDataLen, spsRate;
	
	BOOL noNewDataFlag;
	
	BYTE currSecBuffer[4096], *currSecPtr;
	DataHdrV2 currHdr;
	ULONG currPacketID, testPacketID, packetsReceived;	

	LONG checkPCAvg, checkPCAvgCount;
	
	BYTE currLoopErrors;
	double rmcTimeAvg;
	int rmcTimeCount;
	
	DWORD ackTimer, ackErrors;
	char ackTimerCommand;
		
	int rmcLocked;
	
	BYTE dacA, dacB;
	
	CGpsRef gpsRef;
	CPcRef pcRef;
};

class CAdcSdr24
{
public:
	CAdcSdr24();
	
	CAdcBoard *owner;
	
	void InitAdc( int sps, int numChans );
	void ProcessConfig( BYTE *packet );
	void ProcessNewPacket( BYTE *packet );
	void ProcessAdcMessage( BYTE *packet );
	void ProcessDataPacket( BYTE *packet );
	void ProcessTimeInfo( BYTE *packet );
	void ProcessGpsData( BYTE *packet );
	void ProcessStatusPacket( BYTE *packet );
	void ProcessAck();
	void SendCurrentTime( BOOL reset );
	void SendPacket( char chr );
	void MakeHeader( DataHdrSdr24 * );
	void MakeHeaderHole( ULONG *timeTick );
	void SendTimeDiffRequest( BOOL reset );	
	void NewCheckTimeInfo( LONG offset );
	void SetTimeDiff( LONG adjust, BOOL reset );
	void CheckUTCTime( DataHdrSdr24 * );
	void GetTickHMS( ULONG tick, int *hour, int *min, int *sec );
	void SendNewAdjInfo( ULONG addDrop, BYTE flag );
	void FillDataHole( DWORD diff );
	void AddTimeMs( ULONG *, SYSTEMTIME *, DWORD );	
	void SetRefBoardType();
	void ConvertTimeToMs( ULONG *tick );
	void MakeV2Hdr( DataHdrV2 *v2Hdr, DataHdrSdr24 *dataHdr );
	void SetVcoFreq( WORD onOffTime );
	void SaveTimeInfo( WORD percent );
	void MakeTimeStr( char *to, ULONG tod );
	void MakeTimeTickStr( char *to, ULONG tick );
	
	BOOL SendConfigPacket();
	BOOL GoodConfig( AdcBoardConfig2 *, char *errMsg );
	BOOL SendBoardCommand( DWORD cmd, void * );
	BOOL IsGPSRef();
	
	BYTE GetLockStatus();
	BOOL IsValidSpsRate( WORD sps, WORD numChannels, char *errMsg );

	BOOL setPCTime, checkPCTime, checkUTCFlag, needsResetFlag;
	ULONG checkPCCount;
	
	int firstPacketCount, noPacketCount, sendStatusCount, blockConfigCount;
	
	DataHdrSdr24 lastHdr;
	
	ULONG currUTCTick;
	SYSTEMTIME currUTCTime;
	DataHeader dataHeader;
	BoardInfoSdr24 boardInfo;
	AdjTimeInfo adjTimeInfo;
	
	int numChannels, timeRefType, sampleLoop;
	
	time_t startRunTime;
	
	int adDataLen, adSecDataLen, spsRate;
	
	BOOL noNewDataFlag, firstStatPacket;
	
	int currSecBuffer[1024], sampleCount, sampleSendCount;
	DataHdrSdr24 currHdr;
	ULONG currPacketID, testPacketID, packetsReceived;	

	LONG checkPCAvg, checkPCAvgCount;
	
	BYTE currLoopErrors;
	double rmcTimeAvg;
	int rmcTimeCount;
	
	DWORD ackTimer, ackErrors;
	char ackTimerCommand;
		
	double referenceVolts;
	BYTE adcGainFlags[ MAX_SDR24_CHANNELS ];
	
	CGpsRefVco gpsRefVco;
	CGpsRef gpsRef;
	CPcRef pcRef;
	GpsConfig gpsConfig;
};

class CAdcBoard
{
public:
	CAdcBoard();
	~CAdcBoard();
	
	FILE *saveAllFp;
	
	CAdcV1 adcV1;
	CAdcV2 adcV2;
	CAdcVM adcVM;
	CAdcSdr24 adcSdr24;
	
	void ClosePort( BOOL reset );
	void ReadXmitThread();
	void ProcessNewPacket( BYTE *packet );
	void SendMsgFmt( DWORD type, const char *pszFormat, ... );
	void SendCommData( BYTE *data, int len );
	void SetCallback( void ( *func )( DWORD, void *, void *, DWORD ) );
	void InitAdc();
	void ResetBoard();
	void SendCurrentTime();
	void CheckCurrentTime();
	void SetComputerTime( LONG );
	void CheckIDAndTime( BYTE * );
	void DebugSaveAll( BOOL onOff );
	void SaveAllData( BYTE type, BYTE *data, int len );
				
	BOOL CheckOutputQueue();
	BYTE CalcCRC( BYTE *cp, short cnt );
	BOOL OpenPort( BOOL reset );
	BOOL TryToConnect();
	BOOL SendBoardCommand( DWORD cmd, void * );
	BOOL GetBoardInfo( DWORD type, void *data );
	BOOL GetBoardData( DWORD *type, void *data, void *data1, DWORD *dataLen );
	BOOL GetCommData();
	BOOL NewConfig( AdcBoardConfig2 *config );
	BOOL SetPriv( BOOL fEnable );
				
	BOOL TcpIpConnect( char *hostStr, int port );
	void FlushComm();
	
	DWORD checkID;
	
	/* The following are used to process incoming packets */
	BYTE hdrStr[4], inBuff[4096], inPacket[4096], *curPtr, *currPkt;
	int inHdr, packLen, curCnt, hdrState, dataLen;

	void SendQueueData( DWORD type, BYTE *data, DWORD len );
	void SendQueueAdcData( DWORD type, DataHeader *hdr, void *data, DWORD len );
	
	void CheckSendTime();
	
	BYTE newPacket[8192];
	BOOL goodData, fastLoop;
	DWORD crcErrors, maxInQue;
	
	DWORD lastErrorNumber;
	
#ifdef WIN32_64 
	DWORD packetStartTime;
#else
	TIMEVAL packetStartTime;
#endif
	
	BYTE cbData[8192];
	DataHeader cbDataHeader;
	char cbGpsData[4096];
	char cbString[1024];
	
	HANDLE hPort, hReadThread;
	
#ifndef WIN32_64
	pthread_t readThread;
#endif
	
	BOOL bExitThread;
	BOOL bThreadRunning;
	BOOL blockNewData;
	BOOL configSent;
	BOOL sendTimeFlag, checkTimeFlag;
	BOOL goodConfig;
	AdcBoardConfig2 config;
	void (*Callback)( DWORD, void *, void *, DWORD );
	int	adcBoardType, lastBoardType;
	ULONG currentBaud;
	
	COutQueue outputQueue;
	CUserQueue userQueue;

	BOOL m_tcpipMode;
	int m_tcpipPort;
	char m_tcpipHost[ 256 ];
	SOCKET m_socket;
	int m_tcpipConnectSts;	// = 0 not connected, 1 = connecting, 2 = connected
	int m_tcpipConnectError;
	int m_tcpipTestCount;
	time_t m_threadEndTime;
	time_t m_lastSendTime;
	int m_checkTimeCount;
	int m_debug;
	BOOL m_isUsbDevice;
	BOOL m_noUsbDataError;
	BOOL m_flushComm;
};

class CPSNADBoard
{
public:
	
	CPSNADBoard();
	~CPSNADBoard();
	
	HANDLE OpenBoard();
	
	BOOL StartStopCollect( HANDLE hBoard, DWORD start );
	BOOL NewConfig( HANDLE hBoard, AdcBoardConfig2 *config );
	BOOL OpenPort( HANDLE hBoard );
	BOOL ClosePort( HANDLE hBoard );
	BOOL SendBoardCommand( HANDLE hBoard, DWORD cmd, void * );
	BOOL GetBoardInfo( HANDLE hBoard, DWORD type, void *data );
	BOOL GetBoardData( HANDLE hBoard, DWORD *type, void *data, void *, DWORD *);
	BOOL SetCallback( HANDLE hBoard, void ( *func )( DWORD, void *, void *, DWORD ) );
	BOOL CloseBoard( HANDLE hBoard );

	void CloseAllBoards();

	CAdcBoard *pAdc[MAX_ADC_HANDLES];
	DWORD lastErrorNumber;
};

#ifndef WIN32_64 
void GetSystemTime(SYSTEMTIME *systime);
#endif

time_t MakeLTime( int year, int mon, int day, int hour, int min, int sec );
void libLogWrite( char *pszFormat, ...);
void ms_sleep( int );

#ifdef ANDROID_LIB
int FtdiOpen( int dev, int baud );
int FtdiWrite( char *data, int len );
int FtdiGetReadQueue();
int FtdiRead( char *to, int numToRead );
void FtdiSetClrDtr( int setFlag );
void FtdiPurge();
void FtdiClose();
#endif

void logWrite( const char *, ... );

#ifdef __cplusplus
extern "C" {
#endif

PSNADBOARD_API HANDLE __stdcall PSNOpenBoard();
PSNADBOARD_API BOOL __stdcall PSNCloseBoard( HANDLE );
PSNADBOARD_API BOOL __stdcall PSNConfigBoard( HANDLE, AdcBoardConfig2 *, void (*callback)( DWORD, void *, void *, DWORD ) );
PSNADBOARD_API BOOL __stdcall PSNStartStopCollect( HANDLE, DWORD start );
PSNADBOARD_API DWORD __stdcall PSNGetBoardData( HANDLE hBoard, DWORD *type, void *data, void *data1, DWORD *dataLen );
PSNADBOARD_API BOOL __stdcall PSNSendBoardCommand( HANDLE hBoard, DWORD cmd, void *data );
PSNADBOARD_API BOOL __stdcall PSNGetBoardInfo( HANDLE hBoard, DWORD type, void *data );
#ifdef __cplusplus
}
#endif
