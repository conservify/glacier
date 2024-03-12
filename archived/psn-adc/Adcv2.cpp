// adcv2.cpp - PSN-ADC-SERIAL V2 Board Class

#include "PSNADBoard.h"

CAdcV2::CAdcV2()
{
	noNewDataFlag = setPCTime = checkPCTime = checkPCCount = 0;
	blockConfigCount = 0;
	currLoopErrors = 0;
	rmcTimeAvg = 0.0;
	rmcTimeCount = 0;
	memset( &adjTimeInfo, 0, sizeof( AdjTimeInfo) );
	memset( &gpsConfig, 0, sizeof( gpsConfig ) );
}

void CAdcV2::ProcessNewPacket( BYTE *packet )
{
	PreHdr *preHdr = (PreHdr *)packet;
	
//	if( preHdr->type != 'D' )
//		owner->SendMsgFmt( ADC_MSG, "new type=%c", preHdr->type  );
	
	if( noNewDataFlag )
		return;

	if( blockConfigCount )
		--blockConfigCount;
	
	// process the different incoming packet types
	switch( preHdr->type )  {
		case 'C':		ProcessConfig( packet ); break;
		case 'g':		ProcessGpsData( packet ); break;
		case 'D':		ProcessDataPacket( packet ); break;
		case 'L':		ProcessAdcMessage( packet );break;
		case 'S':		ProcessStatusPacket( packet ); break;
		case 'w':		ProcessTimeInfo( packet ); break;
		case 'a':		ProcessAck(); break;
		default:		owner->SendMsgFmt( ADC_MSG, "Unknown packet type %c (0x%x)", 
							preHdr->type, preHdr->type );
	}
}

void CAdcV2::ProcessConfig( BYTE *packet )
{
	adjTimeInfo = *( ( AdjTimeInfo * )&packet[ sizeof( PreHdr ) ] );
	SendConfigPacket();
}

