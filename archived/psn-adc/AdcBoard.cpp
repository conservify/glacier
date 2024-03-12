// AdcBoard.cpp

#include "PSNADBoard.h"

#ifdef WIN32_64
static int needSetup = 1;
#else
#define closesocket	close
#endif

#define SLEEP_TIME			20				// in milliseconds

CAdcBoard::CAdcBoard()
{
	hPort = (HANDLE)-1;
	m_socket = INVALID_SOCKET;
	m_tcpipMode = 0;
	m_tcpipConnectSts = 0;
	m_tcpipConnectError = 0;
	Callback = 0;
	goodConfig = 0;
	outputQueue.RemoveAll();
	adcBoardType = lastBoardType = BOARD_UNKNOWN;
	saveAllFp = 0;
	lastErrorNumber = E_OK;
	m_lastSendTime = 0;
	m_checkTimeCount = 0;
	m_debug = 0;
	m_isUsbDevice = 0;
	m_noUsbDataError = 0;
	m_flushComm = 0;
}

CAdcBoard::~CAdcBoard()
{
	if( ( hPort != (HANDLE)-1 ) || ( m_socket != INVALID_SOCKET ) )  {
		ClosePort( 0 );
		goodConfig = 0;
	}
	outputQueue.RemoveAll();
	if( saveAllFp )  {
		fclose( saveAllFp );
		saveAllFp = 0;
	}
}

void CAdcBoard::ProcessNewPacket( BYTE *packet )
{
	PreHdr *preHdr = (PreHdr *)packet;
	
	if( m_debug && preHdr->type != 'D' )
		SendMsgFmt( ADC_MSG, "New Adc Packet flags=%x type=%c", preHdr->flags, preHdr->type );
	
	if( adcBoardType == BOARD_UNKNOWN )  {
		if( preHdr->flags == 0xc0 )  {
			adcBoardType = BOARD_SDR24;
			adcSdr24.SetRefBoardType();
		}
		else if( preHdr->flags & 0x80 )  {
			if( preHdr->flags & 0x01 )
				adcBoardType = BOARD_V3;
			else
				adcBoardType = BOARD_V2;
			adcV2.SetRefBoardType( adcBoardType );
		}
		else if( preHdr->flags & 0x40 )  {
			adcBoardType = BOARD_VM;
			adcVM.SetRefBoardType();
		}
		else
			adcBoardType = BOARD_V1;
		lastBoardType = adcBoardType;
	}
	if( adcBoardType == BOARD_V2 || adcBoardType == BOARD_V3 )
		adcV2.ProcessNewPacket( packet );
	else if( adcBoardType == BOARD_VM )
		adcVM.ProcessNewPacket( packet );
	else if( adcBoardType == BOARD_SDR24 )
		adcSdr24.ProcessNewPacket( packet );
	else
		adcV1.ProcessNewPacket( packet );
}

void CAdcBoard::SendQueueAdcData( DWORD type, DataHeader *hdr, void *data, DWORD dataLen )
{
	if( Callback )  {
		memcpy( &cbDataHeader, hdr, sizeof( DataHeader ) );
		memcpy( cbData, data, dataLen );
		Callback( (BYTE)type, &cbDataHeader, cbData, dataLen );
	}
	else
		userQueue.Add( type, (void *)hdr, (void *)data, dataLen );
}

void CAdcBoard::SendQueueData( DWORD type, BYTE *data, DWORD dataLen )
{
	if( Callback )  {
		memcpy( cbString, data, dataLen );
		Callback( (BYTE)type, cbString, 0, dataLen );
	}
	else
		userQueue.Add( type, (void *)data, 0, dataLen );
}

void CAdcBoard::SendMsgFmt( DWORD type, const char *pszFormat, ... )
{
	char string[2048];
	va_list ap;
	
	va_start( ap, pszFormat );
	vsprintf( string, pszFormat, ap );
	va_end( ap );
	SendQueueData( type, (BYTE *)string, (DWORD)strlen( string ) + 1 );
}

void CAdcBoard::SendCurrentTime()
{
	if( outputQueue.AnyData() )
		return;
	sendTimeFlag = 0;
	if( adcBoardType == BOARD_V2 || adcBoardType == BOARD_V3 )
		adcV2.SendCurrentTime( TRUE );
	else if( adcBoardType == BOARD_VM )
		adcVM.SendCurrentTime( TRUE );
	else if( adcBoardType == BOARD_SDR24 )
		adcSdr24.SendCurrentTime( TRUE );
	else
		adcV1.SendCurrentTime();
}

void CAdcBoard::CheckCurrentTime()
{
	if( outputQueue.AnyData() )
		return;
	checkTimeFlag = 0;
	if( adcBoardType == BOARD_V2 || adcBoardType == BOARD_V3 )
		adcV2.SendTimeDiffRequest( 0 );
	else if( adcBoardType == BOARD_VM )
		adcVM.SendTimeDiffRequest( 0 );
	else if( adcBoardType == BOARD_SDR24 )
		adcSdr24.SendTimeDiffRequest( 0 );
}

void CAdcBoard::CheckSendTime()
{
	time_t now;
	
	if( ++m_checkTimeCount < 10 )
		return;
	m_checkTimeCount = 0;
	if( adcBoardType == BOARD_SDR24 || adcBoardType == BOARD_V3 )  {
		time( &now );
		if( ( now - m_lastSendTime ) > 15 )  {
			adcSdr24.SendPacket( 'R' );
		}
	}
}

void CAdcBoard::ReadXmitThread()
{
	int noDataCount = 0, procData;
	
	bThreadRunning = TRUE;
	m_tcpipConnectSts = 0;
	m_threadEndTime = 0;
	m_noUsbDataError = 0;
			
	if( m_tcpipMode )  {
		if( !TryToConnect() )  {
			m_tcpipConnectError = TRUE;
			bThreadRunning = bExitThread = 0;
			time( &m_threadEndTime );
			return;
		}
		m_tcpipConnectError = FALSE;
	}
	
	while( !bExitThread )  {
		if( m_flushComm )  {
			FlushComm();
			m_flushComm = 0;
			noDataCount = 0;
		}
		procData = 0;
		if( GetCommData() && !blockNewData )  {
			ProcessNewPacket( newPacket );
			noDataCount = 0;
			m_noUsbDataError = 0;
			CheckSendTime();
			procData = TRUE;
		}

		if( CheckOutputQueue() )  {
			SendMsgFmt( ADC_MSG, "Output Queue Write Error");
			break;
		}

		if( sendTimeFlag )
			SendCurrentTime();
		else if( checkTimeFlag )
			CheckCurrentTime();
			
		if( !procData )  {
			ms_sleep( SLEEP_TIME );
			if( ++noDataCount >= NO_DATA_WAIT )  {
				noDataCount = 0;
				if( m_isUsbDevice )  {
					m_noUsbDataError = TRUE;
					SendMsgFmt( ADC_ERROR, "No USB Data Timeout");
				}
				else
					SendMsgFmt( ADC_ERROR, "No Data Timeout");
				ResetBoard();
				if( m_tcpipMode )  {
					m_tcpipConnectError = TRUE;
					m_tcpipConnectSts = 0;
					if( m_socket != INVALID_SOCKET )  {
						closesocket( m_socket );
						m_socket = INVALID_SOCKET;
					}
					break;
				}
			}
		}
	}
	
	SendMsgFmt( ADC_MSG, "Exit Receive Data Thread");
	bThreadRunning = bExitThread = 0;
	time( &m_threadEndTime );
}

