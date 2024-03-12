// adcv1.cpp - PSN-ADC-SERIAL V1 Board Class

#include "PSNADBoard.h"

CAdcV1::CAdcV1()
{
	owner = NULL;
	setPCTime = checkPCTime = 0;
	currentPacketID = numberOfSamples = sampleLen = checkPCCount = 0;
}

void CAdcV1::ProcessNewPacket( BYTE *packet )
{
	PreHdr *preHdr = (PreHdr *)packet;
	
	// process the different incoming packet types
	switch( preHdr->type )  {
		case 'D':	ProcessDataPacket( packet ); break;
		case 'R':	ProcessRetranPacket( packet ); break;
		case 'L':	ProcessAdcMessage( packet ); break;
		case 'S':	ProcessStatusPacket( packet ); break;
		case 'T':	ProcessSaveTimeInfo( packet ); break;
		case 'g':	ProcessGpsData( packet ); break;
		case 'C':	SendConfigPacket(); break;
		case 'E':	owner->sendTimeFlag = TRUE; break;
		case 'e':	CheckTimePacket( packet ); break;
		default:	owner->SendMsgFmt( ADC_MSG, "V1 Unknown Packet type = %c", preHdr->type ); 
					break;
	}
}

BOOL CAdcV1::SendBoardCommand( DWORD cmd, void *data )
{
	switch( cmd )  {
		case ADC_CMD_EXIT:				SendExitCommand(); return TRUE;
		case ADC_CMD_SEND_STATUS:		SendCommand( 'S' ); return TRUE;
		case ADC_CMD_RESET_GPS:			SendCommand( 'g' ); return TRUE;
		case ADC_CMD_FORCE_TIME_TEST:	SendCommand( 'G' ); return TRUE;
		case ADC_CMD_CLEAR_COUNTERS:	SendCommand( 'L' );	return TRUE;
		case ADC_CMD_GPS_ECHO_MODE:		SendCommand( 'P' ); return TRUE;
		case ADC_CMD_GPS_DATA_ON_OFF:	
			if( data )
				SendCommand( 'p' , 1 );
			else
				SendCommand( 'p' , 0 ); 
			return TRUE;

	}
	return FALSE;
}

void CAdcV1::ProcessAdcMessage( BYTE *packet )
{
	char *str = (char *)&packet[ sizeof( PreHdr ) ];
	DWORD len = (DWORD)strlen( str );
	owner->SendQueueData( ADC_AD_MSG, (BYTE *)str, len + 1);
}

void CAdcV1::ProcessStatusPacket( BYTE *packet )
{
	StatusInfoV1 *sts = (StatusInfoV1 *)&packet[ sizeof(PreHdr) ];
	StatusInfo dllSts;
	
	dllSts.boardType = sts->sysType;
	dllSts.majorVersion = sts->majVer;
	dllSts.minorVersion = sts->minVer;
	dllSts.lockStatus = sts->lockSts;
	dllSts.numChannels = sts->numChannels;
	dllSts.spsRate = (BYTE)sts->sps;
	dllSts.crcErrors = sts->crcErrs;
	dllSts.numProcessed = sts->numProcessed;
	dllSts.numRetran = sts->numRetran;
	dllSts.numRetranErr = sts->numRetranErr;
	dllSts.packetsRcvd = sts->packetsRcvd;
	memcpy( &dllSts.timeInfo, &sts->timeInfo, sizeof( TimeInfo ) );
	owner->SendQueueData( ADC_STATUS, (BYTE *)&dllSts, sizeof( StatusInfo ) );
}

BOOL CAdcV1::IsValidSpsRate( WORD sps, WORD numChannels, char *errMsg )
{
	if( sps == 200 && numChannels > 4 )  {
		sprintf( errMsg, "Sample Rate (%d) and # Channels (%d) Error, Channels must be 4 or less at 200 SPS", 
			sps, numChannels);
		return FALSE;
	}
	if( sps == 5 || sps == 10 || sps == 20 || sps == 25 || sps == 50 || 
			sps == 100 || sps == 200 )
		return TRUE;
	sprintf( errMsg, "Sample Rate (%d) Error, must be 5, 10, 20, 25, 50, 100 or 200", sps );
	return FALSE;
}