void CAdcV2::ProcessDataPacket( BYTE *newPacket )
{
	int newData = 0, i;
	LONG diff;
				
	++packetsReceived;
		
	DataHdrV2 *dataHdr = (DataHdrV2 *)&newPacket[ sizeof( PreHdr ) ];
	BYTE *dPtr = &newPacket[ sizeof(PreHdr) + sizeof( DataHdrV2 ) ];
		
	if( ackTimer )  {
		--ackTimer;
		if( !ackTimer )  {
			if( ++ackErrors >= 3 )  {
				owner->SendMsgFmt( ADC_ERROR, "ADC Board did not Acknowledge Last Command (%c) !!! Error Count = %d", 
					ackTimerCommand, ackErrors );
				owner->ResetBoard();
				return;
			}
			else
				owner->SendMsgFmt( ADC_MSG, "ADC Board did not Acknowledge Last Command (%c) !!! Error Count = %d", 
					ackTimerCommand, ackErrors );
		}
	}
	if( noPacketCount )  {
		--noPacketCount;
		return;
	}
	
	CheckUTCTime( dataHdr );
	
	if( !owner->configSent && owner->goodConfig )  {
		owner->SendMsgFmt( ADC_MSG, "Data before configuration sent - Resetting Board");
		InitAdc( owner->config.sampleRate, owner->config.numberChannels );
		owner->ResetBoard();
		noPacketCount = 20;
		return;
	}
	
	if( firstPacketCount )  {
		--firstPacketCount;
		if( firstPacketCount )
			return;
		owner->sendTimeFlag = TRUE;
		sendStatusCount = 5;
		return;
	}
		
	if( sendStatusCount )  {
		--sendStatusCount;
		if( sendStatusCount )
			return;
		SendPacket( 'S' );
		noPacketCount = 5;
		return;
	}
	
	// Check for bad ID error, ID should always be higher then last
	if( testPacketID && ( dataHdr->packetID != (testPacketID+1) ) )  {
		if( dataHdr->packetID <= testPacketID )  {
			owner->SendMsgFmt( ADC_ERROR, "Packet ID Error Current=%d Last=%d",
				 dataHdr->packetID, testPacketID  );
			testPacketID = dataHdr->packetID;
			InitAdc( owner->config.sampleRate, owner->config.numberChannels );
			owner->ResetBoard();
			return;
		}
		diff = dataHdr->packetID - testPacketID;
		owner->SendMsgFmt( ADC_MSG, "Data Loss Error Time=%d.%d seconds", 
			diff / 10, diff % 10);
		if( diff <= 50 )
			FillDataHole( diff );
		else if( packetNum )  {
			owner->SendQueueAdcData( ADC_AD_DATA, &dataHeader, (void *)currSecBuffer, 
				adSecDataLen );
			packetNum = 0;
			++currPacketID;
			currSecPtr = currSecBuffer;
		}
	}
	testPacketID = dataHdr->packetID;
		
	owner->goodData = TRUE;
		
	if( timeRefType == TIME_REF_USEPC )
		pcRef.NewPacket();
	else if( IsGPSRef() )
		gpsRef.NewPacket( dataHdr );
	else if( timeRefType == TIME_REF_WWV )
		wwvRef.NewPacket();
	else if( timeRefType == TIME_REF_WWVB )
		wwvbRef.NewPacket();
		
	if( checkPCTime )  {
		if( checkPCCount )
			--checkPCCount;
		if( !checkPCCount )  {
			owner->checkTimeFlag = TRUE;
			checkPCCount = 300;
		}
	}
		
	currHdr = *dataHdr;
	if( !packetNum )  {
		memset( currSecBuffer, 0, adSecDataLen );
		MakeHeader( dataHdr );
	}
	
	if( spsRate == 5 )  {
		LONG *lTo = sps5Accum;
		short *sFrom = (short *)dPtr;
		int numChan = numChannels;
		for( i = 0; i != numChan; i++ )
			*lTo++ += *sFrom++;
		if( ++sps5Count >= 2 )  {
			short *sTo = (short *)currSecPtr;
			for( i = 0; i != numChan; i++ )  {
				*sTo = (short)( sps5Accum[i] / 2 ); 
				if( make12Bit[i] )
					*sTo /= 4;
				++sTo;
			}
			memset( sps5Accum, 0, sizeof( sps5Accum ) );
			sps5Count = 0;
			currSecPtr += adDataLen;
			if( ++packetNum >= ( PACKETS_PER_SEC / 2 ) )  {
				packetNum = 0;
				++currPacketID;
				currSecPtr = currSecBuffer;
				newData = TRUE;
			}
		}
	}
	else  {
		if( make12Flag )  {
			int samples = adDataLen / 2;
			int idx = 0, numChan = numChannels;
			short *to = (short *)currSecPtr, *from = (short *)dPtr;
			while( samples-- )  {
				if( make12Bit[ idx ] )
					*to++ = *from++ / 4;
				else
					*to++ = *from++;
				if( ++idx >= numChan )
					idx = 0;
			}
		}
		else
			memcpy( currSecPtr, dPtr, adDataLen );
		
		currSecPtr += adDataLen;
		if( ++packetNum >= PACKETS_PER_SEC )  {
			packetNum = 0;
			++currPacketID;
			currSecPtr = currSecBuffer;
			newData = TRUE;
		}
	}
	
	if( newData )
		owner->SendQueueAdcData( ADC_AD_DATA, &dataHeader, (void *)currSecBuffer, adSecDataLen );
}

void CAdcV2::FillDataHole( DWORD diff )
{
	int cnt = diff;
	
	while( cnt-- )  {
		if( !packetNum )  {
			memset( currSecBuffer, 0, adSecDataLen );
			MakeHeaderHole( &currHdr.timeTick );
		}
		currSecPtr += adDataLen;
		if( ++packetNum >= PACKETS_PER_SEC )  {
			packetNum = 0;
			++currPacketID;
			currSecPtr = currSecBuffer;
			owner->SendQueueAdcData( ADC_AD_DATA, &dataHeader, (void *)currSecBuffer, adSecDataLen );
		}
	}
}

