// PSNADBoard.cpp

#include "PSNADBoard.h"

CPSNADBoard *adcControl = NULL;

#ifdef WIN32_64
extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch( dwReason )  {
		case DLL_PROCESS_ATTACH:
			if( !adcControl )
				adcControl = new CPSNADBoard();
			if( adcControl )
				adcControl->lastErrorNumber = E_OK;
			break;
		case DLL_PROCESS_DETACH:
			if( adcControl )  {
				delete adcControl;
				adcControl = 0;
			}
			break;
    }
    return TRUE;
}

PSNADBOARD_API HANDLE __stdcall PSNOpenBoard()
{
	if( !adcControl )
		return FALSE;
	adcControl->lastErrorNumber = E_OK;
	return adcControl->OpenBoard();
}
#else
PSNADBOARD_API HANDLE __stdcall PSNOpenBoard()
{
	if( !adcControl )  {
		adcControl = new CPSNADBoard();
		if( !adcControl )
			return FALSE;
	}
	adcControl->lastErrorNumber = E_OK;
	return adcControl->OpenBoard();
}
#endif

PSNADBOARD_API BOOL __stdcall PSNStartStopCollect( HANDLE hBoard, DWORD start )
{
	if( !adcControl )
		return FALSE;
	adcControl->lastErrorNumber = E_OK;
	return adcControl->StartStopCollect( hBoard, start );
}

PSNADBOARD_API BOOL __stdcall PSNConfigBoard( HANDLE hBoard, AdcBoardConfig2 *config, 
	void (*Callback)( DWORD type, void *data, void *data1, DWORD len ) )
{
	if( !adcControl )
		return FALSE;
	adcControl->lastErrorNumber = E_OK;
	if( !adcControl->SetCallback( hBoard, Callback ) )
		return FALSE;
	return adcControl->NewConfig( hBoard, config );
}

PSNADBOARD_API BOOL __stdcall PSNCloseBoard( HANDLE hBoard )
{
	BOOL ret;
	
	if( !adcControl )
		return FALSE;
	adcControl->lastErrorNumber = E_OK;
	ret = adcControl->CloseBoard( hBoard );
#ifndef WIN32_64
	delete adcControl;
	adcControl = 0;
#endif
	return ret;
}

PSNADBOARD_API BOOL __stdcall PSNSendBoardCommand( HANDLE hBoard, DWORD command, void *data )
{
	if( !adcControl )
		return FALSE;
	adcControl->lastErrorNumber = E_OK;
	return adcControl->SendBoardCommand( hBoard, command, data );
}

PSNADBOARD_API BOOL __stdcall PSNGetBoardInfo( HANDLE hBoard, DWORD type, void *data )
{
	if( !adcControl )
		return FALSE;
	if( type != ADC_GET_LAST_ERR_NUM )
		adcControl->lastErrorNumber = E_OK;
	return adcControl->GetBoardInfo( hBoard, type, data );
}

PSNADBOARD_API DWORD __stdcall PSNGetBoardData( HANDLE hBoard, DWORD *type, void *data, void *data1, DWORD *dataLen )
{
	if( !adcControl )
		return ADC_BOARD_ERROR;
	adcControl->lastErrorNumber = E_OK;
	return adcControl->GetBoardData( hBoard, type, data, data1, dataLen );
}

CPSNADBoard::CPSNADBoard()
{ 
	memset( pAdc, 0, sizeof( pAdc ) );
}

HANDLE CPSNADBoard::OpenBoard()
{
	int ret = 0;
	for( int i = 0; i != MAX_ADC_HANDLES; i++ )  {
		if( !pAdc[ i ] )  {
			ret = i + 1;
			pAdc[ i ] = new CAdcBoard();
			break;
		}
	}
	if( !ret )
		lastErrorNumber = E_NO_HANDLES;
	return (HANDLE)ret;
}

BOOL CPSNADBoard::NewConfig( HANDLE hBoard, AdcBoardConfig2 *config )
{
	CAdcBoard *adc = NULL;
	int brd = (int)hBoard;
	
	if( brd < 1 || ( (brd-1) >= MAX_ADC_HANDLES ) )  {
		lastErrorNumber = E_BAD_HANDLE;
		return FALSE;
	}
	if( ! ( adc = pAdc[ brd-1 ] ) )  {
		lastErrorNumber = E_NO_ADC_CONTROL;
		return FALSE;
	}

	if( !strlen( config->commPortTcpHost) && ( config->commPort < 1 || config->commPort > 255 ) )  {
		lastErrorNumber = E_CONFIG_ERROR;
		return FALSE;
	}
	
	if( config->commSpeed != 4800 && config->commSpeed != 9600 && config->commSpeed != 19200 &&
			config->commSpeed != 38400 && config->commSpeed != 57600 )  {
		lastErrorNumber = E_CONFIG_ERROR;
		return FALSE;
	}

	if( config->numberChannels < 1 || config->numberChannels > 8 )  {
		lastErrorNumber = E_CONFIG_ERROR;
		return FALSE;
	}
		
	if( config->timeRefType > TIME_REF_9600  )  {
		lastErrorNumber = E_CONFIG_ERROR;
		return FALSE;
	}
	return  adc->NewConfig( config );
}