BOOL CAdcV1::GoodConfig( AdcBoardConfig2 *cfg, char *errMsg )
{
	errMsg[ 0 ] = 0;
	return IsValidSpsRate( cfg->sampleRate, cfg->numberChannels, errMsg );
}

/* Send config information. */
BOOL CAdcV1::SendConfigPacket()
{
	int i, len = sizeof(ConfigInfo);
	BYTE crc, outDataPacket[ sizeof(ConfigInfo) + 16 ];
	PreHdr *pHdr = (PreHdr *)outDataPacket;
	ConfigInfo *cfg = (ConfigInfo *)&outDataPacket[sizeof(PreHdr)];
	AdcBoardConfig2 *adcCfg = &owner->config;
	int timeMode, ref;
	ULONG mask = 1;
	SYSTEMTIME st;
	char errMsg[ 128 ];
		
	if( !owner->goodConfig )
		return FALSE;
		
	if( !GoodConfig( adcCfg, errMsg ) )  {
		owner->SendMsgFmt( ADC_MSG, "Configuration Error = %s", errMsg );
		return FALSE;
	}
	
	owner->SendMsgFmt( ADC_MSG, "Sending Configuration Information" );
	
	setPCTime = checkPCTime = 0;
	checkPCCount = 30;
	if( adcCfg->checkPCTime && ( adcCfg->timeRefType != TIME_REF_USEPC )  )
		checkPCTime = TRUE;
	if( adcCfg->setPCTime && ( adcCfg->timeRefType != TIME_REF_USEPC ) )
		checkPCTime = setPCTime = TRUE;
	
	memset(cfg, 0, sizeof(ConfigInfo));
	
	ref = owner->config.timeRefType;
	if( ref == TIME_REF_USEPC )
		timeMode = LOCAL_MODE;
	else if( ref == TIME_REF_GARMIN )  {
		timeMode = GPS_MODE;
		cfg->flags |= CF_GPS_GARMIN;
	}
	else if( ref == TIME_REF_MOT_NMEA )  {
		timeMode = GPS_MODE;
		cfg->flags |= CF_GPS_MOT_NMEA;
	}
	else if( ref == TIME_REF_MOT_BIN )  {
		timeMode = GPS_MODE;
	}
	else if( ref == TIME_REF_WWVB )
		timeMode = WWVB_MODE;
	else 
		timeMode = WWV_MODE;
	if( adcCfg->noPPSLedStatus )
		cfg->flags |= CF_NO_ONE_PPS;
	cfg->timeMode = timeMode;
	cfg->baud = adcCfg->commSpeed;
	cfg->numChannels = (BYTE)adcCfg->numberChannels;
	cfg->sps = (BYTE)adcCfg->sampleRate;
	
	cfg->timeInfo.addDropTimer = adcCfg->addDropTimer;
	cfg->timeInfo.addDropMode = (char)adcCfg->addDropMode;
	cfg->timeInfo.pulseWidth = (WORD)adcCfg->pulseWidth;
	cfg->timeInfo.timeOffset = (short)adcCfg->timeOffset;
	for( i = 0; i != MAX_ADC_CHANNELS; i++ )  {
		if( adcCfg->mode12BitFlags & mask )
			cfg->mode12Bit[ i ] = TRUE;
		mask <<= 1;
	}
	GetSystemTime( &st );
	cfg->dayTime.year = st.wYear;
	cfg->dayTime.month = (BYTE)st.wMonth;
	cfg->dayTime.day = (BYTE)st.wDay;
	cfg->dayTime.hour = (BYTE)st.wHour;
	cfg->dayTime.min = (BYTE)st.wMinute;
	cfg->dayTime.sec = (BYTE)st.wSecond;
	cfg->dayTime.dayOfWeek = (BYTE)st.wDayOfWeek;
	InitAdc( adcCfg->sampleRate, adcCfg->numberChannels );
	
	memcpy( pHdr->hdr, hdrStr, 4 );
	pHdr->len = len + 1;
	pHdr->type = 'C';
	pHdr->flags = 0;
	crc = owner->CalcCRC(&outDataPacket[4], (short)(len + 4));
	outDataPacket[len + sizeof(PreHdr)] = crc;
	owner->SendCommData( outDataPacket, len + sizeof(PreHdr) + 1 );
	owner->configSent = TRUE;
	return TRUE;
}

