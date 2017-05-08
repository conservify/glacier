/* AdcDemo.h */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#define BYTE					unsigned char
#define WORD					unsigned short
#define DWORD					unsigned int
#define UINT					unsigned int
#define ULONG					unsigned int
#define LONG					int
#define SLONG					LONG
#define BOOL					int
#define HANDLE					int
#define TRUE					1
#define FALSE					0

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

/* Current ADC Board types  */
#define BOARD_UNKNOWN			0	/* Unknown board type */
#define BOARD_V1				1	/* Version 1.x Rabbit CPU ADC */
#define BOARD_V2				2	/* Version 2.x New PICC CPU ADC */
#define BOARD_VM				3	/* VolksMeter Interface Board */
#define BOARD_V3				4	/* DspPIC based ADC Board */
#define BOARD_SDR24				5	/* New 24-Bit ADC Board */

/* Return values for the PSNGetBoardData() function */
#define ADC_BOARD_ERROR			0	/* An error has occured */
#define ADC_NO_DATA				1	/* No new data is available */
#define ADC_GOOD_DATA			2	/* New data has been placed in the Header and ADC Data buffers */

/* The follow are the various commands that can be sent to the ADC board using the
   PSNSendBoardCommand() function */
#define ADC_CMD_EXIT			0	/* Sets the ADC board in the waiting for config information mode */
#define ADC_CMD_SEND_STATUS		1	/* The DLL will send back a ADC_STATUS message */
#define ADC_CMD_RESET_GPS		2	/* Used to reset the GPS receiver connected to the ADC board */
#define ADC_CMD_FORCE_TIME_TEST	3	/* Used to force a time check between GPS time and the ADC time accumulator */
#define ADC_CMD_CLEAR_COUNTERS	4	/* Clears various counters in the DLL and the ADC board */
#define ADC_CMD_GPS_DATA_ON_OFF	5	/* Raw GPS data will be sent to the application using the ADC_GPS_DATA message */
#define ADC_CMD_GPS_ECHO_MODE	6	/* Places the ADC Board in the GPS Echo mode */
#define ADC_CMD_RESET_BOARD		7	/* Toggles the DTR line to reset the CPU on the ADC board */
#define ADC_CMD_GOTO_BOOTLOADER	8	/* Place the ADC baord in the bootloader mode. V2 board only. */
#define ADC_CMD_SEND_TIME_INFO	9	/* DLL will send AddDrop Time adjustment data to the ADC board for storage in EEPROM */

/* The following are commands used by the PSNGetBoardInfo() function */
#define ADC_GET_BOARD_TYPE		0	/* Returns one the ADC Board types defined above */
#define ADC_GET_DLL_VERSION		1	/* Returns the DLL version */
#define ADC_GET_DLL_INFO		2	/* Returns DLL information */

/* The following are the message types returned in the Callback function or 
   PSNGetBoardData() in the poll mode  */
#define ADC_MSG					0	/* Information message for the DLL */
#define ADC_ERROR				1	/* Error message from the DLL */
#define ADC_AD_MSG				2	/* Information message from the ADC board */
#define ADC_AD_DATA				3	/* New ADC sample data */
#define ADC_GPS_DATA			4	/* New GPS data */
#define ADC_STATUS				5	/* New status information */
#define ADC_SAVE_TIME_INFO		6	/* New time information that should be saved to a file */

/* Defines the maximum number of channels the the ADC board can record */
#define	MAX_ADC_CHANNELS		8

/* Defines the maximum sample rate */
#define MAX_SPS_RATE			500

/* These are the various time reference modes. One of these should be placed in the 
   'timeRefType' field of the AdcBoardConfig structure */
#define TIME_REF_USEPC			0	/* Use PC Time */
#define TIME_REF_GARMIN			1	/* Garmin 16/18 */
#define TIME_REF_MOT_NMEA		2	/* Motorola NMEA */
#define TIME_REF_MOT_BIN		3	/* Motorola Binary */
#define TIME_REF_WWV			4	/* WWV Mode */
#define TIME_REF_WWVB			5	/* WWVB Mode ( V1 board only ) */		

/* Time reference lock status types */
#define TIME_REF_NOT_LOCKED		0	/* Currently not locked to the time reference */
#define TIME_REF_WAS_LOCKED		1	/* Currently not locked to the time reference, but was locked before */
#define TIME_REF_LOCKED			2	/* Currently locked to the time reference */

#define TIME_FILE_NAME			"time.dat"
#define ESC						0x1b

/* NOTE!!! All structures members are aligned on BYTE bounderies. */
#pragma pack(1)

/* The follow structure is returned when the ADC_GET_DLL_INFO is sent to the DLL */
typedef struct  {
	DWORD maxInQueue;			/* Maximum incoming data queue size  */
	DWORD maxUserQueue;			/* Maximum user queue size ( poll mode only ) */
	DWORD maxOutQueue;			/* Maximum output data to the ADC board queue size  */
	DWORD crcErrors;			/* Number of incoming CRC errors */
	DWORD userQueueFullCount;	/* User queue overflow count */
	DWORD xmitQueueFullCount;	/* Transmit queue overflow count */
	DWORD cpuLoopErrors;		/* Indicates a problem with the V2 CPU main loop time  */
} DLLInfo;