#ifdef WIN32_64
unsigned __stdcall  AdcThreadSub( LPVOID parm )
{
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
	CAdcBoard *pAdc = (CAdcBoard *)parm;
	pAdc->ReadXmitThread();
	_endthreadex( 0 );
	return 1;
}
#else
void *AdcThreadSub( void *parm )
{
	CAdcBoard *pAdc = (CAdcBoard *)parm;
	pAdc->ReadXmitThread();
	pthread_exit(NULL);
}
#endif

#ifdef WIN32_64

BOOL CAdcBoard::OpenPort( BOOL reset )
{
	UINT thread;
	DWORD dwChars;
	DCB dcb;
	COMMTIMEOUTS CommTimeOuts;
	char portStr[16], infoStr[16], szStr[ 1024 ];
	OSVERSIONINFO osvi;
	
	lastErrorNumber = E_OK;
	adcBoardType = BOARD_UNKNOWN;
		
	if( !goodConfig )  {
		lastErrorNumber = E_NO_CONFIG_INFO;
		return FALSE;
	}		
	
	if( saveAllFp )
		SaveAllData( 0, 0, 0 );
	
	InitAdc();
	
	if( ( ( config.commPort <= 0 ) || ( config.commPort > 255 ) ) && strlen( config.commPortTcpHost ) )  {
		return TcpIpConnect( config.commPortTcpHost, config.tcpPort );
	}
	
	if( config.commPort >= 10 )  {
		sprintf( portStr, "\\\\.\\COM%d", config.commPort );
		sprintf( infoStr, "COM%d", config.commPort );
	}
	else  {
		sprintf( portStr, "COM%d", config.commPort );
		strcpy( infoStr, portStr );
	}
	hPort = CreateFile(portStr, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if( hPort == (HANDLE)-1 )  {
		lastErrorNumber = E_OPEN_PORT_ERROR;
		return FALSE;
	}
	
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	m_isUsbDevice = 0;
 	if( GetVersionEx( &osvi ) && (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) )  {
    	dwChars = QueryDosDevice( infoStr, szStr, sizeof( infoStr ) );
		if( dwChars )  {
			szStr[dwChars] = 0;
			if( strstr( szStr, "\\Device\\VCP"))  {
				m_isUsbDevice = TRUE;
			}
		}
	}
	
	if( m_isUsbDevice )
		SendMsgFmt( ADC_MSG, "USB Device Detected" );
	
	SetupComm(hPort, 32768, 1024);
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 5000;
	SetCommTimeouts(hPort, &CommTimeOuts);
	
	dcb.DCBlength = sizeof(DCB);
	GetCommState(hPort, &dcb);
	dcb.BaudRate = config.commSpeed;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	dcb.fOutxDsrFlow = 0;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fOutxCtsFlow = 0;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.fInX = dcb.fOutX = 0;
	dcb.fBinary = TRUE;
	dcb.fParity = TRUE;
	dcb.fDsrSensitivity = 0;
	SetCommState(hPort, &dcb);
	EscapeCommFunction( hPort, CLRDTR );
	
	if( !reset )  {
		bExitThread = FALSE;
		if( !_beginthreadex(NULL, 0, AdcThreadSub, this, 0, &thread ) )  { 
			lastErrorNumber = E_THREAD_START_ERROR;
			SendMsgFmt( ADC_ERROR, "Error Creating Receiver Thread" );
			return FALSE;
		}
	}
	
	ms_sleep( 200 );
	
	PurgeComm( hPort, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR );
	
	return TRUE;
}

#elif ANDROID_LIB

BOOL CAdcBoard::OpenPort( BOOL reset )
{
	struct termios tio;
	UINT flags;
	
	lastErrorNumber = E_OK;
	adcBoardType = BOARD_UNKNOWN;
	
	if( !goodConfig )  {
		lastErrorNumber = E_NO_CONFIG_INFO;
		return FALSE;
	}
			
	if( saveAllFp )
		SaveAllData( 0, 0, 0 );
	
	if( FtdiOpen( 0, config.commSpeed ) == 0 )  {
		lastErrorNumber = E_OPEN_PORT_ERROR;
		return FALSE;
	}
	
	hPort = 1;
	
	InitAdc();
	
	// Reset Board here....
	FtdiSetClrDtr( TRUE );
	ms_sleep( 250 );
	FtdiSetClrDtr( FALSE );
	ms_sleep( 250 );
		
	if( !reset )  {
		bExitThread = FALSE;
		if( pthread_create( &readThread, NULL, &AdcThreadSub, this ) )  {
			lastErrorNumber = E_THREAD_START_ERROR;
			SendMsgFmt( ADC_ERROR, "Error Creating Receiver Thread" );
			return FALSE;
		}		
	}
	
	ms_sleep( 200 );
	
	FtdiPurge();
 
	return TRUE;
}
#else

BOOL CAdcBoard::OpenPort( BOOL reset )
{
	struct termios tio;
	UINT flags;
	
	lastErrorNumber = E_OK;
	adcBoardType = BOARD_UNKNOWN;
	
	if( !goodConfig )  {
		lastErrorNumber = E_NO_CONFIG_INFO;
		return FALSE;
	}
			
	if( saveAllFp )
		SaveAllData( 0, 0, 0 );
	
	InitAdc();

	if( ( ( config.commPort <= 0 ) || ( config.commPort > 255 ) ) && strlen( config.commPortTcpHost ) )  {
		return TcpIpConnect( config.commPortTcpHost, config.tcpPort );
	}
	
	hPort = open( config.commPortTcpHost, O_RDWR | O_NOCTTY | O_NONBLOCK | O_NDELAY );
	if( hPort== -1 )  {
		lastErrorNumber = E_OPEN_PORT_ERROR;
		return FALSE;
	}	
	memset( &tio, 0, sizeof(struct termios ) );
	cfmakeraw( &tio );
	
#ifdef _MACOSX
	cfsetspeed( &tio, config.commSpeed );
    tio.c_cflag = CS8 | CLOCAL | CREAD;
#else
	if( config.commSpeed == 57600 )
	    tio.c_cflag = B57600 | CS8 | CLOCAL | CREAD;
	else if( config.commSpeed == 38400 )
		tio.c_cflag = B38400 | CS8 | CLOCAL | CREAD;	
	else if( config.commSpeed == 19200 )
		tio.c_cflag = B19200 | CS8 | CLOCAL | CREAD;	
	else if( config.commSpeed == 9600 )
		tio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;	
 	else  {
		lastErrorNumber = E_CONFIG_ERROR;
		return FALSE;
	}
#endif

    tio.c_cc[VMIN] = 1;
	
    tcsetattr( hPort, TCSANOW, &tio );
		
	ioctl( hPort, TIOCMGET, &flags );
	flags |= TIOCM_DTR;
	ioctl( hPort, TIOCMSET, &flags );
	ms_sleep( 250 );
	ioctl( hPort, TIOCMGET, &flags );
	flags &= ~TIOCM_DTR;
	ioctl( hPort, TIOCMSET, &flags );
	ms_sleep( 250 );
		
	if( !reset )  {
		bExitThread = FALSE;
		if( pthread_create( &readThread, NULL, &AdcThreadSub, this ) )  {
			lastErrorNumber = E_THREAD_START_ERROR;
			SendMsgFmt( ADC_ERROR, "Error Creating Receiver Thread" );
			return FALSE;
		}		
	}
	
	ms_sleep( 200 );
 
	return TRUE;
}
#endif

BOOL CAdcBoard::NewConfig( AdcBoardConfig2 *newConfig )
{
	BOOL newBaud = 0, newTcp = 0;

	if( goodConfig && goodData && newConfig->commPort == (ULONG)-1 )  {		// check for tcpip mode
		if( strcmp( newConfig->commPortTcpHost, config.commPortTcpHost ) || 
				( newConfig->tcpPort != config.tcpPort ) )  {
			newTcp = 1;	
		}
	}
		
	if( goodConfig && goodData && ( newConfig->commSpeed != config.commSpeed ) )
		newBaud = TRUE;
	
	config = *newConfig;
	goodConfig = TRUE;
	if( newTcp || ( goodData && adcBoardType != BOARD_UNKNOWN ) )  {
		blockNewData = TRUE;
		ms_sleep( 200 );
		if( adcBoardType == BOARD_V2 || adcBoardType == BOARD_V3 )
			adcV2.SendConfigPacket();
		else if( adcBoardType == BOARD_VM )
			adcVM.SendConfigPacket();
		else if( adcBoardType == BOARD_SDR24 )
			adcSdr24.SendConfigPacket();
		else
			adcV1.SendConfigPacket();
		ms_sleep( 200 );
		if( newBaud )  {
			ClosePort( 0 );
			OpenPort( 0 );
		}
		blockNewData = FALSE;
	}
	return TRUE;
}

void CAdcBoard::SetCallback( void (*func)( DWORD type, void *data, void *data1, DWORD len ) )
{
	Callback = func;
}

void CAdcBoard::ClosePort( BOOL reset )
{
	if( ( hPort == (HANDLE)-1 ) && ( m_socket == INVALID_SOCKET ) )
		return;
	
	if( saveAllFp )
		SaveAllData( 1, 0, 0 );
		
	if( !reset )  {
		blockNewData = TRUE;
		if( goodData && ( adcBoardType != BOARD_UNKNOWN ) )  {
			if( adcBoardType == BOARD_V2 || adcBoardType == BOARD_V3 )  {
				adcV2.SendPacket( 'x' );
			}
			else if( adcBoardType == BOARD_VM )
				adcVM.SendPacket( 'x' );
			else if( adcBoardType == BOARD_SDR24 )
				adcSdr24.SendPacket( 'x' );
			else
				adcV1.SendExitCommand();
			ms_sleep( 250 );
			goodData = 0;
		}
		
		if( bThreadRunning )  {
			bExitThread = TRUE;
			int cnt = 1000;
			if( m_tcpipMode && m_socket != INVALID_SOCKET )  {
				closesocket( m_socket );
				m_socket = INVALID_SOCKET;
			}
			while( bExitThread && cnt )  { 
				ms_sleep( 10 );
				--cnt;
			}
		}
	}
	
	if( m_socket != INVALID_SOCKET )  {
		closesocket( m_socket );
		m_socket = INVALID_SOCKET;
	}

	if( hPort != (HANDLE)-1 )
#ifdef WIN32_64
		CloseHandle( hPort );
#elif ANDROID_LIB
		FtdiClose();
#else
	 	close( hPort );
#endif
	hPort = (HANDLE)-1;
}

BOOL CAdcBoard::GetCommData()
{
	DWORD numRead;
	BYTE crc, data;
	BOOL ret;
	short *sptr;
	int inlen,sts;
	struct timeval timeout;
	fd_set fds;
#ifdef WIN32_64
	COMSTAT ComStat;
	DWORD dwErrFlags, rcvd;
#else
	int  rcvd;
#endif
	
	if( !curCnt )  {	// if no outstanding local data read more from the serial port or tcp/ip stack
		if( m_tcpipMode )  {
			inlen = 0;
			FD_ZERO(&fds);
			FD_SET( m_socket, &fds );
			memset(&timeout, 0, sizeof(timeout));
			sts = select( (int)m_socket+1, &fds, NULL, NULL, &timeout );
			if( sts > 0 )  {
				inlen = 2048;
			}
		}
		else  {
#ifdef WIN32_64
			ClearCommError( hPort, &dwErrFlags, &ComStat );
			inlen = ComStat.cbInQue;
#elif ANDROID_LIB
			inlen = FtdiGetReadQueue();
#else
			ioctl( hPort, FIONREAD, &inlen );
#endif		
		}
		if( inlen <= 0 )
			return 0;
		
		if(inlen > (int)maxInQue)
			maxInQue = inlen;
		if( inlen > 4095)
			numRead = 4095;
		else
			numRead = inlen;
		if( m_tcpipMode )  {
#ifdef WIN32_64
			rcvd = recv( m_socket, (char *)inBuff, numRead, 0 );
#else
			rcvd = read( m_socket, (char *)inBuff, numRead );	
#endif
			if( rcvd < 0 )  {
				m_tcpipConnectError = bExitThread = TRUE;
				SendMsgFmt( ADC_ERROR, "TCP/IP Connection Failed with %s", (const char *)m_tcpipHost );
			}
			if( rcvd <= 0 )
				return FALSE;
			curCnt = rcvd;
			curPtr = inBuff;
		}
		else  {
#ifdef WIN32_64
			ReadFile(hPort, inBuff, numRead, &rcvd, NULL);
			curCnt = rcvd;
#elif ANDROID_LIB
			curCnt = FtdiRead( (char *)inBuff, numRead );
#else
			curCnt = read( hPort, inBuff, numRead );
#endif		
			curPtr = inBuff;
		}
	}
	
	while( curCnt )  {				// process each stored byte
		data = *curPtr++;
		if( inHdr )  {
			if( hdrState == 7 )   {		// get flags
				sptr = (short *)&inPacket[4];
				dataLen = *sptr;
				if( dataLen >= 0 && dataLen <= 4000 )  {
					*currPkt++ = data;
					++packLen;
					inHdr = 0;
				}
				else  {
					currPkt = inPacket;
					packLen = hdrState = 0;
					curCnt = 0;
					break;
				}
			}
			else if( hdrState == 4 || hdrState == 5 || hdrState == 6 )  {	// data len info
				*currPkt++ = data;
				++packLen;
				++hdrState;
			}  
			else if( data == hdrStr[ hdrState ] )  {		// see if hdr signature
				*currPkt++ = data;
				++packLen;
				if( !hdrState )
#ifdef WIN32_64
					packetStartTime = GetTickCount();
#else
					gettimeofday( &packetStartTime, NULL );
#endif
				++hdrState;
			}
			else if( hdrState )  {
				currPkt = inPacket;
				packLen = hdrState = 0;
				curCnt = 0;
				break;
			}
		}
		else  {					// store the data after the header
			*currPkt++ = data;
			++packLen;
			--dataLen;
			if( !dataLen )  {		// packet done
				crc = CalcCRC(&inPacket[ 4 ], (short)(packLen - 5));
				if(crc != inPacket[ packLen-1 ] )  {	// test the crc	
					SendMsgFmt( ADC_MSG, "Receive Packet CRC Error" );
					++crcErrors;				// count errors
					packLen = 0;
					ret = 0;
					curCnt = 0;
				}
				else  {
					CheckIDAndTime( inPacket );
					memcpy( newPacket, inPacket, packLen ); // save the new packet
					if( saveAllFp )
						SaveAllData( 3, newPacket, packLen );
					ret = TRUE;
				}
				inHdr = 1;				// reset for next packet
				currPkt = inPacket;
				packLen = 0;
				hdrState = 0;
				if( curCnt )
					--curCnt;
				return ret;
			}
		}
		if( curCnt )
			--curCnt;
	}
	return FALSE;
}

void CAdcBoard::CheckIDAndTime( BYTE *packet )
{
	DataHdrV1 *v1;	
	DataHdrV2 *v2; 
	DataHdrSdr24 *sdr24;
	PreHdr *preHdr = (PreHdr *)packet;
	DWORD id;
	
	if( preHdr->type != 'D' )
		return;
	if( ( preHdr->flags & 0xf0 ) == 0xc0 )  {
		sdr24 = (DataHdrSdr24 *)&packet[ sizeof( PreHdr ) ];
		id = sdr24->packetID;
	}
	else if( preHdr->flags & 0x80 )  {
		v2 = (DataHdrV2 *)&packet[ sizeof( PreHdr ) ];
		id = v2->packetID;
	}
	else if( preHdr->flags & 0x40 )  {
		v2 = (DataHdrV2 *)&packet[ sizeof( PreHdr ) ];
		id = v2->packetID;
		if( preHdr->len == ( sizeof(DataHdrV2) + 1) )  {	// if VM dummy packet skip test
			checkID = id;
			return;
		}
	}
	else  {
		v1 = (DataHdrV1 *)&packet[ sizeof( PreHdr ) ];
		id = v1->packetID;
	}
	if( ( checkID > 100 ) && id != ( checkID + 1 ) )
		SendMsgFmt( ADC_MSG, "Incoming Packet ID Error New=%d Last=%d", id, checkID );
	checkID = id;
}

/* Send data to the A/D Board */
void CAdcBoard::SendCommData( BYTE *data, int len )
{
	if( m_noUsbDataError )
		SendMsgFmt( ADC_MSG, "USB No Data Timeout Error; Blocking Output Queue Write" );
	else
		outputQueue.Add( data, len );
}

#ifndef WIN32_64
int WSAGetLastError()
{
	return errno;
}
#endif

BOOL CAdcBoard::CheckOutputQueue()
{
	BYTE output[256];
	DWORD len, sent;
	
	if( !outputQueue.AnyData() )
		return 0;
	
	outputQueue.PeekHead( output, &len );
	if( !len )  {
		SendMsgFmt( ADC_MSG, "peek queue !len");
		return 0;	
	}	
	
	if( m_debug )
		SendMsgFmt( ADC_MSG, "checkOutQueue type = %c", output[1] );
	
	time( &m_lastSendTime );
	
	if( m_tcpipMode && ( m_socket != INVALID_SOCKET ) )  {
		sent = send( m_socket, (char *)output, len, 0 );
		if( sent != len )  {
			if( WSAGetLastError() != WSAEWOULDBLOCK )  {
				m_tcpipConnectError = bExitThread = TRUE;
				SendMsgFmt( ADC_ERROR, "TCP/IP Connection Failed with %s", (const char *)m_tcpipHost );
			}
		}
		else  {
			if( saveAllFp )
				SaveAllData( 4, output, len );
			outputQueue.Remove( NULL, NULL );
		}
	}
		
	if( m_noUsbDataError )  {
		SendMsgFmt( ADC_MSG, "Blocking Write to USB Device" );
		outputQueue.RemoveAll();
		return 0;
	}
		
	if( !m_tcpipMode && ( hPort != (HANDLE)-1 ) )  {

#ifdef WIN32_64
		WriteFile(hPort, output, len, &sent, NULL);
#elif ANDROID_LIB
		sent = FtdiWrite( (char *)output, len );
#else
		sent = write( hPort, output, len );
#endif
		if( sent != len )  {
			SendMsgFmt( ADC_MSG, "CheckOutputQueue WriteFile Error len=%d sent=%d", len, sent );
			return 1;
		}
		else  {
			if( saveAllFp )
				SaveAllData( 4, output, len );
			outputQueue.Remove( NULL, NULL );
		}
	}
	
	return 0;
}

BYTE CAdcBoard::CalcCRC( BYTE *cp, short cnt )
{
	BYTE crc = 0;
	while(cnt--)
		crc ^= *cp++;
	return(crc);
}

void CAdcBoard::InitAdc()
{
	hdrStr[0] = 0xaa; hdrStr[1] = 0x55;
	hdrStr[2] = 0x88; hdrStr[3] = 0x44;
	inHdr = TRUE;
	currPkt = inPacket;
	packLen = hdrState = curCnt= 0;
	adcV1.owner = this;
	adcV2.owner = this;
	adcVM.owner = this;
	adcSdr24.owner = this;
	memcpy( &adcV1.hdrStr, &hdrStr, 4);
	maxInQue = configSent = goodData = blockNewData = 0;
	sendTimeFlag = checkTimeFlag = 0;
	checkID = 0;
	curPtr = inBuff;
	m_checkTimeCount = 0;
	time( &m_lastSendTime );
}

#ifdef WIN32_64

void CAdcBoard::ResetBoard()
{
	BOOL closePort = 0;
	
	if( saveAllFp )
		SaveAllData( 2, 0, 0 );
		
	SendMsgFmt( ADC_MSG, "Reset Board - NoUsbDataError=%d UsbDevice=%d", m_noUsbDataError, m_isUsbDevice );
	
	if( m_tcpipMode )  {
		InitAdc();
		if( lastBoardType == BOARD_V2 )
			SendBoardCommand( ADC_CMD_RESTART_BOARD, NULL );
		else if( lastBoardType != BOARD_UNKNOWN )
			SendBoardCommand( ADC_CMD_GOTO_BOOTLOADER, NULL );
		ms_sleep( 5000 );
		m_flushComm = TRUE;
		return;
	}		
	
	if( hPort == (HANDLE)-1 )
		closePort = TRUE;
	else
		ClosePort( TRUE );
	
	OpenPort( TRUE );
	if( hPort == (HANDLE)-1 )  {
		SendMsgFmt( ADC_MSG, "Port Reopen Error" );
		return;
	}
	
	if( lastBoardType == BOARD_V2 )
		SendBoardCommand( ADC_CMD_RESTART_BOARD, NULL );
	else if( lastBoardType != BOARD_UNKNOWN )
		SendBoardCommand( ADC_CMD_GOTO_BOOTLOADER, NULL );
	
	if( !m_tcpipMode )  {
		EscapeCommFunction( hPort, SETDTR );
		ms_sleep( 250 );
		EscapeCommFunction( hPort, CLRDTR );
		ms_sleep( 250 );
	}

	if( !closePort )  {
		SendMsgFmt( ADC_MSG, "Reset Board - Waiting 5 seconds" );
		ms_sleep( 5000 );
	}
	
	InitAdc();
	
	if( closePort )
		ClosePort( TRUE );
	else
		m_flushComm = TRUE;
}

#elif ANDROID_LIB

void CAdcBoard::ResetBoard()
{
	BOOL closePort = 0;
	UINT flags;

	if( saveAllFp )
		SaveAllData( 2, 0, 0 );
		
	SendMsgFmt( ADC_MSG, "Reset Board" );
		
	if( m_tcpipMode )  {
		InitAdc();
		if( lastBoardType == BOARD_V2 )
			SendBoardCommand( ADC_CMD_RESTART_BOARD, NULL );
		else if( lastBoardType != BOARD_UNKNOWN )
			SendBoardCommand( ADC_CMD_GOTO_BOOTLOADER, NULL );
		ms_sleep( 5000 );
		m_flushComm = TRUE;
		return;
	}

	if( lastBoardType == BOARD_V2 )
		SendBoardCommand( ADC_CMD_RESTART_BOARD, NULL );
	else if( lastBoardType != BOARD_UNKNOWN )
		SendBoardCommand( ADC_CMD_GOTO_BOOTLOADER, NULL );
	ms_sleep( 5000 );
	
	if( !m_tcpipMode )  {
		FtdiSetClrDtr( TRUE );
		ms_sleep( 250 );
		FtdiSetClrDtr( FALSE );
		ms_sleep( 250 );
	}
	
	InitAdc();
	
	m_flushComm = TRUE;
}

#else
void CAdcBoard::ResetBoard()
{
	BOOL closePort = 0;
	UINT flags;

	if( saveAllFp )
		SaveAllData( 2, 0, 0 );
		
	SendMsgFmt( ADC_MSG, "Reset Board" );
		
	if( m_tcpipMode )  {
		InitAdc();
		if( lastBoardType == BOARD_V2 )
			SendBoardCommand( ADC_CMD_RESTART_BOARD, NULL );
		else if( lastBoardType != BOARD_UNKNOWN )
			SendBoardCommand( ADC_CMD_GOTO_BOOTLOADER, NULL );
		ms_sleep( 5000 );
		m_flushComm = TRUE;
		return;
	}

	if( hPort == (HANDLE)-1 )
		closePort = TRUE;
	else
		ClosePort( TRUE );
	OpenPort( TRUE );
	if( hPort == (HANDLE)-1 )  {
		SendMsgFmt( ADC_MSG, "Port Reopen Error" );
		return;
	}

	if( lastBoardType == BOARD_V2 )
		SendBoardCommand( ADC_CMD_RESTART_BOARD, NULL );
	else if( lastBoardType != BOARD_UNKNOWN )
		SendBoardCommand( ADC_CMD_GOTO_BOOTLOADER, NULL );
	ms_sleep( 5000 );
	
	if( !m_tcpipMode )  {
		ioctl( hPort, TIOCMGET, &flags );
		flags |= TIOCM_DTR;
		ioctl( hPort, TIOCMSET, &flags );
		ms_sleep( 250 );
		ioctl( hPort, TIOCMGET, &flags );
		flags &= ~TIOCM_DTR;
		ioctl( hPort, TIOCMSET, &flags );
		ms_sleep( 250 );
	}

	InitAdc();
	
	if( closePort )
		ClosePort( TRUE );
	else
		m_flushComm = TRUE;
}

#endif

BOOL CAdcBoard::SendBoardCommand( DWORD cmd, void *data )
{
	if( cmd == ADC_CMD_RESET_BOARD )  {
		if( !goodConfig )
			return FALSE;
		ResetBoard();
		return TRUE;
	}
	if( cmd == ADC_CMD_DEBUG_REF )  {
		adcV2.SendBoardCommand( cmd, data );
		adcV1.SendBoardCommand( cmd, data );
		adcVM.SendBoardCommand( cmd, data );
		adcSdr24.SendBoardCommand( cmd, data );
		return TRUE;
	}
	if( cmd == ADC_CMD_DEBUG_SAVE_ALL )  {
		if( data )
	 		DebugSaveAll( 1 );
		else
	 		DebugSaveAll( 0 );
		return TRUE;
	}
	if( cmd == ADC_CMD_SET_DAC_A || cmd == ADC_CMD_SET_DAC_B )
		return adcVM.SendBoardCommand( cmd, data );
	
	if( cmd == ADC_CMD_SET_GAIN_REF )  {
		return adcSdr24.SendBoardCommand( cmd, data );
	}
		
	if( cmd == ADC_CMD_GPS_CONFIG )  {
		return adcSdr24.SendBoardCommand( cmd, data );
	}

	if( adcBoardType == BOARD_UNKNOWN )  {
		if( cmd == ADC_CMD_GOTO_BOOTLOADER )  {
			adcV2.SendPacket( 'b' );
			return TRUE;
		}
		return FALSE;
	}		

	if( cmd == ADC_CMD_GOTO_BOOTLOADER || cmd == ADC_CMD_RESTART_BOARD )  {
		if( adcBoardType == BOARD_V1 )
			return FALSE;
		if( adcBoardType == BOARD_V2 )  {
			if( cmd == ADC_CMD_GOTO_BOOTLOADER )
				adcV2.SendPacket( 'b' );
			else
				adcV2.SendPacket( 'R' );
		}
		else if( adcBoardType == BOARD_V3 )
			adcV2.SendPacket( 'b' );
		else if( adcBoardType == BOARD_VM )
			adcVM.SendPacket( 'b' );
		else if( adcBoardType == BOARD_SDR24 )
			adcSdr24.SendPacket( 'b' );
		return TRUE;
	}
		
	if( cmd == ADC_CMD_CLEAR_COUNTERS )  {
		maxInQue = 0;
		userQueue.queue.maxCount = outputQueue.queue.maxCount = 0;
	}	
	
	if( adcBoardType == BOARD_V1 )
		return adcV1.SendBoardCommand( cmd, data );
	if( adcBoardType == BOARD_V2 || adcBoardType == BOARD_V3 )
		return adcV2.SendBoardCommand( cmd, data );
	if( adcBoardType == BOARD_VM )
		return adcVM.SendBoardCommand( cmd, data );
	if( adcBoardType == BOARD_SDR24 )
		return adcSdr24.SendBoardCommand( cmd, data );
	
	return FALSE;
}

BOOL CAdcBoard::GetBoardInfo( DWORD type, void *data )
{
	if( type == ADC_GET_LAST_ERR_NUM )  {
		DWORD *ptr = (DWORD *)data;
		*ptr = lastErrorNumber;
		return TRUE;
	}
	if( type == ADC_GET_BOARD_TYPE )  {
		DWORD *ptr = (DWORD *)data;
		*ptr = adcBoardType;
		return TRUE;
	}
	if( type == ADC_GET_DLL_VERSION )  {
		DWORD ver, *ptr = (DWORD *)data;
		ver = ( DLL_MAJOR_VERSION * 10 ) + DLL_MINOR_VERSION;
		*ptr = ver;
		return TRUE;
	}
	if( adcBoardType == BOARD_UNKNOWN )
		return FALSE;
	if( type == ADC_GET_DLL_INFO )  {
		DLLInfo *info = (DLLInfo *)data;
		info->maxInQueue = maxInQue;
		info->maxUserQueue = userQueue.queue.maxCount;
		info->maxOutQueue = outputQueue.queue.maxCount;
		info->crcErrors = crcErrors; 
		info->userQueueFullCount = userQueue.GetFullCounter();
		info->xmitQueueFullCount = outputQueue.GetFullCounter();
		if( adcBoardType == BOARD_V2 || adcBoardType == BOARD_V3 )
			info->cpuLoopErrors = adcV2.currLoopErrors;
		else if( adcBoardType == BOARD_VM )
			info->cpuLoopErrors = adcVM.currLoopErrors;
		else if( adcBoardType == BOARD_SDR24 )
			info->cpuLoopErrors = adcSdr24.currLoopErrors;
		else
			info->cpuLoopErrors = 0;
		return TRUE;
	}
	if( type == ADC_GET_NUM_CHANNELS )  {
		DWORD *ptr = (DWORD *)data;
		if( adcBoardType == BOARD_VM )
			*ptr = adcVM.boardInfo.numConverters & 0x07;
		else if( adcBoardType == BOARD_SDR24 )
			*ptr = adcSdr24.boardInfo.modeNumConverters & 0x0f;
		else
			*ptr = 8;
		return TRUE;
	}
	if( type == ADC_GET_LAST_ERR_NUM )  {
		DWORD *ptr = (DWORD *)data;
		*ptr = lastErrorNumber;
		return TRUE;
	}
	
	return FALSE;
}

#ifdef WIN32_64
void CAdcBoard::SetComputerTime( LONG diff )
{
	SYSTEMTIME tm;
	FILETIME ft;
	UINT64 lt64;
	
	if( !SetPriv( TRUE ) )
		SendMsgFmt( ADC_MSG, "Can't Set Privileges");
		
	HANDLE process = GetCurrentProcess();
	DWORD priority = GetPriorityClass( process );
	if( ! priority )
		SendMsgFmt( ADC_MSG, "Can't get process priority");
	else
		SetPriorityClass( process, REALTIME_PRIORITY_CLASS );
	GetSystemTime( &tm );
	SystemTimeToFileTime( &tm, &ft );
	memcpy( &lt64, &ft, 8 );
	lt64 += ( diff * 10000 );
	memcpy( &ft, &lt64, 8 );
	FileTimeToSystemTime( &ft, &tm );
	if( !SetSystemTime( &tm ) )
		SendMsgFmt( ADC_MSG, "Error Settings Computer Time" );
	if( priority )
		SetPriorityClass( process, priority );
	SetPriv( FALSE );
}

BOOL CAdcBoard::SetPriv( BOOL fEnable )
{
    HANDLE hToken;
    LUID TakeOwnershipValue;
    TOKEN_PRIVILEGES tkp;
	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
			&hToken)) {
		return FALSE;
    }
	if(fEnable) {
		if(!LookupPrivilegeValue((LPSTR) NULL, SE_SYSTEMTIME_NAME, &TakeOwnershipValue))
			return FALSE;
        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Luid = TakeOwnershipValue;
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES),
			(PTOKEN_PRIVILEGES) NULL, (PDWORD) NULL);
        if(GetLastError() != ERROR_SUCCESS)
            return FALSE;
	}
	else {
		AdjustTokenPrivileges(hToken, TRUE, (PTOKEN_PRIVILEGES) NULL, (DWORD) 0,
			(PTOKEN_PRIVILEGES) NULL, (PDWORD) NULL);
		if (GetLastError() != ERROR_SUCCESS)
			return FALSE;
	}
	return TRUE;
}
#else
void CAdcBoard::SetComputerTime( LONG diff )
{
	struct timeval t_now, t_set;
	double d;
	int sts;
		
	gettimeofday( &t_now, NULL );
	d = t_now.tv_sec + ( t_now.tv_usec / 1000000.0 );
	d += ( diff / 1000.0 );
	t_set.tv_sec = (time_t)d;
	t_set.tv_usec = (long)(( d - (double)t_set.tv_sec ) * 1000000.0) ;
	sts = settimeofday( &t_set, NULL );
	if( sts )
		SendMsgFmt( ADC_MSG, "Error Setting Computer Time" );
}
#endif