void CAdcV1::SendExitCommand()
{
	int len = sizeof(ConfigInfo);
	BYTE crc, outDataPacket[ sizeof(ConfigInfo) + 16 ];
	PreHdr *pHdr = (PreHdr *)outDataPacket;
	ConfigInfo *cfg = (ConfigInfo *)&outDataPacket[sizeof(PreHdr)];
	
	owner->blockNewData = TRUE;
	owner->SendMsgFmt( ADC_MSG, "Sending Exit Command");
	owner->configSent = FALSE;
	
	memset(cfg, 0, sizeof(ConfigInfo));
	
	cfg->exitFlag = TRUE;
	
	memcpy( pHdr->hdr, hdrStr, 4 );
	pHdr->len = len + 1;
	pHdr->type = 'C';
	pHdr->flags = 0;
	crc = owner->CalcCRC(&outDataPacket[4], (short)(len + 4));
	outDataPacket[len + sizeof(PreHdr)] = crc;
	owner->SendCommData( outDataPacket, len + sizeof(PreHdr) + 1 );
}

void CAdcV1::ProcessSaveTimeInfo( BYTE *packet )
{
	owner->SendQueueData( ADC_SAVE_TIME_INFO, &packet[ sizeof( PreHdr ) ], sizeof( TimeInfo ) );
}

/* Process data packet */
void CAdcV1::ProcessDataPacket( BYTE *packet )
{
	DataHdrV1 *dataHdr = (DataHdrV1 *)&packet[sizeof(PreHdr)];
	ULONG pID = dataHdr->packetID;
	DataHeader header;
				
	if( noPacketCount )  {
		--noPacketCount;
		return;
	}
	if( checkPCTime )  {
		if( checkPCCount )
			--checkPCCount;
		if( !checkPCCount )  {
			SendTestCheckTime();
			checkPCCount = 60;
		}
	}
	if( !owner->configSent && owner->goodConfig )  {
		owner->SendMsgFmt( ADC_MSG, "Data before configuration sent - Resetting Board");
		InitAdc( owner->config.sampleRate, owner->config.numberChannels );
		owner->ResetBoard();
		SendConfigPacket();
		noPacketCount = 4;
		return;
	}
	/* On the very first data packet send a status request packet */
	if( !owner->goodData )
		SendCommand( 'S' );
	owner->goodData = TRUE;
	
	// Check for bad ID error, ID should always be higher then last
	if( currentPacketID )  {
		if( pID <= currentPacketID )  {
			owner->SendMsgFmt( ADC_ERROR, "Incoming ID Error New=%d Old=%d - Restarting ADC Board", 
				pID, currentPacketID );
			if( retranMode )  {
				owner->SendMsgFmt( ADC_MSG, "Retran ID Reset");
				retran.Reset();
			}
			InitAdc( owner->config.sampleRate, owner->config.numberChannels );
			retranMode = 0;
			owner->ResetBoard();
			SendConfigPacket();
			return;
		}
		else  {
			int diff = pID - ( currentPacketID + 1 );
			if( diff )
				owner->SendMsgFmt( ADC_MSG, "Hole in data diff=%d", diff );
				
			if( diff && retranMode )  {
				owner->SendMsgFmt( ADC_MSG, "Hole in data while retran mode, resetting diff=%d", diff );
				RetranQueueToUser();
				retran.Reset();
				retranMode = 0;
			}
			else if( diff && !retranMode )  {
				if( diff < MAX_RETRAN )  {
					owner->SendMsgFmt( ADC_MSG, "Retran Start Diff=%d", diff );
					if( retran.Start( currentPacketID+1, diff ) )  {
						retran.SavePacket( packet );
						retranMode = TRUE;
						SendNextRetranID();
						currentPacketID = pID;
						return;
					}
					else
						owner->SendMsgFmt( ADC_MSG, "Retran Start Error" );
				}
				else
					owner->SendMsgFmt( ADC_MSG, "Hole in incoming data to large to retransmit packet diff = %d", 
						diff );
			}
			else if( retranMode )  {
				owner->SendMsgFmt( ADC_MSG, "Retran Save Packet %d", pID );
				if( !retran.SavePacket( packet ) )  {
					owner->SendMsgFmt( ADC_MSG, "Retran Save Packet Error" );
					retran.Reset();
					retranMode = 0;
				}
				else if( retran.Check() )  {
					currentPacketID = pID;
					return;
				}
				RetranQueueToUser();
				retran.Reset();
				retranMode = 0;
			}
		}
	}
	currentPacketID = pID;
	
	UnpackRawData( packet, adcData );
	MakeHeader( &header, dataHdr );
	owner->SendQueueAdcData( ADC_AD_DATA, &header, (void *)adcData, sampleLen );
}