BOOL CAdcV2::SendConfigPacket()
{
	BYTE crc, outDataPacket[128];
	int ref, timeRef, len = sizeof( ConfigInfoV2 );
	ConfigInfoV2 *cfg = (ConfigInfoV2 *)&outDataPacket[ 2 ];
	AdcBoardConfig2 *adcCfg = &owner->config;
	SYSTEMTIME sysTm;
	char errMsg[ 128 ];
		
	if( blockConfigCount )
		return TRUE;

	if( !owner->goodConfig )  {
		owner->SendMsgFmt( ADC_MSG, "SendConfigPacket Error No Config Info");
		return FALSE;
	}
	if( !GoodConfig( adcCfg, errMsg ) )  {
		owner->SendMsgFmt( ADC_MSG, "Configuration Error = %s", errMsg );
		return FALSE;
	}
	
	setPCTime = checkPCTime = 0;
	checkPCCount = 300;
	if( adcCfg->checkPCTime && ( adcCfg->timeRefType != TIME_REF_USEPC )  )
		checkPCTime = TRUE;
	if( adcCfg->setPCTime && ( adcCfg->timeRefType != TIME_REF_USEPC ) )
		checkPCTime = setPCTime = TRUE;
	
	owner->SendMsgFmt( ADC_MSG, "Sending Configuration Information" );
	
	InitAdc( adcCfg->sampleRate, adcCfg->numberChannels );
	
	UpdateMake12( adcCfg->mode12BitFlags );
	
	GetSystemTime( &sysTm );
	currUTCTime = sysTm;
	checkUTCFlag = TRUE;
	
	memset( cfg, 0, sizeof( ConfigInfoV2 ) );
	outDataPacket[0] = 0x02;
	outDataPacket[1] = 'C';
	cfg->numChannels = (BYTE)adcCfg->numberChannels;
	if( adcCfg->sampleRate == 5 )
		cfg->sps = 10;
	else
		cfg->sps = adcCfg->sampleRate;
	timeRefType = ref = owner->config.timeRefType;
	if( ref == TIME_REF_SKG )
		timeRef = TIME_REFV2_SKG;
	else if( ref == TIME_REF_GARMIN )
		timeRef = TIME_REFV2_GARMIN;
	else if( ref == TIME_REF_MOT_NMEA )
		timeRef = TIME_REFV2_MOT_NMEA;
	else if( ref == TIME_REF_MOT_BIN )
		timeRef = TIME_REFV2_MOT_BIN;
	else if( ref == TIME_REF_WWV )
		timeRef = TIME_REFV2_WWV;
	else if( ref == TIME_REF_WWVB )
		timeRef = TIME_REFV2_WWVB;
	else if( ref == TIME_REF_4800 )
		timeRef = TIME_REFV2_4800;
	else if( ref == TIME_REF_9600 )
		timeRef = TIME_REFV2_9600;
	else
		timeRef = TIME_REFV2_USEPC;
	
	cfg->timeRefType = timeRef;
	if( adcCfg->highToLowPPS )
		cfg->flags |= HIGH_TO_LOW_PPS_FLAG;
	if( adcCfg->noPPSLedStatus )
		cfg->flags |= NO_LED_PPS_FLAG;
	cfg->flags |= FG_WATCHDOG_CHECK;
	
	if( owner->adcBoardType == BOARD_V3 && m_statusInfo.majorVersion >= 2 )  {
		if( gpsConfig.only2DMode )
			cfg->flags |= FG_2D_ONLY;
		if( gpsConfig.enableWAAS )
			cfg->flags |= FG_WAAS_ON;
	}
		
	cfg->timeTick = ( ( ( sysTm.wHour * 3600 ) + ( sysTm.wMinute * 60 ) + sysTm.wSecond ) * 1000 ) 
		+ sysTm.wMilliseconds;
	currUTCTick = cfg->timeTick;
	
	crc = owner->CalcCRC( &outDataPacket[1], (short)( len + 1 ) );
	outDataPacket[ len + 2 ] = crc;
	outDataPacket[ len + 3 ] = 0x03;
	owner->SendCommData( outDataPacket, len + 4 );
	owner->configSent = TRUE;
	blockConfigCount = 20;
	testPacketID = 0;
	return TRUE;
}

/* Send a one character command to the A/D Board */
void CAdcV2::SendPacket( char chr )
{
	BYTE crc, outDataPacket[32];
					
	
	if( chr == 'a' || chr == 's' || chr == 'd' || chr == 'j' )  {
		ackTimer = 20;
		ackTimerCommand = chr;
	}
	outDataPacket[0] = 0x02;
	outDataPacket[1] = chr;
	crc = owner->CalcCRC( &outDataPacket[1], (short)1 );
	outDataPacket[2] = crc;
	outDataPacket[3] = 0x03;
	owner->SendCommData( outDataPacket, 4 );
}