CQueue::CQueue()
{
	count = size = fullCounter = maxCount = 0;
	array = 0;
}

CQueue::~CQueue()
{
	if( array )  {
		if( count )  {
			for( DWORD i = 0; i != count; i++ )
				free( array[ i ] );
		}
		free( array );
#ifdef WIN32_64
		DeleteCriticalSection( &lock );
#else
		pthread_mutex_destroy( &mutex );
#endif
	}
}

void CQueue::Init( DWORD qsize )
{
#ifdef WIN32_64
	InitializeCriticalSection( &lock );
#else
	pthread_mutex_init( &mutex, NULL );
#endif
	maxCount = count = fullCounter = 0;
	size = qsize;
	array = (void **)malloc( size * sizeof( void * ) );
}

BOOL CQueue::Add( void *ptr )
{
	if( count >= ( size - 1 ) || !array )  {
		return FALSE;
	}
#ifdef WIN32_64
	EnterCriticalSection( &lock );
#else
	pthread_mutex_lock( &mutex );
#endif
	array[ count++ ] = ptr;
	if( count > maxCount )
		maxCount = count;
#ifdef WIN32_64
	LeaveCriticalSection( &lock );
#else
	pthread_mutex_unlock( &mutex );
#endif
	return TRUE;
}

void *CQueue::Remove()
{
	if( !array )
		return NULL;

#ifdef WIN32_64
	EnterCriticalSection( &lock );
#else
	pthread_mutex_lock( &mutex );
#endif		
		
	if( !count )  {
#ifdef WIN32_64
		LeaveCriticalSection( &lock );
#else
		pthread_mutex_unlock( &mutex );
#endif		
		return NULL;
	}
	
	void *ret = array[0];
	
	if( count == 1 )
		array[0] = array[1];
	else
		memcpy( &array[0], &array[1], sizeof( void * ) * count );
	--count;

#ifdef WIN32_64
		LeaveCriticalSection( &lock );
#else
		pthread_mutex_unlock( &mutex );
#endif		
	return ret;
}