void CAdcV1::SendNextRetranID()
{
	DWORD id = retran.GetCurrentID();
	owner->SendMsgFmt( ADC_MSG, "Retrans ID Request=%d", id );	
	SendRetranPacket( id );
}

/* Process retransmission data packet */
void CAdcV1::ProcessRetranPacket( BYTE *packet )
{
	DataHeader header;
	if( !retranMode )
		return;
	
	DataHdrV1 *dataHdr = (DataHdrV1 *)&packet[ sizeof( PreHdr ) ];
	DWORD id = retran.GetCurrentID();
	owner->SendMsgFmt( ADC_MSG, "New Retran Packet ID=%d Looking for %d", dataHdr->packetID, id );	
	if( id != dataHdr->packetID )  {
		owner->SendMsgFmt( ADC_MSG, "Retran Packet ID Error");
		RetranQueueToUser();
		retran.Reset();
		retranMode = 0;
		return;
	}
	
	UnpackRawData( packet, adcDataRetran );
	MakeHeader( &header, dataHdr );
	owner->SendQueueAdcData( ADC_AD_DATA, &header, (void *)adcDataRetran, sampleLen );
	if( retran.Done() )  {
		RetranQueueToUser();
		retranMode = 0;
	}
	else  {
		retran.SetNextID();
		SendNextRetranID();
	}
}

/* Send a status request packet. When the A/D board receives this packet it will
   send a status packet back to the host */
void CAdcV1::SendCommand( char command )
{
	BYTE crc, outDataPacket[ 64 ];
	PreHdr *pHdr = (PreHdr *)outDataPacket;
	memcpy(pHdr->hdr, hdrStr, 4);
	pHdr->len = 1;
	pHdr->type = command;
	pHdr->flags = 0;
	crc = owner->CalcCRC(&outDataPacket[4], (short)4);
	outDataPacket[sizeof(PreHdr)] = crc;
	owner->SendCommData(outDataPacket, sizeof(PreHdr) + 1 );
}

/* Send a status request packet. When the A/D board receives this packet it will
   send a status packet back to the host */
void CAdcV1::SendCommand( char command, BYTE flags )
{
	BYTE crc, outDataPacket[ 64 ];
	PreHdr *pHdr = (PreHdr *)outDataPacket;
	memcpy(pHdr->hdr, hdrStr, 4);
	pHdr->len = 1;
	pHdr->type = command;
	pHdr->flags = flags;
	crc = owner->CalcCRC(&outDataPacket[4], (short)4);
	outDataPacket[sizeof(PreHdr)] = crc;
	owner->SendCommData(outDataPacket, sizeof(PreHdr) + 1 );
}

void CAdcV1::InitAdc( int spsRate, int numChannels )
{
	currentPacketID = 0;
	numberOfSamples = spsRate * numChannels;
	sampleLen = numberOfSamples * sizeof( short );
	coiLen = (numberOfSamples / 8) + 1;
	noPacketCount = 0;
	retranMode = 0;
}

void CAdcV1::UnpackRawData(BYTE *packet, short *data )
{
	BYTE *coi = &packet[sizeof(PreHdr) + sizeof(DataHdrV1)], mask = 1;
	int bitcnt = 0, cnt = numberOfSamples;
	char *ptr = (char *)&packet[sizeof(PreHdr) + sizeof(DataHdrV1) + coiLen ];
			
	while( cnt-- )  {				// for each sample unpack the data
		if( *coi & mask )  {		// full int word
			*data++ = *( (short *)ptr );
			ptr += 2;
		}
		else
			*data++ = *ptr++;	// char (+- 128) sample 
		if( ++bitcnt >= 8 )  {	// next coi array byte
			bitcnt = 0;
			mask = 1;
			++coi;
		}
		else
			mask <<= 1;			// next bit
	}
}