BOOL CPSNADBoard::StartStopCollect( HANDLE hBoard, DWORD start )
{
	CAdcBoard *adc = NULL;
	int brd = (int)hBoard;
	
	if( brd < 1 || ( (brd-1) >= MAX_ADC_HANDLES ) )  {
		lastErrorNumber = E_BAD_HANDLE;
		return FALSE;
	}
	if( ! ( adc = pAdc[ brd-1 ] ) )  {
		lastErrorNumber = E_NO_ADC_CONTROL;
		return FALSE;
	}
	if( start )  {
		if( !adc->goodConfig )
			return FALSE;
		return adc->OpenPort( 0 );
	}
	else  {
		adc->ClosePort( 0 );
		return TRUE;
	}
}

BOOL CPSNADBoard::CloseBoard( HANDLE hBoard )
{
	int brd = (int)hBoard;
	if( brd < 1 || ( (brd-1) >= MAX_ADC_HANDLES ) )  {
		lastErrorNumber = E_BAD_HANDLE;
		return FALSE;
	}
	CAdcBoard *adc = pAdc[ brd-1 ]; 	
	if( !adc )  {
		lastErrorNumber = E_NO_ADC_CONTROL;
		return FALSE;
	}
	adc->ClosePort( FALSE );
	delete adc;
	pAdc[ brd-1 ] = 0; 	
	return TRUE;
}

BOOL CPSNADBoard::SendBoardCommand( HANDLE hBoard, DWORD command, void *data )
{
	int brd = (int)hBoard;
	if( brd < 1 || ( (brd-1) >= MAX_ADC_HANDLES ) )  {
		lastErrorNumber = E_BAD_HANDLE;
		return FALSE;
	}
	CAdcBoard *adc = pAdc[ brd-1 ]; 	
	if( !adc )  {
		lastErrorNumber = E_NO_ADC_CONTROL;
		return FALSE;
	}
	return adc->SendBoardCommand( command, data );
}

BOOL CPSNADBoard::GetBoardInfo( HANDLE hBoard, DWORD type, void *data )
{
	DWORD *err = (DWORD *)data;
	
	if( type == ADC_GET_LAST_ERR_NUM && lastErrorNumber != E_OK )  {
		*err = lastErrorNumber;
		return TRUE;
	}
	
	int brd = (int)hBoard;
	if( brd < 1 || ( (brd-1) >= MAX_ADC_HANDLES ) )  {
		lastErrorNumber = E_BAD_HANDLE;
		return FALSE;
	}
	CAdcBoard *adc = pAdc[ brd-1 ]; 	
	if( !adc )  {
		lastErrorNumber = E_NO_ADC_CONTROL;
		return FALSE;
	}
	return adc->GetBoardInfo( type, data );
}

BOOL CPSNADBoard::GetBoardData( HANDLE hBoard, DWORD *type, void *data, void *data1, DWORD *dataLen )
{
	int brd = (int)hBoard;
	if( brd < 1 || ( (brd-1) >= MAX_ADC_HANDLES ) )  {
		lastErrorNumber = E_BAD_HANDLE;
		return FALSE;
	}
	CAdcBoard *adc = pAdc[ brd-1 ]; 	
	if( !adc )  {
		lastErrorNumber = E_NO_ADC_CONTROL;
		return FALSE;
	}
	return adc->GetBoardData( type, data, data1, dataLen );
}

void CPSNADBoard::CloseAllBoards()
{
	for( int i = 0; i != MAX_ADC_HANDLES; i++ )  {
		CAdcBoard *adc = pAdc[ i ]; 	
		if( adc )  {
			delete adc;
			pAdc[ i ] = 0;
		}
	}
}

BOOL CPSNADBoard::SetCallback( HANDLE hBoard, void (*func)( DWORD type, void *data, void *data1, DWORD dataLen ) )
{
	int brd = (int)hBoard;
	if( brd < 1 || (brd-1) >= MAX_ADC_HANDLES )  {
		lastErrorNumber = E_BAD_HANDLE;
		return FALSE;
	}
	CAdcBoard *adc = pAdc[ brd-1 ]; 	
	if( !adc )  {
		lastErrorNumber = E_NO_ADC_CONTROL;
		return FALSE;
	}
	adc->SetCallback( func );
	return TRUE;
}

