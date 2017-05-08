// PcRef.cpp - PC Timing Class

#include "PSNADBoard.h"

void CPcRef::NewPacket()
{
	++addSubTimer;
	++currPacketTime;
		
	if( ++packetCounter >= 3000 )  {
		packetCounter = 0;
		LogStatus();
	}
		
	if( checkTime )
		--checkTime;
	else  {
		checkTime = 300;
		if( boardType == BOARD_V2 || boardType == BOARD_V3 )
			ownerV2->SendTimeDiffRequest( FALSE );
		else if( boardType == BOARD_VM )
			ownerVM->SendTimeDiffRequest( FALSE );
		else if( boardType == BOARD_SDR24 )
			ownerSdr24->SendTimeDiffRequest( FALSE );
	}
		
	if( ( boardType != BOARD_SDR24 ) && adjTimer && ( addSubTimer >= adjTimer ) )  {
		addSubTimer = 0;
		if( adjModeAdd )
			SendPacket( 'a' );
		else			
			SendPacket( 's' );
	}
}

void CPcRef::SendPacket( char chr )
{
	if( boardType == BOARD_V2 || boardType == BOARD_V3 )
		ownerV2->SendPacket( chr );
	else if( boardType == BOARD_VM )
		ownerVM->SendPacket( chr );
	else if( boardType == BOARD_SDR24 )
		ownerSdr24->SendPacket( chr );
}

void CPcRef::NewTimeInfo( LONG diff )
{
	time_t now;
	
	pcLockStatus = PC_LOCK_STATUS;
		
	if( firstTimeInfo )  {
		firstTimeInfo = 0;
		if( boardType == BOARD_V2 || boardType == BOARD_V3 )
			ownerV2->SetTimeDiff( -diff, 0 );
		else if( boardType == BOARD_VM )
			ownerVM->SetTimeDiff( -diff, 0 );
		else if( boardType == BOARD_SDR24 )
			ownerSdr24->SetTimeDiff( -diff, 0 );
		else
			return;
		
		adjTimer = owner->config.addDropTimer;
		adjModeAdd = owner->config.addDropMode;
		if( !adjTimer )  {
			if( boardType == BOARD_V2 || boardType == BOARD_V3 )  {
				adjTimer = ownerV2->adjTimeInfo.addDropTimer;
				adjModeAdd = ownerV2->adjTimeInfo.addTimeFlag;
			}
			else if( boardType == BOARD_VM )  {
				adjTimer = ownerVM->adjTimeInfo.addDropTimer;
				adjModeAdd = ownerVM->adjTimeInfo.addTimeFlag;
			}
			else if( boardType == BOARD_SDR24 )  {
				adjTimer = ownerSdr24->adjTimeInfo.addDropTimer;
				adjModeAdd = ownerSdr24->adjTimeInfo.addTimeFlag;
			}
		}
		lockStartTime = time( 0 );
		return;
	}

	currDiff = AddDiff( diff );
	absDiff = abs( currDiff );
	if( ( avgNum >= MAX_LOC_AVG ) && ( absDiff > MAX_AVG_DIFF ) )  {
		owner->SendMsgFmt( ADC_MSG, "PCRef: Reset Diff=%d", currDiff );
		if( boardType == BOARD_V2 || boardType == BOARD_V3 )
			ownerV2->SetTimeDiff( -currDiff, 0 );
		else if( boardType == BOARD_VM )
			ownerVM->SetTimeDiff( -currDiff, 0 );
		else if( boardType == BOARD_SDR24 )
			ownerSdr24->SetTimeDiff( -currDiff, 0 );
		avgIdx = avgNum = absDiff = currDiff = 0;
		now = time( 0 );
		lastLockTime = now - lockStartTime;
		lockStartTime = now;
	}
}

CPcRef::CPcRef()
{
	owner = 0;
	ownerV2 = 0; 
	ownerVM = 0; 
	Init();	
}

int CPcRef::AddDiff( int newDiff )
{
	int avg, i;
			
	if( !avgNum )  {
		AddAvg( newDiff );
		return newDiff;
	}
	AddAvg( newDiff );
	avg = 0;
	for( i = 0; i != avgNum; i++ )
		avg += timeAvg[ i ];
	if( avg < 0 )
		avg = (int)( ( (double)avg / (double)avgNum ) - 0.5 );
	else
		avg = (int)( ( (double)avg / (double)avgNum ) + 0.5 );
	return avg;
}

void CPcRef::AddAvg( int diff )
{
	timeAvg[ avgIdx ] = diff;
	if( ++avgIdx >= MAX_LOC_AVG )
		avgIdx = 0;
	if( avgNum < MAX_LOC_AVG )
		++avgNum;
}

void CPcRef::Init()
{
	lockStartTime = lastLockTime = packetCounter = currPacketTime = 0;
	adjTimer = addSubTimer = 0;
	checkTime = 300;
	memset( timeAvg, 0, sizeof( timeAvg ) );
	avgIdx = avgNum = currDiff = absDiff = 0;
	adjModeAdd = firstTimeInfo = 0;
	firstTimeInfo = TRUE;
}

void CPcRef::LogStatus()
{
	char lockStr[80], lastStr[80];
	int hour, min; 	

	lockStr[0] = lastStr[0] = 0;
	if( lockStartTime )  {
		min = (int)(( time(0) - lockStartTime ) / 60);
		hour = min / 60;
		sprintf( lockStr, "%02d:%02d", hour, min % 60 );
	}
	if( lastLockTime )  {
		min = (int)(lastLockTime / 60);
		hour = min / 60;
		sprintf( lastStr, "%02d:%02d", hour, min % 60 );
	}
	owner->SendMsgFmt( ADC_MSG, "PCRef: Current Diff=%dms Lock Time=%s LastTime=%s", 
		currDiff, lockStr, lastStr );
}

void CPcRef::MakeTimeInfo( TimeInfo *info )
{
	info->addDropMode = adjModeAdd;
	info->addDropTimer = adjTimer;
	info->timeDiff = (char)currDiff;
	if( lockStartTime )
		info->timeLocked = (LONG)(time( 0 ) - lockStartTime);
}