void CAdcV1::SendCurrentTime()
{
	BYTE crc, outDataPacket[ 64 ];
	short len = sizeof(ULONG);
	PreHdr *pHdr = (PreHdr *)outDataPacket;
	ULONG *lPtr = (ULONG *)&outDataPacket[sizeof(PreHdr)];
	SYSTEMTIME tm;
	
	memcpy(pHdr->hdr, hdrStr, 4);
	pHdr->len = len + 1;
	pHdr->type = 'e';
	pHdr->flags = 0;
	
#ifdef WIN32_64
	HANDLE process;
	DWORD priority;
	
	process = GetCurrentProcess();
	priority = GetPriorityClass( process );
	if( priority )
		SetPriorityClass( process, REALTIME_PRIORITY_CLASS );
#endif	
	GetSystemTime(&tm);		// get the system time
	// calculate the number of millseconds since midnight UTC time
	*lPtr = (((tm.wHour * 3600) + (tm.wMinute * 60) + tm.wSecond) * 1000) + 
			tm.wMilliseconds;
	crc = owner->CalcCRC(&outDataPacket[4], (short)(len + 4));
	outDataPacket[len + sizeof(PreHdr)] = crc;
	owner->SendCommData(outDataPacket, len + sizeof(PreHdr) + 1 );
#ifdef WIN32_64
	if( priority )
		SetPriorityClass( process, priority );
#endif
}

void CAdcV1::CheckTimePacket( BYTE *packet )
{
	LONG t, diff, absDiff, *lPtr;
	AdcBoardConfig2 *adcCfg = &owner->config;
	char str[80];
	SYSTEMTIME tm;
#ifndef WIN32_64
	TIMEVAL now;
	long long n, s;
#endif

	lPtr = (LONG *)&packet[ sizeof(PreHdr) ];
	if( *lPtr == -1 )			// -1 indicates time is not locked to a time reference
		return;
	GetSystemTime( &tm );
	/* if near midnight do nothing */
	if(tm.wHour == 23 && tm.wMinute >= 58)
		return;
	if(!tm.wHour && tm.wMinute <= 2)
		return;
	/* calculate the time in milliseconds passed midnight */
	t = (((tm.wHour * 3600) + (tm.wMinute * 60) + tm.wSecond) * 1000) + 
			tm.wMilliseconds;
	
	/* calculate the time difference */
#ifdef WIN32_64
	diff = ( (t - *lPtr) - ( GetTickCount() - owner->packetStartTime ) ) - 10;
#else
	gettimeofday( &now, NULL );
	n = ( now.tv_sec * 1000 ) + ( now.tv_usec / 1000 );
	s = ( owner->packetStartTime.tv_sec * 1000 ) + ( owner->packetStartTime.tv_usec / 1000 );
	diff = (t - *lPtr) - (n - s) - 10;
#endif
	
	absDiff = abs( diff );
	if( diff < 0 )
		sprintf( str, "-%d.%03d", absDiff / 1000, absDiff % 1000 );
	else
		sprintf( str, "%d.%03d", absDiff / 1000, absDiff % 1000 );
	if( setPCTime && ( absDiff > 100 ) )  {
		if( ( adcCfg->setPCTime == SET_PC_TIME_NORMAL ) && ( absDiff > 7200000 ) )  // 7200000 = 2H
			owner->SendMsgFmt( ADC_MSG, "Time between A/D board and computer to large=%s sec, time not set", str );
		else {
			owner->SendMsgFmt( ADC_MSG, "Adjusting computer time by %s seconds", str );
			owner->SetComputerTime( diff );
		}
	}
	else if( adcCfg->checkPCTime )
		owner->SendMsgFmt( ADC_MSG, "Time difference between A/D Board and Host Computer=%s seconds", str );
}

void CAdcV1::ProcessGpsData( BYTE *packet )
{
	PreHdr *preHdr = (PreHdr *)packet;
	BYTE *str;
			
	str = &packet[ sizeof(PreHdr) ];
	str[preHdr->len-1] = 0;
	owner->SendQueueData( ADC_GPS_DATA, str, preHdr->len );
}

