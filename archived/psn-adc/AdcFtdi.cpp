/* AdcBoard.cpp */

#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "ftd2xx.h"
#include "WinTypes.h"
#include "PSNADBoard.h"

static const char *classPathName = "com/ftdi/D2xx";

#define LOG logWrite
void logWrite( const char *msg, ... );

ULONG packetID, goodPackets, badPackets, maxRxQue, curQue;

FT_HANDLE gblHandle = 0;
HANDLE hBoard = 0;

#define MAX_ADC_CHANNELS	8
#define MAX_SPS_RATE		500

/* Data buffers */
#define NEW_DATA_LENGTH		1024
BYTE newData[ NEW_DATA_LENGTH ], newSampleData[ MAX_ADC_CHANNELS * MAX_SPS_RATE * sizeof( LONG ) ];

int FtdiOpen( int devIndex, int baudRate )
{
	FT_HANDLE handle;
	
	if( gblHandle )  {
		LOG("FT Device already opened");
		FtdiClose();
	}
	
	if( FT_Open( devIndex, &handle ) != FT_OK)
		return 0;
	
	if( FT_SetBaudRate( handle, (DWORD)baudRate ) != FT_OK )
		return 0;
	if( FT_SetBitMode( handle, 0, 0 ) != FT_OK )
		return 0;
	if( FT_SetDataCharacteristics( handle, 8, 0, 0) != FT_OK )
		return 0;
	if( FT_SetFlowControl( handle, 0, 0x11, 0x13 ) != FT_OK )
		return 0;
	if( FT_SetLatencyTimer( handle, 16 ) != FT_OK )
		return 0;
	if( FT_SetTimeouts( handle, 5000, 1000 ) != FT_OK )
		return 0;
	if( FT_Purge( handle, 3 ) != FT_OK )
		return 0;
	
	packetID = goodPackets = badPackets = maxRxQue = curQue = 0;
	gblHandle = handle;
	
	return 1;
}	

int FtdiWrite( char *data, int len )
{
	DWORD ret;
	if( gblHandle )  {
		if( FT_Write( gblHandle, data, len, &ret ) != FT_OK ) 
			return -1;
	}
	return (int)ret;
}

int FtdiGetReadQueue()
{
	DWORD rxQSts, txQSts, eventSts;
	
	if( FT_GetStatus( gblHandle, &rxQSts, &txQSts, &eventSts ) != FT_OK )  {
		LOG( "FT_GetStatus Error" );
		return -1;
	}
	curQue = rxQSts;
	if( rxQSts > maxRxQue )
		maxRxQue = rxQSts;
	return (int)rxQSts;
}

int FtdiRead( char *to, int numToRead )
{
	DWORD numRead;
	if( FT_Read( gblHandle, (void*)to, (DWORD)numToRead, &numRead ) != FT_OK )  {
		LOG( "FT_Read Error" );
		return -1;
	}
	return (int)numRead;
}		

void FtdiPurge()
{
	if( gblHandle )
		FT_Purge( gblHandle, 3 );
}

void FtdiClose()
{
	if( gblHandle )
		FT_Close( gblHandle );	
	gblHandle = 0;
}

void FtdiSetClrDtr( int setFlag )
{
	if( gblHandle )  {
		if( setFlag )
			FT_SetDtr( gblHandle );
		else
			FT_ClrDtr( gblHandle );
	}
}

JNIEXPORT jint JNICALL Java_com_ftdi_D2xx_getStats( JNIEnv *env, jobject obj, jintArray values )
{
	DWORD type, dataLen;
	
	int sts = PSNGetBoardData( hBoard, &type, newData, newSampleData, &dataLen );
	if( sts == ADC_BOARD_ERROR )
		LOG("adc board error");
	else if( sts == ADC_GOOD_DATA )  {
		++goodPackets;
		if( type == ADC_ERROR || type == ADC_MSG )
			LOG("Msg: %s", (char *)newData );
	}
		
	jint *vjf = env->GetIntArrayElements( values, NULL );	
	vjf[0] = goodPackets;
	vjf[1] = badPackets;
	vjf[2] = curQue;
	vjf[3] = maxRxQue;
	env->ReleaseIntArrayElements( values, vjf, JNI_ABORT );
}