/* Sends a command to adjust the time accumulator on the A/D board */
void CAdcV2::SetTimeDiff( LONG adjust, BOOL reset )
{
	BYTE outDataPacket[ 128 ], crc;
	TimeDiffInfo *tm = ( TimeDiffInfo * )&outDataPacket[ 2 ];
	int len = sizeof( TimeDiffInfo );
	
	tm->adjustTime = adjust;
	if( reset )
		tm->reset = 1;
	else
		tm->reset = 0;
	outDataPacket[0] = 0x02;
	outDataPacket[1] = 'd';
	crc = owner->CalcCRC( &outDataPacket[1], (short)( len + 1 ) );
	outDataPacket[len + 2] = crc;
	outDataPacket[len + 3] = 0x03;
	owner->SendCommData(outDataPacket, len + 4 );
}

BOOL CAdcV2::IsValidSpsRate( WORD sps, WORD numChannels, char *errMsg )
{
	if( numChannels > 8 )  {
		sprintf( errMsg, "Number Channels (%d) Error, Max = 8 ", numChannels);
		return FALSE;
	}
		
	if( owner->adcBoardType == BOARD_V3 )  {
		if( sps == 500 && numChannels > 4 )  {
			sprintf( errMsg, "Sample Rate (%d) and Number Channels (%d) Error, Channels must be 4 or less at 500 SPS", 
				sps, numChannels);
			return FALSE;
		}
		if( sps == 10 || sps == 20 || sps == 50 || sps == 100 || sps == 200 || sps == 250 || sps == 500 )
			return TRUE;
		sprintf( errMsg, "Sample Rate (%d) Error, must be 10, 20, 50, 100, 200, 250 or 500", sps );
		return FALSE;
	}
	
	if( sps == 10 || sps == 20 || sps == 50 || sps == 100 || sps == 200 )
		return TRUE;
	sprintf( errMsg, "Sample Rate (%d) Error, must be 10, 20, 50, 100 or 200", sps );
	return FALSE;
}

BOOL CAdcV2::GoodConfig( AdcBoardConfig2 *cfg, char *errMsg )
{
	errMsg[ 0 ] = 0;
	if( cfg->commSpeed < 9600 || cfg->commSpeed > 57600 )  {
		sprintf( errMsg, "Baud Rate (%d) must be between 9600 and 57600", cfg->commSpeed );
		return FALSE;
	}
	return IsValidSpsRate( cfg->sampleRate, cfg->numberChannels, errMsg );
}

BOOL CAdcV2::SendBoardCommand( DWORD cmd, void *data )
{
	BYTE b;
	int i;
		
	if( cmd == ADC_CMD_EXIT )  {
		noNewDataFlag = TRUE;
		owner->SendMsgFmt( ADC_MSG, "Sending Exit Command" );
		SendPacket( 'x' );
		owner->SendMsgFmt( ADC_MSG, "clear configsent flag" );
		owner->configSent = FALSE;
		return TRUE;
	}
	if( cmd == ADC_CMD_SEND_TIME_INFO )  {
		AdjTimeInfo *ai = ( AdjTimeInfo *)data;
		SendNewAdjInfo( ai->addDropTimer, ai->addTimeFlag );
		return TRUE;
	}
	if( cmd == ADC_CMD_GPS_DATA_ON_OFF )  {
		if( data )
			b = 1;
		else
			b = 0;
		owner->SendMsgFmt(ADC_MSG, "GPS OnOff %d", b );
		if( data )
			SendPacket('P');
		else
			SendPacket('p');
		return TRUE;
	}
	if( cmd == ADC_CMD_SEND_STATUS )  {
		SendPacket( 'S' );
		return TRUE;
	}
	if( cmd == ADC_CMD_DEBUG_REF )  {
		if( data )
			i = 1;
		else
			i = 0;
		gpsRef.debug = i;
		wwvRef.debug = i;
		wwvbRef.debug = i;
		return TRUE;
	}
	if( cmd == ADC_CMD_RESET_GPS )  {
		if( IsGPSRef() )  {
			SendPacket( 'g' );
			gpsRef.ResetRef( TRUE );
			owner->SendMsgFmt(ADC_MSG, "GPS Reset Command Sent to A/D Board" );
			currLoopErrors = 0;			
			return TRUE;
		}
		return FALSE;
	}

	if( cmd == ADC_CMD_GPS_CONFIG )  {
		memcpy( &gpsConfig, data, sizeof( gpsConfig ) );
		return TRUE;
	}

	if( cmd == ADC_CMD_FORCE_TIME_TEST )  {
		SendPacket( 'i' );
		owner->SendMsgFmt(ADC_MSG, "GPS Force Test" );
		checkPCAvg = checkPCAvgCount = 0;
		checkPCCount = 300;
		return TRUE;
	}
	
	return FALSE;
}

