// adcsdr24.cpp - Sdr24 ADC Board Class

#include "PSNADBoard.h"

CAdcSdr24::CAdcSdr24()
{
	noNewDataFlag = setPCTime = checkPCTime = checkPCCount = 0;
	blockConfigCount = 0;
	currLoopErrors = 0;
	rmcTimeAvg = 0.0;
	rmcTimeCount = 0;
	memset( &boardInfo, 0, sizeof( BoardInfoSdr24) );
	memset( &adjTimeInfo, 0, sizeof( AdjTimeInfo ) );
	memset( adcGainFlags, 0, sizeof( adcGainFlags ) );
	boardInfo.modeNumConverters = 255;
	memset( &gpsConfig, 0, sizeof( gpsConfig ) );
	currUTCTick = 0;
	needsResetFlag = 0;
}

void CAdcSdr24::ProcessNewPacket( BYTE *packet )
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

void CAdcSdr24::ProcessConfig( BYTE *packet )
{
	boardInfo = *( ( BoardInfoSdr24 * )&packet[ sizeof( PreHdr ) ] );
	owner->SendMsgFmt( ADC_MSG, "ADC Board Info: Channels:%d GoodFlags:0x%02x NeedsRst:%d", 
		boardInfo.modeNumConverters & 0x0f, boardInfo.goodConverterFlags, needsResetFlag );
	if( needsResetFlag )  {
		needsResetFlag = 0;
		owner->SendMsgFmt( ADC_MSG, "Resetting ADC Board" );
		owner->ResetBoard();  // reset the board and wait for next send conf command
		return;
	}
	SendConfigPacket();
}

// Converts the tick time to milliseconds 
void CAdcSdr24::ConvertTimeToMs( ULONG *tick )
{
	ULONG ms = *tick % 1600;
	ULONG sec = ( *tick / 1600 ) * 1000;
	sec += (ULONG)( (double)ms * 0.625   );
	*tick = sec;
}

void CAdcSdr24::MakeV2Hdr( DataHdrV2 *v2Hdr, DataHdrSdr24 *dataHdr )
{
	v2Hdr->packetID = dataHdr->packetID; 
	v2Hdr->timeTick = dataHdr->timeTick;
	v2Hdr->ppsTick = dataHdr->ppsTick;
	v2Hdr->gpsTOD = dataHdr->gpsTOD;
	v2Hdr->gpsLockSts = dataHdr->gpsLockSts;
	v2Hdr->gpsSatNum = dataHdr->gpsSatNum;
	v2Hdr->gpsMonth = dataHdr->gpsMonth;
	v2Hdr->gpsDay = dataHdr->gpsDay;
	v2Hdr->gpsYear = dataHdr->gpsYear;
}