/* This Data Header information is returned with each new ADC data block */
typedef struct  {
	SYSTEMTIME packetTime;		/* Current time of the sample data */
	ULONG packetID;				/* Current packet ID */
	BYTE timeRefStatus;			/* Time reference status */
	BYTE flags;					/* Additional flags - Currently not used */
} DataHeader;
	
/* This structure is sent to the DLL to configure the DLL and ADC board. NOTE This structure is not used in DLL version 4.x */
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
	
/* This structure is sent to the DLL to configure the DLL and ADC board. NOTE This structure is for DLL version 4.0 and higher */
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

/* This structure is used to  */
typedef struct  {
	char addDropFlag; 		/* Indicates if the time should be added or dropped */
	BYTE flags;				/* Currently not used */
	WORD pulseWidth;		/* Average WWV pulse width  */
	LONG addDropCount; 		/* Add/Drop time timer value */
	LONG timeLocked; 		/* Current time lock status */
	LONG adjustNumber;		/* Number of time adjustments made to the time accumulator */
	short averageTimeDiff;	/* Average time difference between the reference and the time accumulator */
	short timeOffset;		/* Should be the same value passed in the AdcBoardConfig 'timeOffset' member */
} TimeInfo;
	
#define BOARD_TYPE_SDR_SERVER	0	/* PC/DOS SDR Server */
#define BOARD_TYPE_RABBIT_CPU	1	/* V1.x ADC board */
#define BOARD_TYPE_PICC_CPU		2	/* V2.x ADC boards */
	
typedef struct  {
	BYTE boardType;				/* One of the BOARD_TYPE_* above */
	BYTE majorVersion;			/* ADC firmware major version */
	BYTE minorVersion;			/* ADC firmware minor version */
	BYTE lockStatus;			/* Current Time reference status */
	BYTE numChannels;			/* Number of channels being recorded */
	WORD spsRate;				/* Current Sample Rate */
	ULONG crcErrors;			/* Number of incoming CRC errors to the ADC board */
	ULONG numProcessed;			/* Number of out going packets */
	ULONG numRetran;			/* Number of retransmissions packets (V1 board only ) */
	ULONG numRetranErr;			/* Number of retransmissions errors (V1 board only )*/
	ULONG packetsRcvd;			/* Number of packets received by the ADC board */
	TimeInfo timeInfo;			/* Current time information */
} StatusInfo;

/* Go back to normal structure member alignment */
#pragma pack()

/* NOTE: The DLL entry functions use the C style calling convention. The
   following must be used if your code is in a .cpp file. */

/* Imported DLL functions */
#ifdef WIN32
#define PSNADBOARD_API __declspec(dllimport)
#else
#define PSNADBOARD_API
#define __stdcall
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Opens the  ADC board and returns a HANDLE that is then used by the other functions */
PSNADBOARD_API HANDLE __stdcall PSNOpenBoard();

/* Closes the ADC Board */
PSNADBOARD_API BOOL __stdcall PSNCloseBoard( HANDLE );

/* Used to send configuration information and sets the option Callback function */
PSNADBOARD_API BOOL __stdcall PSNConfigBoard( HANDLE, AdcBoardConfig2 *, 
	void (*callback)( DWORD, void *, void *, DWORD ) );

/* Used to start and stop data collection; set start=TRUE to start 
   collection or FALSE to stop collection */
PSNADBOARD_API BOOL __stdcall PSNStartStopCollect( HANDLE, DWORD start );

/* Used in the poll data mode to retrive data from the DLL or ADC Board. 
   Do not use this function in the Callback mode. */
PSNADBOARD_API DWORD __stdcall PSNGetBoardData( HANDLE hBoard, DWORD *msgType, void *data, void *data1, DWORD *dataLen );

/* Used to send various commands and data to the board; command = ADC_CMD_* defined above */
PSNADBOARD_API BOOL __stdcall PSNSendBoardCommand( HANDLE hBoard, DWORD command, void *data );

/* Used to get various information from the DLL or ADC board; type = ADC_GET_* defined above */
PSNADBOARD_API BOOL __stdcall PSNGetBoardInfo( HANDLE hBoard, DWORD type, void *data );

#ifdef __cplusplus
}
#endif

/* Local program function prototypes */
void ProcessNewData( DWORD type, void *newData, void *newData1, DWORD dataLen );
void DisplayMsg( DWORD type, char *string );
void NewADData( DWORD type, DataHeader *hdr, void *adcData, DWORD dataLen );
void NewGpsData( BYTE *data, DWORD dataLen );
void DisplayStatus( StatusInfo *status );
void SaveTimeInfo( TimeInfo *info );
void ReadTimeInfo( TimeInfo *info );
void MakeConfig( AdcBoardConfig2 *cfg );
void DisplayDllInfo();
BOOL ProcessUserInput();

#ifndef WIN32
void _sleep( int );
BOOL kbhit();
char getch();
#endif