void *CQueue::Peek()
{
	if( !array )
		return NULL;

#ifdef WIN32_64
	EnterCriticalSection( &lock );
#else
	pthread_mutex_lock( &mutex );
#endif		
		
	if( !count )  {
#ifdef WIN32_64
		LeaveCriticalSection( &lock );
#else
		pthread_mutex_unlock( &mutex );
#endif		
		return NULL;
	}
	
	void *ret = array[0];

#ifdef WIN32_64
		LeaveCriticalSection( &lock );
#else
		pthread_mutex_unlock( &mutex );
#endif		
	return ret;
}

BOOL CQueue::IsEmpty()
{
	DWORD ret;
#ifdef WIN32_64
	EnterCriticalSection( &lock );
	ret = count;
	LeaveCriticalSection( &lock );
#else
	pthread_mutex_lock( &mutex );
	ret = count;
	pthread_mutex_unlock( &mutex );
#endif		
	if( ret )
		return FALSE;
	return TRUE;
}

BOOL COutQueue::Add( BYTE *data, DWORD len )
{
	OutQueueInfo *out = ( OutQueueInfo *)malloc( sizeof( OutQueueInfo ) );
	if( !out )
		return FALSE;
	out->data = (BYTE *)malloc( len );
	if( !out->data )  {
		free( out );
		return FALSE;
	}
	memcpy( out->data, data, len );
	out->dataLen = len;
	queue.Add( (void *)out );
	return TRUE;
}