JNIEXPORT jint JNICALL Java_com_ftdi_D2xx_open( JNIEnv *env, jobject obj, jint devIndex, jint baudRate )
{
	DWORD version;
	FT_HANDLE handle;
	AdcBoardConfig2 config;
	int sts;
	
	memset( &config, 0, sizeof( config ) );
	config.commPort = 1;
	config.commSpeed = 38400;
	config.numberChannels = 4;
	config.sampleRate = 200;
	config.timeRefType = 1;		// Garmin
	
	/* Open the board. Function will return 0 if an error has occurred */
	hBoard = PSNOpenBoard();
	if( !hBoard )  {
		LOG("PSNOpenBoard Error");
		return 0;
	}

	/* Get DLL Version number. Version info is returned a 16 bit unsigned word */
	if( !PSNGetBoardInfo( hBoard, ADC_GET_DLL_VERSION, &version ) )  {
		LOG("PSNGetBoardInfo Error");
		PSNCloseBoard( hBoard );
		return 0;
	}
	LOG("PSNADBoard DLL Version %d.%1d", version / 10, version % 10 );
	
	PSNConfigBoard( hBoard, &config, NULL );
	
	PSNStartStopCollect( hBoard, TRUE );
	
	return 1;
}

JNIEXPORT jint JNICALL Java_com_ftdi_D2xx_create( JNIEnv * env, jclass clazz )
{
	DWORD numDevs = 0;
	
	system( "su -c chmod -R 777 dev/bus/usb*" );
	
	if( FT_CreateDeviceInfoList( &numDevs ) != FT_OK )
		return 0;
	gblHandle = 0;
	hBoard = 0;
	return (jint)numDevs;
}

JNIEXPORT void JNICALL Java_com_ftdi_D2xx_shutDown( JNIEnv *env, jobject obj )
{
	LOG("Shutdown Board");
	PSNStartStopCollect( hBoard, FALSE );
	PSNCloseBoard( hBoard );
}

JNIEXPORT void JNICALL Java_com_ftdi_D2xx_resetStats( JNIEnv *env, jobject obj )
{
	badPackets = maxRxQue = 0;
}

/**********************************************************************************
 * JNI function registration
 **********************************************************************************/
static JNINativeMethod sMethods[] = {
	/*Name							Signature						Function Pointer*/
	{"create",						"()I",							(void*)Java_com_ftdi_D2xx_create},
	{"open",			 			"(II)I",						(void*)Java_com_ftdi_D2xx_open},
	{"getStats",				    "([I)I",						(void*)Java_com_ftdi_D2xx_getStats},
	{"resetStats",				    "()V",							(void*)Java_com_ftdi_D2xx_resetStats},
	{"shutDown",				    "()V",							(void*)Java_com_ftdi_D2xx_shutDown},
};

JNIEXPORT jint JNICALL JNI_OnLoad( JavaVM *vm, void *reserved )
{
	JNIEnv *env;
	
	if( vm->GetEnv((void**)&env, JNI_VERSION_1_4 ) != JNI_OK )
		return -1;

    jclass clazz = env->FindClass(classPathName);
    if( clazz == NULL )  {
        LOG("Native registration unable to find class '%s'\n", classPathName );
        return -1;
    }
    if( env->RegisterNatives( clazz, sMethods, ( sizeof( sMethods ) / sizeof( sMethods[0] ) ) ) < 0 )  {
        LOG("RegisterNatives failed for '%s'\n", classPathName);
        return -1;
    }
	
	return JNI_VERSION_1_4;
}

void logWrite( const char *msg, ... )
{
    va_list ap;
    char buff[ 1024 ], tmStr[ 32 ];
    FILE *fp;
	time_t t;
	struct tm *nt;
	
	time( &t );
	nt = localtime( &t );
	sprintf( tmStr, "%02d/%02d %02d:%02d:%02d", nt->tm_mon+1, nt->tm_mday, 
			nt->tm_hour, nt->tm_min, nt->tm_sec);
	if( ! ( fp = fopen( "/sdcard/adcboard.log", "a" ) ) )
    	return;
    va_start(ap, msg);
    vsprintf( buff, msg, ap );
    va_end( ap );
    fprintf( fp, "%s %s\n", tmStr, buff );
    fclose( fp );
}