BOOL CPSNADBoard::OpenPort( HANDLE hBoard )
{
	int brd = (int)hBoard;
	if( brd < 1 || (brd-1) >= MAX_ADC_HANDLES )  {
		lastErrorNumber = E_BAD_HANDLE;
		return FALSE;
	}
	CAdcBoard *adc = pAdc[ brd-1 ]; 	
	if( !adc )  {
		lastErrorNumber = E_NO_ADC_CONTROL;
		return FALSE;
	}
	return adc->OpenPort( FALSE );
}

BOOL CPSNADBoard::ClosePort( HANDLE hBoard )
{
	int brd = (int)hBoard;
	if( brd < 1 || (brd-1) >= MAX_ADC_HANDLES )  {
		lastErrorNumber = E_BAD_HANDLE;
		return FALSE;
	}
	CAdcBoard *adc = pAdc[ brd-1 ]; 	
	if( !adc )  {
		lastErrorNumber = E_NO_ADC_CONTROL;
		return FALSE;
	}
	adc->ClosePort( FALSE );
	return TRUE;
}

CPSNADBoard::~CPSNADBoard()
{
	CloseAllBoards();
}

void StrDateTime( time_t t, char *to, int utc )
{
	struct tm *nt;
	
	if( utc )
		nt = gmtime( &t );
	else
		nt = localtime( &t );
	sprintf(to, "%02d/%02d/%02d %02d:%02d:%02d", 
		nt->tm_mon+1, nt->tm_mday, nt->tm_year % 100, nt->tm_hour, nt->tm_min, nt->tm_sec );
}

time_t MakeLTime( int year, int mon, int day, int hour, int min, int sec )
{
	struct tm in;

	in.tm_year = year - 1900;
	in.tm_mon = mon-1;
	in.tm_mday = day;
	in.tm_hour = hour;
	in.tm_min = min;
	in.tm_sec = sec;
	in.tm_wday = 0;
	in.tm_isdst = -1;
	in.tm_yday = 0;
#ifdef WIN32_64
	return _mkgmtime( &in );
#elif ANDROID_LIB
	return timegm64( &in );
#else
	return timegm( &in );
#endif
}

void ms_sleep( int ms )
{
#ifdef WIN32_64
	Sleep( ms );
#else
	usleep( ms * 1000 );
#endif
}

#ifndef WIN32_64
/* Save debug information into a log file */
void libLogWrite( char *pszFormat, ...)
{
	FILE *logFile;
	char buff[ 1024 ], date[ 64 ], fname[ 64 ];
	va_list ap;
	time_t t;
	struct tm *nt;
			
	time( &t );
	nt = gmtime( &t );
	sprintf( fname, "/tmp/psnadboard_%02d%02d.log", nt->tm_mon+1, nt->tm_mday );
	
	va_start( ap, pszFormat );
	vsprintf( buff, pszFormat, ap );
	va_end( ap );
	if( (logFile = fopen( fname, "a") ) == NULL )
		return;
	sprintf( date, "%02d/%02d/%02d %02d:%02d:%02d", nt->tm_mon+1, nt->tm_mday, 
		nt->tm_year % 100, nt->tm_hour, nt->tm_min, nt->tm_sec );
	fprintf(logFile, "%s %s\n", date, buff ); 
	fclose(logFile);
}

void GetSystemTime( SYSTEMTIME *systime )
{
	time_t local_time;
	struct tm *local_tm;
	TIMEVAL tv;
 
	gettimeofday(&tv, NULL);
	local_time = tv.tv_sec;
	local_tm = gmtime(&local_time);
	systime->wYear = local_tm->tm_year + 1900;
	systime->wMonth = local_tm->tm_mon + 1;
	systime->wDayOfWeek = local_tm->tm_wday;
	systime->wDay = local_tm->tm_mday;
	systime->wHour = local_tm->tm_hour;
	systime->wMinute = local_tm->tm_min;
	systime->wSecond = local_tm->tm_sec;
	systime->wMilliseconds = (tv.tv_usec / 1000) % 1000;
}
#else
/* Save debug information into a log file */
void libLogWrite( char *pszFormat, ...)
{
	FILE *logFile;
	char buff[1024], date[24], tmstr[24];
	va_list ap;
	static BOOL firstLog = TRUE;
	
	va_start( ap, pszFormat );
	vsprintf( buff, pszFormat, ap );
	va_end( ap );
	if( firstLog )  {
		if( (logFile = fopen( "psnboard.log", "w") ) == NULL )
			return;
		firstLog = 0;
	}
	else if( (logFile = fopen( "psnboard.log", "a") ) == NULL )
		return;
	_strdate( date );
	_strtime( tmstr );
	fprintf(logFile, "%s %s %s\n", date, tmstr, buff );
	fclose(logFile);
}
#endif