BOOL COutQueue::Remove( BYTE *data, DWORD *len )
{
	OutQueueInfo *out = ( OutQueueInfo * )queue.Remove();
	if( !out )  {
		*len = 0;
		return FALSE;
	}
	if( data )
		memcpy( data, out->data, out->dataLen );
	if( len )
		*len = out->dataLen;
	free ( out->data );
	free( out );
	return TRUE;
}

BOOL COutQueue::PeekHead( BYTE *data, DWORD *len )
{
	OutQueueInfo *out = ( OutQueueInfo * )queue.Peek();
	if( !out )  {
		*len = 0;
		return FALSE;
	}
	memcpy( data, out->data, out->dataLen );
	*len = out->dataLen;
	return TRUE;
}

BOOL COutQueue::AnyData()
{
	if( !queue.IsEmpty() )
		return TRUE;
	return FALSE;
}
 
void COutQueue::RemoveAll()
{
	OutQueueInfo *out;
	DWORD cnt = queue.GetSize();
	if( !cnt )
		return;
	for( DWORD i = 0; i != cnt; i++ )  {
		out = (OutQueueInfo *)queue.Remove();
		if( out )  {
			free( out->data );
			free( out );
		}
	}
}

BOOL CUserQueue::AnyData()
{
	if( !queue.IsEmpty() )
		return TRUE;
	return FALSE;	
}