/* Send the current time to the A/D board */
void CAdcV2::SendCurrentTime( BOOL reset )
{
	BYTE outDataPacket[ 128 ], crc;
	TimeInfoV2 *tm = (TimeInfoV2 *)&outDataPacket[ 2 ];
	int len = sizeof( TimeInfoV2 );
	SYSTEMTIME sTm;
		
	if( reset )
		tm->reset = 1;
	else
		tm->reset = 0;
	outDataPacket[0] = 0x02;
	outDataPacket[1] = 'T';
	
#ifdef WIN32_64
	HANDLE process;
	DWORD priority;
	
	process = GetCurrentProcess();
	priority = GetPriorityClass( process );
	if( ! priority )
		owner->SendMsgFmt( ADC_MSG, "Can't get process priority");
	else
		SetPriorityClass( process, REALTIME_PRIORITY_CLASS );
#endif
	
	GetSystemTime( &sTm );
	tm->timeTick = ( ( ( sTm.wHour * 3600 ) + ( sTm.wMinute * 60 ) + sTm.wSecond ) * 1000 ) 
			+ sTm.wMilliseconds;
	crc = owner->CalcCRC( &outDataPacket[1], (short)(len + 1));
	outDataPacket[len + 2] = crc;
	outDataPacket[len + 3] = 0x03;
	owner->SendCommData( outDataPacket, len + 4 );
	
#ifdef WIN32_64
	if( priority )
		SetPriorityClass( process, priority );
#endif
}

void CAdcV2::ProcessGpsData( BYTE *packet )
{
	PreHdr *preHdr = (PreHdr *)packet;
	BYTE *str;
	str = &packet[ sizeof(PreHdr) ];
	str[preHdr->len-1] = 0;
	owner->SendQueueData( ADC_GPS_DATA, str, preHdr->len-1 );
}

void CAdcV2::ProcessAdcMessage( BYTE *packet )
{
	char *str = (char *)&packet[ sizeof( PreHdr ) ];
	DWORD len = (DWORD)strlen( str );
	owner->SendQueueData( ADC_AD_MSG, (BYTE *)str, len + 1);
}
	
void CAdcV2::ProcessStatusPacket( BYTE *packet )
{	
	StatusInfoV2 *sts = (StatusInfoV2 *)&packet[ sizeof( PreHdr ) ];
	StatusInfo dllSts;
	
	m_statusInfo = *sts;
	
	if( firstStatPacket )  {
		owner->SendMsgFmt( ADC_MSG, "ADC Board Firmware Version: %d.%d", 
			m_statusInfo.majorVersion, m_statusInfo.minorVersion );
		firstStatPacket = 0;
	}
		
	memset( &dllSts, 0, sizeof( dllSts ) );
	
	dllSts.boardType = sts->boardType;
	dllSts.majorVersion = sts->majorVersion;
	dllSts.minorVersion = sts->minorVersion;
	dllSts.numChannels = sts->numChannels;
	dllSts.spsRate = sts->sps;
	dllSts.lockStatus = GetLockStatus();
	dllSts.crcErrors = owner->crcErrors;
	dllSts.numProcessed = dllSts.packetsRcvd = packetsReceived;
	if( IsGPSRef() )
		gpsRef.MakeTimeInfo( &dllSts.timeInfo );
	else if( timeRefType == TIME_REF_USEPC )
		pcRef.MakeTimeInfo( &dllSts.timeInfo );
	else if( timeRefType == TIME_REF_WWV )
		wwvRef.MakeTimeInfo( &dllSts.timeInfo );
	else if( timeRefType == TIME_REF_WWVB )
		wwvbRef.MakeTimeInfo( &dllSts.timeInfo );
	owner->SendQueueData( ADC_STATUS, (BYTE *)&dllSts, sizeof( StatusInfo ) );
}

void CAdcV2::UpdateMake12( DWORD flags )
{
	BYTE mask = 0x01;
	
	make12Flag = 0;
	memset( make12Bit, 0, sizeof( make12Bit ) );
	for( int i = 0; i != MAX_ADC_CHANNELS; i++ )  {
		if( flags & mask )  {
			make12Bit[i] = TRUE;
			make12Flag = TRUE;
		}
		mask <<= 1;
	}
}