/* Send a retransmission request packet. Not support in this test program. */
void CAdcV1::SendRetranPacket( ULONG num )
{
	BYTE crc, outDataPacket[ 64 ];
	short len = sizeof(ULONG);
	PreHdr *pHdr = (PreHdr *)outDataPacket;
	RetranNum *idNum = (RetranNum *)&outDataPacket[sizeof(PreHdr)];

	memcpy(pHdr->hdr, hdrStr, 4);
	pHdr->len = len + 1;
	pHdr->type = 'R';
	pHdr->flags = 0;
	idNum->packetID = num;
	crc = owner->CalcCRC(&outDataPacket[4], (short)(len + 4));
	outDataPacket[len + sizeof(PreHdr)] = crc;
	owner->SendCommData(outDataPacket, len + sizeof(PreHdr) + 1 );
}

void CAdcV1::MakeHeader( DataHeader *dh, DataHdrV1 *dv1 )
{
	SYSTEMTIME *st = &dh->packetTime;
	ULONG tick = dv1->timeTick;
	st->wDayOfWeek = 0;
	st->wYear = dv1->year;
	st->wMonth = dv1->month;
	st->wDay = dv1->day;
	
	st->wMilliseconds = (WORD)( tick % 1000 );
	tick /= 1000;
	st->wHour = (WORD)( tick / 3600 );
	tick %= 3600;
	st->wMinute = (WORD)( tick / 60 );
	st->wSecond = (WORD)( tick % 60 );
	dh->packetID = dv1->packetID;
	dh->timeRefStatus = dv1->lockSts;
	dh->flags = dv1->flags;
}

void CAdcV1::SendTestCheckTime()
{
	BYTE outDataPacket[ 64 ];
	PreHdr *pHdr = (PreHdr *)outDataPacket;
	BYTE crc;

	memcpy( pHdr->hdr, hdrStr, 4 );
	pHdr->len = 1;
	pHdr->type = 'E';
	pHdr->flags = 0;
	crc = owner->CalcCRC( &outDataPacket[4], (short)4 );
	outDataPacket[sizeof(PreHdr)] = crc;
	owner->SendCommData( outDataPacket, sizeof(PreHdr) + 1 );
}

void CAdcV1::RetranQueueToUser()
{
	BYTE *packet;
	DataHeader header;
	
	while( TRUE )  {
		if( !( packet = retran.GetQueuedPacket() ) )
			return;
		DataHdrV1 *dataHdr = (DataHdrV1 *)&packet[ sizeof( PreHdr ) ];
		UnpackRawData( packet, adcDataRetran );
		MakeHeader( &header, dataHdr );
		owner->SendQueueAdcData( ADC_AD_DATA, &header, (void *)adcDataRetran, sampleLen );
		free( packet );
	}
}

void CRetran::Init()
{
	queue.Init( 30 );
	number = currIdx = 0;
}

void CRetran::Reset()
{
	if( int count = queue.GetSize() )  {
		while( count-- )  {
			BYTE *ptr = (BYTE *)queue.Remove();
			if( ptr )
				free( ptr );
		}
	}
	number = currIdx = 0;
}

BOOL CRetran::Start( DWORD id, DWORD num )
{
	Retran *r;
	
	Reset();
	startTime = time(0);
	number = num;
	for( DWORD i = 0; i != num; i++ )  {
		r = &list[i];
		r->id = id++;
		r->state = RETRAN_NOT_SENT;
	}
	return TRUE;
}

BOOL CRetran::SavePacket( BYTE *packet )
{
	PreHdr *pHdr = (PreHdr *)packet;
	DWORD len = pHdr->len+sizeof( PreHdr)+1;
	BYTE *p;
	
	if( ! ( p = (BYTE *)malloc( len ) ) )
		return FALSE;
	memcpy( p, packet, len );
	if( !queue.Add( (void *)p ) )  {
		free( p );
		return FALSE;
	}
	return TRUE;
}

DWORD CRetran::GetCurrentID()
{
	return list[currIdx].id;
}

void CRetran::SetNextID()
{
	++currIdx;
}

BOOL CRetran::Done()
{
	if( currIdx >= ( number - 1 ) )
		return TRUE;
	return FALSE;
}

BOOL CRetran::Check()
{
	if( ( time(0) - startTime ) > 30 )
		return FALSE;
	return TRUE;
}

BYTE *CRetran::GetQueuedPacket()
{
	if( queue.IsEmpty() )
		return 0;
	return (BYTE *)queue.Remove();
}