void CUserQueue::RemoveAll()
{
	UserQueueInfo *ui;
	DWORD cnt = queue.GetSize();
	if( !cnt )
		return;
	for( DWORD i = 0; i != cnt; i++ )  {
		ui = ( UserQueueInfo * )queue.Remove();
		if( ui )  {
			free( ui->data );
			free( ui );
		}
	}
}

BOOL CUserQueue::Remove( DWORD *type, void *data, void *data1, DWORD *dataLen )
{
	UserQueueInfo *ui;
	
	ui = ( UserQueueInfo * )queue.Remove();
	if( !ui )  {
		*dataLen = 0;
		return FALSE;
	}
	*type = ui->type;
	*dataLen = ui->dataLen;
	if( ui->type == ADC_AD_DATA )  {
		memcpy( data, ui->data, sizeof( DataHeader ) );
		memcpy( data1, &ui->data[ sizeof( DataHeader ) ], ui->dataLen );
	}
	else
		memcpy( data, ui->data, ui->dataLen );
	free( ui->data );
	free( ui );
	return TRUE;
}

BOOL CUserQueue::Add( DWORD type, void *data, void *data1, DWORD dataLen )
{
	UserQueueInfo *ui;
	
	ui = (UserQueueInfo *)malloc( sizeof( UserQueueInfo ) );
	if( !ui )
		return FALSE;
		
	if( type == ADC_AD_DATA )
		ui->data = (BYTE *)malloc( sizeof( DataHeader ) + dataLen );
	else
		ui->data = (BYTE *)malloc( dataLen );
	if( !ui->data )  {
		free( ui );
		return FALSE;
	}
	ui->type = (BYTE)type;
	ui->dataLen = dataLen;
	if( type == ADC_AD_DATA )  {
		memcpy( ui->data, data, sizeof( DataHeader) );
		memcpy( &ui->data[ sizeof( DataHeader) ], data1, dataLen );
	}
	else
		memcpy( ui->data, data, dataLen );
	queue.Add( (void *)ui );
	return TRUE;
}