void CAdcV2::MakeHeader( DataHdrV2 *dv2 )
{
	DataHeader *dh = &dataHeader;
	SYSTEMTIME *st = &dh->packetTime;
	ULONG tick = dv2->timeTick;
	
	*st = currUTCTime;
	st->wDayOfWeek = 0;
	st->wMilliseconds = (WORD)( tick % 1000 );
	tick /= 1000;
	st->wHour = (WORD)( tick / 3600 );
	tick %= 3600;
	st->wMinute = (WORD)( tick / 60 );
	st->wSecond = (WORD)( tick % 60 );
	dh->packetID = currPacketID;
	dh->timeRefStatus = GetLockStatus();
	dh->flags = 0x80;
}

void CAdcV2::MakeHeaderHole( ULONG *timeTick )
{
	DataHeader *dh = &dataHeader;
	SYSTEMTIME *st = &dh->packetTime;
	ULONG tick;
	
	AddTimeMs( timeTick, &currUTCTime, 1000 );
	tick = *timeTick;	
	*st = currUTCTime;
	st->wDayOfWeek = 0;
	st->wMilliseconds = (WORD)( tick % 1000 );
	tick /= 1000;
	st->wHour = (WORD)( tick / 3600 );
	tick %= 3600;
	st->wMinute = (WORD)( tick / 60 );
	st->wSecond = (WORD)( tick % 60 );
	dh->packetID = currPacketID;
	dh->timeRefStatus = GetLockStatus();
	dh->flags = 0x80;
}

void CAdcV2::InitAdc( int sps, int numChans )
{	
	startRunTime = time(0);
	
 	spsRate = sps;
	numChannels = numChans;
	
	gpsRef.ownerV2 = this;
	gpsRef.owner = owner;
	gpsRef.Init();
	
	pcRef.ownerV2 = this;
	pcRef.owner = owner;
	pcRef.Init();
	
	wwvRef.ownerV2 = this;
	wwvRef.owner = owner;
	wwvRef.Init();
	
	wwvbRef.ownerV2 = this;
	wwvbRef.owner = owner;
	wwvbRef.Init();
	
	packetsReceived = currPacketID = 0;
	currSecPtr = currSecBuffer;
	if( spsRate == 5 )
		adDataLen = ( 10 * numChannels * 2 ) / PACKETS_PER_SEC;
	else
		adDataLen = ( spsRate * numChannels * 2 ) / PACKETS_PER_SEC;
	adSecDataLen = adDataLen * 10;
	
	memset( sps5Accum, 0, sizeof( sps5Accum ) );
	sps5Count = 0;
	ackTimer = ackErrors = 0;
	memset( &currHdr, 0, sizeof( currHdr ) );
	
	rmcLocked = 0;
	
	testPacketID = 0;
	packetNum = 0;
	noNewDataFlag = 0;
	sendStatusCount = noPacketCount = 0;
	firstPacketCount = 10;
	checkPCAvg = checkPCAvgCount = 0;
	checkUTCFlag = TRUE;
	GetSystemTime( &currUTCTime );
	firstStatPacket = TRUE;
}
	