void CAdcSdr24::ProcessDataPacket( BYTE *newPacket )
{
	int i, s, numSamples, iData;
	PreHdr *preHdr = (PreHdr *)newPacket;
	LONG diff;
	BYTE *iPtr = (BYTE *)&iData;
						
	++packetsReceived;
	
	if( !needsResetFlag &&  ( packetsReceived > 10 ) )
		needsResetFlag = 1;

	DataHdrSdr24 *dataHdr = (DataHdrSdr24 *)&newPacket[ sizeof( PreHdr ) ];
		
	ConvertTimeToMs( &dataHdr->timeTick );	
	ConvertTimeToMs( &dataHdr->ppsTick );	
	
//	char todStr[ 64 ], ppsStr[ 64 ];
//	MakeTimeStr( todStr, dataHdr->gpsTOD );
//	MakeTimeTickStr( ppsStr, dataHdr->ppsTick );
//	owner->SendMsgFmt( ADC_MSG, "tod=%s pps=%s", todStr, ppsStr );
	
	numSamples = ( preHdr->len - 1 - sizeof( DataHdrSdr24 ) ) / 3;

	if( ackTimer )  {
		--ackTimer;
		if( !ackTimer )  {
			if( ++ackErrors >= 3 )  {
				owner->SendMsgFmt( ADC_ERROR, "ADC Board did not Acknowledge Last Command (%c) !!! Error Count = %d", 
					ackTimerCommand, ackErrors );
				owner->ResetBoard();
				blockConfigCount = 0;
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
		blockConfigCount = 0;
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
		if( dataHdr->packetID == testPacketID )  {
			owner->SendMsgFmt( ADC_ERROR, "Packet ID Error Current=%d Last=%d",
				 dataHdr->packetID, testPacketID  );
			return;
		}
		if( dataHdr->packetID < testPacketID )  {
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
		else if( sampleCount )  {
			owner->SendQueueAdcData( ADC_AD_DATA, &dataHeader, (void *)currSecBuffer, 
				adSecDataLen );
			sampleCount = 0;
			++currPacketID;
		}
	}
	testPacketID = dataHdr->packetID;
		
	owner->goodData = TRUE;

	if( timeRefType == TIME_REF_USEPC )  {
		pcRef.NewPacket();
		if( spsRate == 25 || spsRate == 15 )
			pcRef.NewPacket();
	}
	else if( IsGPSRef() )  {
		DataHdrV2 v2Hdr;
		MakeV2Hdr( &v2Hdr, dataHdr );
		gpsRefVco.NewPacket( &v2Hdr );
		if( spsRate == 25 || spsRate == 15 )
			gpsRefVco.NewPacket( &v2Hdr );
	}
		
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
		
	if( !sampleCount )  {
		memset( currSecBuffer, 0, adSecDataLen );
		MakeHeader( dataHdr );
	}
	
	BYTE *dataPtr = (BYTE *)&newPacket[ sizeof(PreHdr) + sizeof( DataHdrSdr24 ) ];
	for( s = 0; s != numSamples/numChannels; s++ )  {
		for( i = 0; i != numChannels; i++ )  {
			if( dataPtr[0] & 0x80 )
				iPtr[3] = 0xff;
			else
				iPtr[3] = 0;
			iPtr[2] = dataPtr[0];
			iPtr[1] = dataPtr[1];
			iPtr[0] = dataPtr[2];
			currSecBuffer[ sampleCount++ ] = iData;
			dataPtr += 3;
		}
	}			
	
	if( sampleCount >= sampleSendCount )  {
		++currPacketID;
		owner->SendQueueAdcData( ADC_AD_DATA, &dataHeader, (void *)currSecBuffer, adSecDataLen );
		sampleCount = 0;
	}
}

void CAdcSdr24::FillDataHole( DWORD diff )
{
	int cnt = diff;
	
	while( cnt-- )  {
		if( !sampleCount )  {
			memset( currSecBuffer, 0, adSecDataLen );
			MakeHeaderHole( &currHdr.timeTick );
		}
		if( sampleCount >= sampleSendCount )  {
			++currPacketID;
			owner->SendQueueAdcData( ADC_AD_DATA, &dataHeader, (void *)currSecBuffer, adSecDataLen );
			sampleCount = 0;
		}
	}
}

BOOL CAdcSdr24::SendConfigPacket()
{
	BYTE crc, outDataPacket[128];
	int timeRef, len = sizeof( ConfigInfoSdr24 );
	ConfigInfoSdr24 *cfg = (ConfigInfoSdr24 *)&outDataPacket[ 2 ];
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
	
	owner->SendMsgFmt( ADC_MSG, "Sending Configuration Information Channels:%d", adcCfg->numberChannels );
	
	InitAdc( adcCfg->sampleRate, adcCfg->numberChannels );
	
	checkUTCFlag = TRUE;
	
	memset( cfg, 0, sizeof( ConfigInfoSdr24 ) );
	outDataPacket[0] = 0x02;
	outDataPacket[1] = 'C';
	
	cfg->numChannels = numChannels;
	cfg->sps = (BYTE)adcCfg->sampleRate;
	
	timeRefType = owner->config.timeRefType;
	if( timeRefType == TIME_REF_GARMIN )
		timeRef = TIME_SDR24_GARMIN;
	else if( timeRefType == TIME_REF_SKG )
		timeRef = TIME_SDR24_SKG;
	else if( timeRefType == TIME_REF_4800 )
		timeRef = TIME_SDR24_4800;
	else if( timeRefType == TIME_REF_9600 )
		timeRef = TIME_SDR24_9600;
	else
		timeRef = TIME_SDR24_USEPC;
	
	cfg->timeRefType = timeRef;
	
	if( adcCfg->highToLowPPS )
		cfg->flags |= FG_PPS_HIGH_TO_LOW;
	if( adcCfg->noPPSLedStatus )
		cfg->flags |= FG_DISABLE_LED;
	cfg->flags |= FG_WATCHDOG_CHECK;
		
	if( gpsConfig.only2DMode )
		cfg->flags |= FG_2D_ONLY;
	if( gpsConfig.enableWAAS )
		cfg->flags |= FG_WAAS_ON;
	
	for( int c = 0; c != MAX_SDR24_CHANNELS; c++ )
		cfg->gainFlags[ c ] = adcGainFlags[ c ];
			
	if( ( adcCfg->pulseWidth < 5 ) || ( adcCfg->pulseWidth > 95 ) )
		adcCfg->pulseWidth = 50;
	
	if( owner->m_debug )
		owner->SendMsgFmt( ADC_MSG, "SetVco=%d", (int)adcCfg->pulseWidth );
	
	gpsRefVco.SetVcoOnOffPercent( (int)adcCfg->pulseWidth );
	
	cfg->vcoOnTime = adcCfg->pulseWidth;
	cfg->vcoOffTime = 100 - cfg->vcoOnTime;
	
	GetSystemTime( &sysTm );
	currUTCTime = sysTm;
	cfg->timeTick = ( ( sysTm.wHour * 3600 ) + ( sysTm.wMinute * 60 ) + sysTm.wSecond ) * 1600; 
	cfg->timeTick += ( int )( ( (double)sysTm.wMilliseconds * 1.6 ) + 0.5 ); 
	currUTCTick = cfg->timeTick;
//	ConvertTimeToMs( &currUTCTick );	
	crc = owner->CalcCRC( &outDataPacket[1], (short)( len + 1 ) );
	outDataPacket[ len + 2 ] = crc;
	outDataPacket[ len + 3 ] = 0x03;
	owner->SendCommData( outDataPacket, len + 4 );
	owner->configSent = TRUE;
	blockConfigCount = 40;
	testPacketID = 0;
		
	return TRUE;
}

/* Send a one character command to the A/D Board */
void CAdcSdr24::SendPacket( char chr )
{
	BYTE crc, outDataPacket[32];
					
	if( owner->m_debug )
		owner->SendMsgFmt( ADC_MSG, "SDR24 Send Packet Type = %c", chr );
		
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
void CAdcSdr24::SetTimeDiff( LONG adjust, BOOL reset )
{
	BYTE outDataPacket[ 128 ], crc;
	TimeDiffInfo *tm = ( TimeDiffInfo * )&outDataPacket[ 2 ];
	int len = sizeof( TimeDiffInfo );
	double diff = ( (double)adjust * 1.6 );
	int tdiff;
	
	if( diff >= 0.0 )
		tdiff = (int)( diff + 0.5 ); 
	else
		tdiff = (int)( diff - 0.5 ); 
	tdiff += 5;
	
	if( owner->m_debug )
		owner->SendMsgFmt( ADC_MSG, "SetTimeDiff Adj=%d Send=%d Rst=%d", adjust, tdiff, reset );

	tm->adjustTime = tdiff;
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

BOOL CAdcSdr24::IsValidSpsRate( WORD sps, WORD numChannels, char *errMsg )
{
	if( numChannels > 4 )  {
		sprintf( errMsg, "Number of Channels (%d) must be 4 or less for this ADC board", numChannels );
		return FALSE;
	}
	if( sps == 15 || sps == 25 || sps == 30 || sps == 50 || sps == 60 || sps == 100 || sps == 120 || sps == 200 )
		return TRUE;
	sprintf( errMsg, "Sample Rate (%d) Error, must be 15, 25, 30, 50, 60, 100, 120 or 200", sps );
	return FALSE;
}

BOOL CAdcSdr24::GoodConfig( AdcBoardConfig2 *cfg, char *errMsg )
{
	errMsg[0] = 0;
	if( cfg->commSpeed < 9600 || cfg->commSpeed > 57600 )  {
		sprintf( errMsg, "Baud Rate (%d) must be between 9600 and 57600", cfg->commSpeed );
		return FALSE;
	}
	return IsValidSpsRate( cfg->sampleRate, cfg->numberChannels, errMsg );
}

BOOL CAdcSdr24::SendBoardCommand( DWORD cmd, void *data )
{
	int i, gain;
		
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
		if( data )
			i = 1;
		else
			i = 0;
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
		if( data )
			i = 1;
		else
			i = 0;
		gpsRef.debug = i;
		gpsRefVco.m_debug = i;
		return TRUE;
	}
		
	if( cmd == ADC_CMD_SET_GAIN_REF )  {
		AdcConfig *pConfig = (AdcConfig *)data;
		referenceVolts = pConfig->referenceVolts; 
		for( int c = 0; c != MAX_SDR24_CHANNELS; c++ )  {
			gain = pConfig->adcGainFlags[c] & CFG_GF_MASK;
			if( gain <= 1 )
				adcGainFlags[ c ] = 0x00;
			else if( gain == 2 )
				adcGainFlags[ c ] = 0x01;
			else if( gain == 4 )
				adcGainFlags[ c ] = 0x02;
			else if( gain == 8 )
				adcGainFlags[ c ] = 0x03;
			else if( gain == 16 )
				adcGainFlags[ c ] = 0x04;
			else if( gain == 32 )
				adcGainFlags[ c ] = 0x05;
			else if( gain >= 64 )
				adcGainFlags[ c ] = 0x06;
			if( pConfig->adcGainFlags[c] & CFG_GF_UNIPOLAR )  {
				owner->SendMsgFmt(ADC_MSG, "Chan %d is unipolar", c+1 );
				adcGainFlags[ c ] |= GF_UNIPOLAR_INPUT;
			}
		}	
		return TRUE;
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

	if( cmd == ADC_CMD_SET_VCO )  {
		int *ptr = (int *)data;
		WORD vco;
		if( *ptr < 1 || *ptr > 99 )
			vco = 50;
		else
			vco = (WORD)*ptr;
		SetVcoFreq( vco );
	}

	return FALSE;
}

/* Send the current time to the A/D board */
void CAdcSdr24::SendCurrentTime( BOOL reset )
{
	BYTE outDataPacket[ 128 ], crc;
	TimeInfoV2 *tm = (TimeInfoV2 *)&outDataPacket[ 2 ];
	int len = sizeof( TimeInfoV2 );
	SYSTEMTIME sTm;
		
	if( owner->m_debug )
		owner->SendMsgFmt( ADC_MSG, "SendCurrentTime reset=%d", reset );
	
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
	tm->timeTick = ( ( sTm.wHour * 3600 ) + ( sTm.wMinute * 60 ) + sTm.wSecond ) * 1600;
	tm->timeTick += ( int )( ( (double)sTm.wMilliseconds * 1.6 ) + 0.5 );
	crc = owner->CalcCRC( &outDataPacket[1], (short)(len + 1));
	outDataPacket[len + 2] = crc;
	outDataPacket[len + 3] = 0x03;
	owner->SendCommData( outDataPacket, len + 4 );
	
#ifdef WIN32_64
	if( priority )
		SetPriorityClass( process, priority );
#endif
}

void CAdcSdr24::ProcessGpsData( BYTE *packet )
{
	PreHdr *preHdr = (PreHdr *)packet;
	BYTE *str;
	str = &packet[ sizeof(PreHdr) ];
	str[preHdr->len-1] = 0;
	owner->SendQueueData( ADC_GPS_DATA, str, preHdr->len-1 );
}

void CAdcSdr24::ProcessAdcMessage( BYTE *packet )
{
	char *str = (char *)&packet[ sizeof( PreHdr ) ];
	DWORD len = (DWORD)strlen( str );
	owner->SendQueueData( ADC_AD_MSG, (BYTE *)str, len + 1);
}
	
void CAdcSdr24::ProcessStatusPacket( BYTE *packet )
{	
	StatusInfoV2 *sts = (StatusInfoV2 *)&packet[ sizeof( PreHdr ) ];
	StatusInfo dllSts;
	
	if( firstStatPacket )  {
		owner->SendMsgFmt( ADC_MSG, "ADC Board Firmware Version: %d.%d", 
			sts->majorVersion, sts->minorVersion );
		firstStatPacket = 0;
	}
		
	memset( &dllSts, 0, sizeof( dllSts ) );
	
	dllSts.boardType = sts->boardType;
	dllSts.majorVersion = sts->majorVersion;
	dllSts.minorVersion = sts->minorVersion;
	dllSts.numChannels = sts->numChannels;
	dllSts.spsRate = (BYTE)sts->sps;
	dllSts.lockStatus = GetLockStatus();
	dllSts.crcErrors = owner->crcErrors;
	dllSts.numProcessed = dllSts.packetsRcvd = packetsReceived;
	if( IsGPSRef() )  {
		gpsRefVco.MakeTimeInfo( &dllSts.timeInfo );
	}
	else if( timeRefType == TIME_REF_USEPC )
		pcRef.MakeTimeInfo( &dllSts.timeInfo );
	owner->SendQueueData( ADC_STATUS, (BYTE *)&dllSts, sizeof( StatusInfo ) );
}

void CAdcSdr24::MakeHeader( DataHdrSdr24 *hdr )
{
	DataHeader *dh = &dataHeader;
	SYSTEMTIME *st = &dh->packetTime;
	ULONG tick = hdr->timeTick;
	
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
	dh->flags = 0xc0;
}

void CAdcSdr24::MakeHeaderHole( ULONG *timeTick )
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
	dh->flags = 0xc0;
}

void CAdcSdr24::InitAdc( int sps, int numChans )
{	
	startRunTime = time(0);
	
 	spsRate = sps;
	numChannels = numChans;
	
	gpsRef.ownerSdr24 = this;
	gpsRef.owner = owner;
	gpsRef.Init();
	
	gpsRefVco.m_pOwner = this;
	gpsRefVco.m_pAdcBoard = owner;
	gpsRefVco.Init();
	
	pcRef.ownerSdr24 = this;
	pcRef.owner = owner;
	pcRef.Init();
	
	packetsReceived = currPacketID = 0;
	if( spsRate == 15 || spsRate == 25 )  {
		adDataLen = ( spsRate * numChannels * 4 ) / PACKETS_PER_SEC_SDR24_5;
		adSecDataLen = adDataLen * PACKETS_PER_SEC_SDR24_5;
	}
	else  {
		adDataLen = ( spsRate * numChannels * 4 ) / PACKETS_PER_SEC_SDR24_10;
		adSecDataLen = adDataLen * PACKETS_PER_SEC_SDR24_10;
	}
	sampleSendCount = spsRate * numChannels;
	sampleCount = 0;
	
	ackTimer = ackErrors = 0;
	memset( &currHdr, 0, sizeof( currHdr ) );
	
	testPacketID = 0;
	noNewDataFlag = 0;
	sendStatusCount = noPacketCount = 0;
	firstPacketCount = 10;
	checkPCAvg = checkPCAvgCount = 0;
	checkUTCFlag = TRUE;
	GetSystemTime( &currUTCTime );
	firstStatPacket = TRUE;
	needsResetFlag = 0;
}
	
void CAdcSdr24::NewCheckTimeInfo( LONG offset )
{
	char str[64];
	LONG diff, absDiff;
	int sts;
	
	if( IsGPSRef() )  {
		sts = gpsRefVco.m_currLockSts;
		if( !sts )  {
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
void CAdcSdr24::ProcessTimeInfo( BYTE *packet )
{
	WWVLocInfo *info = (WWVLocInfo *)&packet[ sizeof( PreHdr ) ];
	int tdiff;
	
	double diff = (double)info->data *  0.625;
	
	if( owner->m_debug )
		owner->SendMsgFmt( ADC_MSG, "ProcessTimeInfo=%d ms=%g", info->data, diff );
	
	if( diff >= 0.0 )
		tdiff = (int)( diff + .5 );
	else	
		tdiff = (int)( diff - .5 );
	if( owner->config.timeRefType != TIME_REF_USEPC && ( checkPCTime || setPCTime ) )  {
		NewCheckTimeInfo( tdiff );
		return;
	}
	
	if( owner->config.timeRefType == TIME_REF_USEPC )
		pcRef.NewTimeInfo( tdiff );		
}

void CAdcSdr24::SendTimeDiffRequest( BOOL reset )
{
	BYTE outDataPacket[ 128 ], crc;
	TimeInfoV2 *tm = (TimeInfoV2 *)&outDataPacket[ 2 ];
	int len = sizeof( TimeInfoV2 );
	SYSTEMTIME sTm;

	if( owner->m_debug )
		owner->SendMsgFmt( ADC_MSG, "SendTimeDiffRequest reset=%d", reset );
	
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
	tm->timeTick = ( ( ( sTm.wHour * 3600 ) + ( sTm.wMinute * 60 ) + sTm.wSecond ) * 1600 );
	tm->timeTick += ( int )( ( (double)sTm.wMilliseconds * 1.6 ) + 0.5 );
	
	crc = owner->CalcCRC( &outDataPacket[1], (short)(len + 1));
	outDataPacket[len + 2] = crc;
	outDataPacket[len + 3] = 0x03;
	owner->SendCommData(outDataPacket, len + 4 );

#ifdef WIN32_64
	if( priority )
		SetPriorityClass( process, priority );
#endif
}

BYTE CAdcSdr24::GetLockStatus()
{
	if( IsGPSRef() )
		return gpsRefVco.m_gpsLockStatus;
	return pcRef.pcLockStatus;
}

BOOL CAdcSdr24::IsGPSRef()
{
	if( timeRefType == TIME_REF_GARMIN || timeRefType == TIME_REF_SKG || 
			timeRefType == TIME_REF_4800 || timeRefType == TIME_REF_9600 )
		return TRUE;
	return FALSE;
}

void CAdcSdr24::CheckUTCTime( DataHdrSdr24 *hdr )
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
		t += ROLL_OVER_SEC;				// was 86400
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
	
void CAdcSdr24::GetTickHMS( ULONG tick, int *hour, int *min, int *sec )
{
	tick /= 1000;
	*hour = (WORD)( tick / 3600 );
	tick %= 3600;
	*min = (WORD)( tick / 60 );
	*sec = (WORD)( tick % 60 );
}

void CAdcSdr24::MakeTimeStr( char *to, ULONG tod )
{
	int hour = (WORD)( tod / 3600 );
	tod %= 3600;
	int min = (WORD)( tod / 60 );
	int sec = (WORD)( tod % 60 );
	sprintf( to, "%02d:%02d:%02d", hour, min, sec );
}

void CAdcSdr24::MakeTimeTickStr( char *to, ULONG tick )
{
	
	int ms = tick % 1000;
	tick /= 1000;
	
	int hour = tick / 3600;
	tick %= 3600;
	int min = tick / 60;
	int sec = tick % 60;
	sprintf( to, "%02d:%02d:%02d.%03d", hour, min, sec, ms );
}

/* 
void CAdcSdr24::DumpHdr( char *str, DataHdrV2 *hdr )
{
	owner->SendMsgFmt( ADC_MSG, "%s-ID=%d Tck=%d PPS=%d TOD=%d LE=%d Lck=%d Num=%d %d/%d/%d", str, 
		hdr->packetID, hdr->timeTick, hdr->ppsTick, hdr->gpsTOD, hdr->loopError, hdr->gpsLockSts,
		hdr->gpsSatNum, hdr->gpsMonth, hdr->gpsDay, hdr->gpsYear ); 
}
*/

void CAdcSdr24::SendNewAdjInfo( ULONG addDropTimer, BYTE addFlag )
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

void CAdcSdr24::AddTimeMs( ULONG *tick, SYSTEMTIME *st, DWORD ms )
{
	time_t t;
	struct tm *nt;
		
	*tick += ms;
	if( *tick >= 86400000 )  {
		*tick = *tick - 86400000;
		currUTCTime.wHour = currUTCTime.wMinute = currUTCTime.wSecond = 0;
		currUTCTime.wMilliseconds = 0;
		t = MakeLTime( currUTCTime.wYear, currUTCTime.wMonth, currUTCTime.wDay, 0, 0, 0 );
		t += ROLL_OVER_SEC;			// was 86400
		nt = gmtime( &t );
		currUTCTime.wYear = nt->tm_year + 1900;
		currUTCTime.wMonth = nt->tm_mon + 1;
		currUTCTime.wDay = 	nt->tm_mday;
	}
}

void CAdcSdr24::ProcessAck()
{
	ackErrors = ackTimer = 0;
}

void CAdcSdr24::SetRefBoardType()
{
	gpsRef.boardType = BOARD_SDR24;
	pcRef.boardType = BOARD_SDR24;
}

void CAdcSdr24::SaveTimeInfo( WORD percent )
{
	TimeInfo timeInfo;
		
	timeInfo.addDropMode = 0;
	timeInfo.addDropTimer = 0;
	timeInfo.pulseWidth = percent;
	owner->SendQueueData( ADC_SAVE_TIME_INFO, (BYTE *)&timeInfo, sizeof( TimeInfo ) );
}

/* Sends a command to adjust the VCO voltage */
void CAdcSdr24::SetVcoFreq( WORD onOffPercent )
{
	BYTE outDataPacket[ 128 ], crc;
	VcoInfo *vi = ( VcoInfo * )&outDataPacket[ 2 ];
	int len = sizeof( VcoInfo );
	
	SaveTimeInfo( onOffPercent );
	
	vi->onTime = onOffPercent;
	vi->offTime = 100 - vi->onTime;
	
	if( owner->m_debug )
		owner->SendMsgFmt( ADC_MSG, "SetVcoFreq=%d%% OnTime=%d OffTime=%d", onOffPercent, vi->onTime, vi->offTime );
	
	outDataPacket[0] = 0x02;
	outDataPacket[1] = 'V';
	crc = owner->CalcCRC( &outDataPacket[1], (short)( len + 1 ) );
	outDataPacket[len + 2] = crc;
	outDataPacket[len + 3] = 0x03;
	owner->SendCommData( outDataPacket, len + 4 );
	ackTimer = 20;
	ackTimerCommand = 'V';
}