void CAdcBoard::DebugSaveAll( BOOL onOff )
{
	if( onOff )  {
		SendMsgFmt( ADC_MSG, "Debug Save All On");
		saveAllFp = fopen("RawData.dat", "wb");
	}
	else if( saveAllFp ) {
		SendMsgFmt( ADC_MSG, "Debug Save All Off");
		fclose( saveAllFp );
		saveAllFp = 0;
	}
}

void CAdcBoard::SaveAllData( BYTE type, BYTE *data, int len )
{
	SaveAllHdr hdr;
	
	hdr.saveType = type;
	GetSystemTime( &hdr.tm );
	hdr.len = (WORD)len;
	fwrite( &hdr, 1, sizeof( SaveAllHdr ), saveAllFp );
	if( len )
		fwrite( data, 1, len, saveAllFp );
	fflush( saveAllFp );
}

BOOL CAdcBoard::GetBoardData( DWORD *type, void *data, void *data1, DWORD *dataLen )
{
	time_t now;
	
	if( userQueue.AnyData() )  {
		userQueue.Remove( type, data, data1, dataLen );
		return ADC_GOOD_DATA;
	}
	
	if( m_tcpipMode )  {
		if( ++m_tcpipTestCount >= 20 )  {
			m_tcpipTestCount = 0;
			if( m_tcpipConnectSts <= 1 && m_tcpipConnectError && !bThreadRunning && !bExitThread )  {
				time( &now );
				if( ( now - m_threadEndTime ) > 15 )  {
					SendMsgFmt( ADC_MSG, "Creating New TCP/IP Receiver Thread" );
					m_threadEndTime = now;
					m_socket = INVALID_SOCKET;
#ifdef WIN32_64
					UINT thread;
					if( !_beginthreadex(NULL, 0, AdcThreadSub, this, 0, &thread ) )  { 
						lastErrorNumber = E_THREAD_START_ERROR;
						SendMsgFmt( ADC_ERROR, "Error Creating Receiver Thread" );
						return ADC_NO_DATA;
					}
#else
					if( pthread_create( &readThread, NULL, &AdcThreadSub, this ) )  {
						lastErrorNumber = E_THREAD_START_ERROR;
						SendMsgFmt( ADC_ERROR, "Error Creating Receiver Thread" );
						return ADC_NO_DATA;
					}		
#endif			
				}
			}
			
			if( m_debug )
				SendMsgFmt( ADC_MSG, "Get ConnectSts=%d Socket=%d ConnectError=%d Running=%d", m_tcpipConnectSts, 
					m_socket, m_tcpipConnectError, bThreadRunning );
		}
	}
	
	return ADC_NO_DATA;
}

