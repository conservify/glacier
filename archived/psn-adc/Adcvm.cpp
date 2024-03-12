// adcvm.cpp - VolksMeter Interface Board Class

#include "PSNADBoard.h"

CAdcVM::CAdcVM()
{
	noNewDataFlag = setPCTime = checkPCTime = checkPCCount = 0;
	blockConfigCount = 0;
	currLoopErrors = 0;
	rmcTimeAvg = 0.0;
	rmcTimeCount = 0;
	dacA = dacB = 0;
	memset( &boardInfo, 0, sizeof( BoardInfoVM) );
	memset( &adjTimeInfo, 0, sizeof( AdjTimeInfo ) );
	boardInfo.numConverters = 255;
}

void CAdcVM::ProcessNewPacket( BYTE *packet )
{
	PreHdr *preHdr = (PreHdr *)packet;
	
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

void CAdcVM::ProcessConfig( BYTE *packet )
{
	boardInfo = *( ( BoardInfoVM * )&packet[ sizeof( PreHdr ) ] );
	SendConfigPacket();
}

void CAdcVM::ProcessDataPacket( BYTE *newPacket )
{
	int newData = 0, i, s, numSamples;
	PreHdr *preHdr = (PreHdr *)newPacket;
	LONG diff;
				
	++packetsReceived;
		
	DataHdrV2 *dataHdr = (DataHdrV2 *)&newPacket[ sizeof( PreHdr ) ];
	BYTE *dPtr = &newPacket[ sizeof(PreHdr) + sizeof( DataHdrV2 ) ];
		
	numSamples = preHdr->len - 1 - sizeof( DataHdrV2 );
	
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
	
	if( numSamples )
		CheckUTCTime( dataHdr );
	
	if( !owner->configSent && owner->goodConfig )  {
		owner->SendMsgFmt( ADC_MSG, "Data before configuration sent - Resetting Board");
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
	if( checkPCTime )  {
		if( checkPCCount )
			--checkPCCount;
		if( !checkPCCount )  {
			owner->checkTimeFlag = TRUE;
			checkPCCount = 300;
		}
	}
		
	currHdr = *dataHdr;
	
	if( !numSamples )
		return;			
		
	if( !packetNum )  {
		memset( currSecBuffer, 0, adSecDataLen );
		MakeHeader( dataHdr );
	}
	
	if( spsRate == 5 )  {
		LONG *lTo = sps5Accum;
		int *iFrom = (int *)dPtr;
		for( i = 0; i != numChannels; i++ )
			*lTo++ += *iFrom++;
		if( ++sps5Count >= 2 )  {
			int *iTo = (int *)currSecPtr;
			for( i = 0; i != numChannels; i++ )
				*iTo++ = (int)( sps5Accum[i] / 2 ); 
			memset( sps5Accum, 0, sizeof( sps5Accum ) );
			sps5Count = 0;
			currSecPtr += adDataLen;
			if( ++packetNum >= PACKETS_PER_SEC_VM )  {
				packetNum = 0;
				++currPacketID;
				currSecPtr = currSecBuffer;
				newData = TRUE;
			}
		}
	}
	else  {
		int *iTo, data;
		BYTE *iPtr = (BYTE *)&data;
		iTo = (int *)currSecPtr;
		for( s = 0; s != numSamples; s++ )  {
			for( i = 0; i != numChannels; i++ )  {
				iPtr[3] = 0;
				iPtr[2] = *dPtr++;
				iPtr[1] = *dPtr++;
				iPtr[0] = *dPtr++;
				*iTo++ = 8388608 - data;
			}
		}			
		
		currSecPtr += adDataLen;
		if( ++packetNum >= PACKETS_PER_SEC_VM )  {
			packetNum = 0;
			++currPacketID;
			currSecPtr = currSecBuffer;
			newData = TRUE;
		}
	}
	
	if( newData )
		owner->SendQueueAdcData( ADC_AD_DATA, &dataHeader, (void *)currSecBuffer, adSecDataLen );
}

void CAdcVM::FillDataHole( DWORD diff )
{
	int cnt = diff;
	
	while( cnt-- )  {
		if( !packetNum )  {
			memset( currSecBuffer, 0, adSecDataLen );
			MakeHeaderHole( &currHdr.timeTick );
		}
		currSecPtr += adDataLen;
		if( ++packetNum >= PACKETS_PER_SEC_VM )  {
			packetNum = 0;
			++currPacketID;
			currSecPtr = currSecBuffer;
			owner->SendQueueAdcData( ADC_AD_DATA, &dataHeader, (void *)currSecBuffer, adSecDataLen );
		}
	}
}

BOOL CAdcVM::SendConfigPacket()
{
	BYTE crc, outDataPacket[128];
	int ref, timeRef, len = sizeof( ConfigInfoVM );
	ConfigInfoVM *cfg = (ConfigInfoVM *)&outDataPacket[ 2 ];
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
	
	checkUTCFlag = TRUE;
	
	memset( cfg, 0, sizeof( ConfigInfoVM ) );
	outDataPacket[0] = 0x02;
	outDataPacket[1] = 'C';
	
	cfg->numChannels = numChannels;
	cfg->sps = (BYTE)adcCfg->sampleRate;
	timeRefType = ref = owner->config.timeRefType;
	if( ref == TIME_REF_GARMIN )
		timeRef = TIME_REFV2_GARMIN;
	else
		timeRef = TIME_REFV2_USEPC;
	
	cfg->timeRefType = timeRef;
	if( adcCfg->highToLowPPS )
		cfg->flags |= HIGH_TO_LOW_PPS_FLAG;
	if( adcCfg->noPPSLedStatus )
		cfg->flags |= NO_LED_PPS_FLAG;
	
	cfg->dacA = dacA;
	cfg->dacB = dacB;
		
	GetSystemTime( &sysTm );
	currUTCTime = sysTm;
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
void CAdcVM::SendPacket( char chr )
{
	BYTE crc, outDataPacket[32];
					
	if( chr == 'a' || chr == 's' || chr == 'x' || chr == 'b' || chr == 'd' || chr == 'j' )  {
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
void CAdcVM::SetTimeDiff( LONG adjust, BOOL reset )
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

BOOL CAdcVM::GoodConfig( AdcBoardConfig2 *cfg, char *errMsg )
{
	errMsg[ 0 ] = 0;
	if( cfg->commSpeed < 4800 || cfg->commSpeed > 57600 )  {
		sprintf( errMsg, "Baud Rate (%d) must be between 4800 and 57600", cfg->commSpeed );
		return FALSE;
	}
	
	if( cfg->sampleRate != 5 && cfg->sampleRate != 10 && cfg->sampleRate != 20 && cfg->sampleRate != 25 && 
			cfg->sampleRate != 40 && cfg->sampleRate != 50 && cfg->sampleRate != 80 )  {
		sprintf( errMsg, "Sample Rate (%d) must be 5, 10, 20, 25, 40 or 80 SPS", cfg->sampleRate );
		return FALSE;
	}
	return TRUE;
}

BOOL CAdcVM::SendBoardCommand( DWORD cmd, void *data )
{
	int i;
	long l;
	
	if( data )
		i = 1;
	else
		i = 0;
	if( cmd == ADC_CMD_EXIT )  {
		noNewDataFlag = TRUE;
		owner->SendMsgFmt( ADC_MSG, "Sending Exit Command" );
		SendPacket( 'x' );
		owner->configSent = FALSE;
		return TRUE;
	}
	if( cmd == ADC_CMD_SEND_TIME_INFO )  {
		AdjTimeInfo *ai = ( AdjTimeInfo *)data;
		SendNewAdjInfo( ai->addDropTimer, ai->addTimeFlag );
		return TRUE;
	}
	if( cmd == ADC_CMD_GPS_DATA_ON_OFF )  {
		if( data )  {
			owner->SendMsgFmt(ADC_MSG, "GPS OnOff=On" );
			SendPacket('P');
		}
		else  {
			owner->SendMsgFmt(ADC_MSG, "GPS OnOff=Off" );
			SendPacket('p');
		}
		return TRUE;
	}
	if( cmd == ADC_CMD_SEND_STATUS )  {
		SendPacket( 'S' );
		return TRUE;
	}
	if( cmd == ADC_CMD_DEBUG_REF )  {
		gpsRef.debug = i;
		return TRUE;
	}
	if( cmd == ADC_CMD_SET_DAC_A )  {
		l = (long)data;
		dacA = (BYTE)l;
		return TRUE;
	}
	if( cmd == ADC_CMD_SET_DAC_B )  {
		l = (long)data;
		dacB = (BYTE)l;
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
void CAdcVM::SendCurrentTime( BOOL reset )
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

void CAdcVM::ProcessGpsData( BYTE *packet )
{
	PreHdr *preHdr = (PreHdr *)packet;
	BYTE *str;
	str = &packet[ sizeof(PreHdr) ];
	str[preHdr->len-1] = 0;
	owner->SendQueueData( ADC_GPS_DATA, str, preHdr->len-1 );
}

void CAdcVM::ProcessAdcMessage( BYTE *packet )
{
	char *str = (char *)&packet[ sizeof( PreHdr ) ];
	DWORD len = (DWORD)strlen( str );
	owner->SendQueueData( ADC_AD_MSG, (BYTE *)str, len + 1);
}
	
void CAdcVM::ProcessStatusPacket( BYTE *packet )
{	
	StatusInfoV2 *sts = (StatusInfoV2 *)&packet[ sizeof( PreHdr ) ];
	StatusInfo dllSts;
	
	memset( &dllSts, 0, sizeof( dllSts ) );
	
	dllSts.boardType = sts->boardType;
	dllSts.majorVersion = sts->majorVersion;
	dllSts.minorVersion = sts->minorVersion;
	dllSts.numChannels = sts->numChannels;
	dllSts.spsRate = (BYTE)sts->sps;
	dllSts.lockStatus = GetLockStatus();
	dllSts.crcErrors = owner->crcErrors;
	dllSts.numProcessed = dllSts.packetsRcvd = packetsReceived;
	if( IsGPSRef() )
		gpsRef.MakeTimeInfo( &dllSts.timeInfo );
	else if( timeRefType == TIME_REF_USEPC )
		pcRef.MakeTimeInfo( &dllSts.timeInfo );
	owner->SendQueueData( ADC_STATUS, (BYTE *)&dllSts, sizeof( StatusInfo ) );
}

void CAdcVM::MakeHeader( DataHdrV2 *dv2 )
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

void CAdcVM::MakeHeaderHole( ULONG *timeTick )
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

void CAdcVM::InitAdc( int sps, int numChans )
{	
	startRunTime = time(0);
	
 	spsRate = sps;
	numChannels = numChans;
	
	gpsRef.ownerVM = this;
	gpsRef.owner = owner;
	gpsRef.Init();
	
	pcRef.ownerVM = this;
	pcRef.owner = owner;
	pcRef.Init();
	
	packetsReceived = currPacketID = 0;
	currSecPtr = currSecBuffer;
	if( spsRate == 5 )
		adDataLen = ( 10 * numChannels * 4 ) / PACKETS_PER_SEC_VM;
	else
		adDataLen = ( spsRate * numChannels * 4 ) / PACKETS_PER_SEC_VM;
	adSecDataLen = adDataLen * PACKETS_PER_SEC_VM;
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
}
	
void CAdcVM::NewCheckTimeInfo( LONG offset )
{
	char str[64];
	LONG diff, absDiff;
	
	if( IsGPSRef() )  {
		if( !gpsRef.currLockSts )  {
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
		if( ( adcCfg->setPCTime == SET_PC_TIME_NORMAL ) && ( absDiff > 7200000 ) )	// 7200000 = 2H 
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
void CAdcVM::ProcessTimeInfo( BYTE *packet )
{
	WWVLocInfo *info = (WWVLocInfo *)&packet[ sizeof( PreHdr ) ];
	
	if( owner->config.timeRefType != TIME_REF_USEPC && ( checkPCTime || setPCTime ) )  {
		NewCheckTimeInfo( info->data );
		return;
	}
	if( owner->config.timeRefType == TIME_REF_USEPC )
		pcRef.NewTimeInfo( info->data );		
}

void CAdcVM::SendTimeDiffRequest( BOOL reset )
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

BYTE CAdcVM::GetLockStatus()
{
	if( IsGPSRef() )
		return gpsRef.gpsLockStatus;
	else
		return pcRef.pcLockStatus;
	return 0;
}

BOOL CAdcVM::IsGPSRef()
{
	if( timeRefType == TIME_REF_GARMIN || timeRefType == TIME_REF_MOT_NMEA || 
			timeRefType == TIME_REF_MOT_BIN )
		return TRUE;
	return FALSE;
}

void CAdcVM::CheckUTCTime( DataHdrV2 *hdr )
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
	
void CAdcVM::GetTickHMS( ULONG tick, int *hour, int *min, int *sec )
{
	tick /= 1000;
	*hour = (WORD)( tick / 3600 );
	tick %= 3600;
	*min = (WORD)( tick / 60 );
	*sec = (WORD)( tick % 60 );
}

/* 
void CAdcVM::DumpHdr( char *str, DataHdrV2 *hdr )
{
	owner->SendMsgFmt( ADC_MSG, "%s-ID=%d Tck=%d PPS=%d TOD=%d LE=%d Lck=%d Num=%d %d/%d/%d", str, 
		hdr->packetID, hdr->timeTick, hdr->ppsTick, hdr->gpsTOD, hdr->loopError, hdr->gpsLockSts,
		hdr->gpsSatNum, hdr->gpsMonth, hdr->gpsDay, hdr->gpsYear ); 
}
*/

void CAdcVM::SendNewAdjInfo( ULONG addDropTimer, BYTE addFlag )
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

void CAdcVM::AddTimeMs( ULONG *tick, SYSTEMTIME *st, DWORD ms )
{
	time_t t;
	struct tm *nt;
		
	*tick += ms;
	if( *tick >= 86400000 )  {
		*tick = *tick - 86400000;
		currUTCTime.wHour = currUTCTime.wMinute = currUTCTime.wSecond = 0;
		currUTCTime.wMilliseconds = 0;
		t = MakeLTime( currUTCTime.wYear, currUTCTime.wMonth, currUTCTime.wDay, 0, 0, 0 );
		t += ROLL_OVER_SEC;					// was 86400
		nt = gmtime( &t );
		currUTCTime.wYear = nt->tm_year + 1900;
		currUTCTime.wMonth = nt->tm_mon + 1;
		currUTCTime.wDay = 	nt->tm_mday;
	}
}

void CAdcVM::ProcessAck()
{
	ackErrors = ackTimer = 0;
}

void CAdcVM::SetRefBoardType()
{
	gpsRef.boardType = BOARD_VM;
	pcRef.boardType = BOARD_VM;
}