void CAdcV2::NewCheckTimeInfo( LONG offset )
{
	char str[64];
	LONG diff, absDiff;
	
	if( IsGPSRef() )  {
		if( !gpsRef.currLockSts )  {
			checkPCAvg = checkPCAvgCount = 0;
			return;
		}
	}
	else if( timeRefType == TIME_REF_WWV )  {
		if( wwvRef.timeState != 2 )  {
			checkPCAvg = checkPCAvgCount = 0;
			return;
		}
	}	
	else if( timeRefType == TIME_REF_WWVB )  {
		if( !wwvbRef.lockStatus )  {
			checkPCAvg = checkPCAvgCount = 0;
			return;
		}
	}	
	else  {
		checkPCAvg = checkPCAvgCount = 0;
		return;
	}	

	checkPCAvg += offset;
	++checkPCAvgCount;
	
	if( gpsRef.debug )
		owner->SendMsgFmt( ADC_MSG, "Time Check Diff = %dms Count = %d", offset, checkPCAvgCount );
	
	if( checkPCAvgCount < 10 )
		return;		
				  
	AdcBoardConfig2 *adcCfg = &owner->config;
	diff = checkPCAvg / checkPCAvgCount;
	if( gpsRef.debug )
		owner->SendMsgFmt( ADC_MSG, "Time Check Avg = %dms", diff );
	
	absDiff = abs( diff );
	checkPCAvg = checkPCAvgCount = 0;
	if( diff < 0 )
		sprintf( str, "-%d.%03d", absDiff / 1000, absDiff % 1000 );
	else
		sprintf( str, "%d.%03d", absDiff / 1000, absDiff % 1000 );
	if( setPCTime && ( absDiff > 50 ) )  {
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

/* WWV and Local PC time updates come here */
void CAdcV2::ProcessTimeInfo( BYTE *packet )
{
	WWVLocInfo *info = (WWVLocInfo *)&packet[ sizeof( PreHdr ) ];
	
	if( owner->config.timeRefType == TIME_REF_WWV && info->msCount )  {
		wwvRef.NewTimeInfo( info->data, info->msCount );		
		return;
	}
	if( owner->config.timeRefType == TIME_REF_WWVB && info->msCount )  {
		wwvbRef.NewTimeInfo( info->data, info->msCount );		
		return;
	}
	if( owner->config.timeRefType != TIME_REF_USEPC && ( checkPCTime || setPCTime ) )  {
		NewCheckTimeInfo( info->data );
		return;
	}
	if( owner->config.timeRefType == TIME_REF_USEPC )
		pcRef.NewTimeInfo( info->data );		
}

void CAdcV2::SendTimeDiffRequest( BOOL reset )
{
	BYTE outDataPacket[ 128 ], crc;
	TimeInfoV2 *tm = (TimeInfoV2 *)&outDataPacket[ 2 ];
	int len = sizeof( TimeInfoV2 );
	SYSTEMTIME sTm;

	tm->reset = 0;
	outDataPacket[0] = 0x02;
	outDataPacket[1] = 'D';
	
#ifdef WIN32_64	
	DWORD priority;
	HANDLE process;
	
	process = GetCurrentProcess();
	priority = GetPriorityClass( process );
	if( ! priority )
		owner->SendMsgFmt( ADC_MSG, "Can't get process priority");
	else
		SetPriorityClass( process, REALTIME_PRIORITY_CLASS );
#endif
	
	GetSystemTime( &sTm );
	tm->timeTick = ( ( ( sTm.wHour * 3600 ) + ( sTm.wMinute * 60 ) + sTm.wSecond ) * 1000 ) 
		+ sTm.wMilliseconds;
	crc = owner->CalcCRC( &outDataPacket[1], (short)(len + 1));
	outDataPacket[len + 2] = crc;
	outDataPacket[len + 3] = 0x03;
	owner->SendCommData(outDataPacket, len + 4 );
#ifdef WIN32_64
	if( priority )
		SetPriorityClass( process, priority );
#endif
}

BYTE CAdcV2::GetLockStatus()
{
	if( IsGPSRef() )
		return gpsRef.gpsLockStatus;
	else if( timeRefType == TIME_REF_WWV )
		return wwvRef.wwvLockStatus;
	else if( timeRefType == TIME_REF_WWVB )
		return wwvbRef.lockStatus;
	else
		return pcRef.pcLockStatus;
	return 0;
}

BOOL CAdcV2::IsGPSRef()
{
	if( timeRefType == TIME_REF_GARMIN || timeRefType == TIME_REF_MOT_NMEA || 
			timeRefType == TIME_REF_MOT_BIN || timeRefType == TIME_REF_SKG || 
			timeRefType == TIME_REF_4800 || timeRefType == TIME_REF_9600 )
		return TRUE;
	return FALSE;
}

void CAdcV2::CheckUTCTime( DataHdrV2 *hdr )
{
	int newHour, newMin, newSec, lastHour, lastMin, lastSec;
	SYSTEMTIME old;
	struct tm *nt;
	time_t t;
	
	if( IsGPSRef() && hdr->gpsLockSts != '0' && hdr->gpsDay )  {
		if( hdr->gpsDay > 31 || hdr->gpsMonth > 12 || hdr->gpsYear > 80 )
			owner->SendMsgFmt( ADC_MSG, "GPS - GPS Month, Day, or Year Error %d %d %d", 
				hdr->gpsMonth, hdr->gpsDay, hdr->gpsYear );
	}
	
	if( checkUTCFlag && IsGPSRef() && hdr->gpsLockSts != '0' && hdr->gpsDay &&
			hdr->gpsDay <= 31 && hdr->gpsMonth <= 12 && hdr->gpsYear < 80 )  {
		checkUTCFlag = 0;
		currUTCTime.wDay = hdr->gpsDay;
		currUTCTime.wMonth = hdr->gpsMonth;
		currUTCTime.wYear = hdr->gpsYear + 2000;
		currUTCTick = hdr->timeTick;
		lastHdr = *hdr;
		return;
	}
		
	if( hdr->timeTick >= currUTCTick )  {
		currUTCTick = hdr->timeTick;
		lastHdr = *hdr;
		return;
	}
		
	GetTickHMS( hdr->timeTick, &newHour, &newMin, &newSec );
	GetTickHMS( currUTCTick, &lastHour, &lastMin, &lastSec );
		
	if( lastHour == 23 && !newHour )  {
		old = currUTCTime;
		currUTCTime.wHour = currUTCTime.wMinute = currUTCTime.wSecond = 0;
		currUTCTime.wMilliseconds = 0;
		t = MakeLTime( currUTCTime.wYear, currUTCTime.wMonth, currUTCTime.wDay, 0, 0, 0 );
		t += ROLL_OVER_SEC;					// was 86400
		nt = gmtime( &t );
		currUTCTime.wYear = nt->tm_year + 1900;
		currUTCTime.wMonth = nt->tm_mon + 1;
		currUTCTime.wDay = 	nt->tm_mday;
	}
	else  {
		if( IsGPSRef() && hdr->gpsLockSts != '0' && hdr->gpsDay && 
				hdr->gpsDay <= 31 && hdr->gpsMonth <= 12 && hdr->gpsYear < 80 ) {
			currUTCTime.wDay = hdr->gpsDay;
			currUTCTime.wMonth = hdr->gpsMonth;
			currUTCTime.wYear = hdr->gpsYear + 2000;
		}
		else
			GetSystemTime( &currUTCTime );
	}
	currUTCTick = hdr->timeTick;
}
	
void CAdcV2::GetTickHMS( ULONG tick, int *hour, int *min, int *sec )
{
	tick /= 1000;
	*hour = (WORD)( tick / 3600 );
	tick %= 3600;
	*min = (WORD)( tick / 60 );
	*sec = (WORD)( tick % 60 );
}

/* 
void CAdcV2::DumpHdr( char *str, DataHdrV2 *hdr )
{
	owner->SendMsgFmt( ADC_MSG, "%s-ID=%d Tck=%d PPS=%d TOD=%d LE=%d Lck=%d Num=%d %d/%d/%d", str, 
		hdr->packetID, hdr->timeTick, hdr->ppsTick, hdr->gpsTOD, hdr->loopError, hdr->gpsLockSts,
		hdr->gpsSatNum, hdr->gpsMonth, hdr->gpsDay, hdr->gpsYear ); 
}
*/

void CAdcV2::SendNewAdjInfo( ULONG addDropTimer, BYTE addFlag )
{
	BYTE outDataPacket[ 128 ], crc;
	AdjTimeInfo *at = (AdjTimeInfo *)&outDataPacket[ 2 ];
	at->addDropTimer = addDropTimer;
	at->addTimeFlag = addFlag;
	int len = sizeof( AdjTimeInfo );
	outDataPacket[0] = 0x02;
	outDataPacket[1] = 'j';
	crc = owner->CalcCRC( &outDataPacket[1], (short)(len + 1));
	outDataPacket[len + 2] = crc;
	outDataPacket[len + 3] = 0x03;
	owner->SendCommData(outDataPacket, len + 4 );
	owner->SendMsgFmt( ADC_MSG, "Sent Time Info to Board AddMode=%d Timer=%d", addFlag, addDropTimer );
}

void CAdcV2::AddTimeMs( ULONG *tick, SYSTEMTIME *st, DWORD ms )
{
	time_t t;
	struct tm *nt;
		
	*tick += ms;
	if( *tick >= 86400000 )  {
		*tick = *tick - 86400000;
		currUTCTime.wHour = currUTCTime.wMinute = currUTCTime.wSecond = 0;
		currUTCTime.wMilliseconds = 0;
		t = MakeLTime( currUTCTime.wYear, currUTCTime.wMonth, currUTCTime.wDay, 0, 0, 0 );
		t += ROLL_OVER_SEC; 			 // was 86400
		nt = gmtime( &t );
		currUTCTime.wYear = nt->tm_year + 1900;
		currUTCTime.wMonth = nt->tm_mon + 1;
		currUTCTime.wDay = 	nt->tm_mday;
	}
}

void CAdcV2::ProcessAck()
{
	ackErrors = ackTimer = 0;
}

void CAdcV2::SetRefBoardType( int type )
{
	gpsRef.boardType = pcRef.boardType = type;
}