BOOL CAdcBoard::TcpIpConnect( char *hostStr, int port )
{
	m_tcpipMode = TRUE;
	m_tcpipPort = port;
	strcpy( m_tcpipHost, hostStr );
	m_tcpipTestCount = 0;
	m_socket = INVALID_SOCKET;
	bExitThread = FALSE;

#ifdef WIN32_64
	WSADATA WsaData;
	UINT thread;
	if( needSetup )  {
		if(WSAStartup(0x0101, &WsaData) == SOCKET_ERROR)  {
			SendMsgFmt( ADC_MSG, "WSAStartup() failed: %ld", GetLastError());
  			return FALSE;
		}
		needSetup = 0;
	}
	if( !_beginthreadex(NULL, 0, AdcThreadSub, this, 0, &thread ) )  { 
		lastErrorNumber = E_THREAD_START_ERROR;
		SendMsgFmt( ADC_ERROR, "Error Creating Receiver Thread" );
		return FALSE;
	}
#else
	if( pthread_create( &readThread, NULL, &AdcThreadSub, this ) )  {
		lastErrorNumber = E_THREAD_START_ERROR;
		SendMsgFmt( ADC_ERROR, "Error Creating Receiver Thread" );
		return FALSE;
	}		
#endif
	return(TRUE);
}

#ifdef WIN32_64
BOOL CAdcBoard::TryToConnect()
{
	int bufferSize;
	struct hostent *host;
	SOCKADDR_IN remoteAddr;
	IN_ADDR RemoteIpAddress;
	int timeout = 20000;
	UINT err;
	
	m_tcpipConnectSts = 1;
			
	if( ! ( host = gethostbyname( m_tcpipHost ) ) )  {
		SendMsgFmt( ADC_MSG, "gethostbyname failed Host = %s", m_tcpipHost );
		return 0;
	}		
	DWORD *d = (DWORD *)host->h_addr_list[0];
	RemoteIpAddress.S_un.S_addr	= *d;
	
	// Open a socket using the Internet Address family and TCP
	if((m_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)  {
		SendMsgFmt( ADC_MSG, "socket open failed: %ld", GetLastError());
		return 0;	
	}

	// Set the receive buffer size...
	bufferSize = 2048;
	err = setsockopt( m_socket, SOL_SOCKET, SO_RCVBUF, (char *)&bufferSize, sizeof(bufferSize) );
	if(err == SOCKET_ERROR)  {
    	SendMsgFmt( ADC_MSG, "setsockopt(SO_RCVBUF) failed: %ld", GetLastError() );
		closesocket( m_socket );
		m_socket = INVALID_SOCKET;
		return 0;
	}
	ZeroMemory(&remoteAddr, sizeof(remoteAddr));
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = htons( m_tcpipPort );
	remoteAddr.sin_addr = RemoteIpAddress;

	SendMsgFmt( ADC_MSG, "Connecting to %s...", m_tcpipHost );

	if( connect( m_socket, (PSOCKADDR)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR )  {
		SendMsgFmt( ADC_MSG, "Connect Failed: Error %ld", GetLastError());
		closesocket( m_socket );
		m_socket = INVALID_SOCKET;
		return 0;
	}
	
	setsockopt( m_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
	setsockopt( m_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	
	SendMsgFmt( ADC_MSG, "Connection Established with Host" );
	
	m_tcpipConnectSts = 2;
	
	return TRUE;
}
#else
BOOL CAdcBoard::TryToConnect()
{
	int bufferSize;
    struct hostent *host;
    struct sockaddr_in host_addr;
	int timeout = 20000;
	int err;
	
	m_tcpipConnectSts = 1;
			
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if( m_socket < 0 )  { 
		SendMsgFmt( ADC_MSG, "Socket open failed: errno = %d", errno );
		return 0;	
	}
	
    host = gethostbyname( m_tcpipHost  );
    if( host == NULL) {
		SendMsgFmt( ADC_MSG, "gethostbyname failed Host = %s", m_tcpipHost );
		return 0;
    }
    bzero((char *)&host_addr, sizeof( host_addr ) );
    host_addr.sin_family = AF_INET;
    bcopy((char *)host->h_addr, (char *)&host_addr.sin_addr.s_addr, host->h_length);
    host_addr.sin_port = htons( m_tcpipPort );
	
	SendMsgFmt( ADC_MSG, "Connecting to %s...", m_tcpipHost );
	
    if( connect( m_socket,(struct sockaddr *)&host_addr, sizeof( host_addr ) ) < 0)  {
		SendMsgFmt( ADC_MSG, "Connect Failed: Error %ld", errno );
		closesocket( m_socket );
		m_socket = INVALID_SOCKET;
		return 0;
	}
	
	bufferSize = 2048;
	err = setsockopt( m_socket, SOL_SOCKET, SO_RCVBUF, (char *)&bufferSize, sizeof(bufferSize) );
	if( err == SOCKET_ERROR )  {
    	SendMsgFmt( ADC_MSG, "setsockopt(SO_RCVBUF) failed: %ld", errno );
		closesocket( m_socket );
		m_socket = INVALID_SOCKET;
		return 0;
	}

	setsockopt( m_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
	timeout = 20000;
	setsockopt( m_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	
	SendMsgFmt( ADC_MSG, "Connection Established with Host" );
	
	m_tcpipConnectSts = 2;
	
	return TRUE;
}
#endif

void CAdcBoard::FlushComm()
{
	int max = 100, sts, rcvd;
	struct timeval timeout;
	fd_set fds;
	char buff[ 1024 ];
			
	curPtr = inBuff;
	packLen = hdrState = curCnt = 0;

	if( m_tcpipMode )  {
		while( max-- )  {
			FD_ZERO(&fds);
			FD_SET( m_socket, &fds );
			memset(&timeout, 0, sizeof(timeout));
			sts = select( (int)m_socket+1, &fds, NULL, NULL, &timeout );
			if( !sts )
				break;
			rcvd = recv( m_socket, (char *)buff, 1024, 0 );
			if( rcvd < 0 )  {
				m_tcpipConnectError = bExitThread = TRUE;
				SendMsgFmt( ADC_ERROR, "TCP/IP Connection Failed with %s", (const char *)m_tcpipHost );
			}
			if( rcvd <= 0 )
				break;
		}		
	}
	else  {
#ifdef WIN32_64
		PurgeComm( hPort, PURGE_RXCLEAR | PURGE_RXABORT );
#elif ANDROID_LIB
		FtdiPurge();
#else
		tcflush( hPort, TCIFLUSH );
#endif
	}
}
