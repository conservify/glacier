// gpsref.cpp - GPS Timing Class

#include "PSNADBoard.h"

#define CURR_LOCK_WAIT		( 15 * 10 )

#define UNLOCK_WAIT			2 * 60 * 60 * 10 		// 2 hours
#define WAS_LOCK_WAIT		UNLOCK_WAIT / 2

/* Called every packet when the GPS time reference is enabled */
void CGpsRef::NewPacket( DataHdrV2 *dataHdr )
{
	int ppsMs;
	char str[128];
	static int testCnt = 0;
					
	/* Increment some time variables */
	++currPacketTime;
	++addSubTimer;
	++msAdjPackets;
	++msAdjTimer;
	++recalcAdjTimer;
	
	if( skipPackets )  {
		--skipPackets;
		return;
	}
	
	currGpsHdr = *dataHdr;

	if( goodGpsData )
		CheckLockSts();			// Check for GPS time lock
	else
		CheckUnlockSts();
			
	if( logStrCount )  {
		--logStrCount;
		if( !logStrCount )  {
			MakeStatusStr( str );
			owner->SendMsgFmt( ADC_MSG, "GPSRef: %s", str );
			if( debug )  {
				TimeStrMin( str, notLockedTimer );
				owner->SendMsgFmt( ADC_MSG, "GPSRef: GpsSts:%d OffCnt:%d OnCnt:%d CurLockSts:%d NotLockTm:%s", 
					gpsLockStatus, lockOffCount, currOnCount, currLockSts, str );
				logStrCount = 1200;
			}
			else
				logStrCount = 3000;
		}
	}
		
	if( lastPPSTime == -1 || ( ++noGpsPPSCount >= 600 ) )  {
		if( ( lastPPSTime != -1 ) && ( currGpsHdr.gpsLockSts == '0' || currGpsHdr.gpsSatNum <= 2 ) )  {
			noGpsPPSCount = goodPPS = 0;
			return;
		}
		if( ++noGpsPPSCount >= 600 )  {
			owner->SendMsgFmt( ADC_ERROR, "GPSRef: No 1PPS Signal LockSts=%c NumSats=%d PPS=%d", 
				currGpsHdr.gpsLockSts, currGpsHdr.gpsSatNum, dataHdr->ppsTick );
			ResetRef( FALSE );
			SendPacket( 'g' );
		}
		SetUnlockCounter();
		goodGpsData = noGpsPPSCount = goodPPS = 0;
		lastPPSTime = dataHdr->ppsTick;
		return;
	}
	
	if( lastPPSTime != (LONG)dataHdr->ppsTick )  {
		goodPPS = TRUE;
		noGpsPPSCount = 0;
		lastPPSTime = (LONG)dataHdr->ppsTick;
		lastGpsTime = dataHdr->gpsTOD;
		return;
	}
	
	/* Check to see if the time of day header field is changing */
	if( ( (int)lastGpsTime == -1 ) || ( lastGpsTime == dataHdr->gpsTOD ) )  {
		if( currGpsHdr.gpsLockSts == '0' || currGpsHdr.gpsSatNum <= 2 )  {
			noGpsDataCount = 0;
			return;
		}
		if( ++noGpsDataCount >= 900 )  {			// wait 90 sec before displaying error
			noGpsDataCount = 0;
			owner->SendMsgFmt( ADC_ERROR, "GPSRef: No Data from Receiver LastTime=%d GPS=%d LockSts=%c (%d) SatNum=%d", 
				lastGpsTime, dataHdr->gpsTOD, currGpsHdr.gpsLockSts, currGpsHdr.gpsLockSts, currGpsHdr.gpsSatNum );
			logStrCount = 1200;
			owner->SendMsgFmt( ADC_MSG, "GPSRef: Sending Reset" );
			SendPacket( 'g' );
			SetUnlockCounter();
			goodGpsData = goodPPS = 0;
		}
		lastGpsTime = dataHdr->gpsTOD;
		return;
	}
		
	if( noDataFlag )  {
		RestartRef();
		return;
	}
	
	goodGpsData = TRUE;
	
	if( lastGpsTime != dataHdr->gpsTOD )  {
		noGpsDataCount = 0;
		ppsMs = dataHdr->ppsTick % 1000;
		if( currLockSts && currGpsHdr.gpsLockSts != '0' )  {
			currSecDiff = (LONG)dataHdr->gpsTOD - ( (LONG)dataHdr->timeTick / 1000 );
			if( currSecDiff )  {
				owner->SendMsgFmt( ADC_MSG, "GPSRef: Time Error = %d Seconds - Resetting Board Time", currSecDiff );
				SendPacket( 'i' );
				waitSecTime = 20;
				ResetRef( FALSE );
				return;
			}
		}
		if( ppsMs >= 500 && ppsMs <= 999 )  {
			currPPSDiffAbs = (1000 - ppsMs);
			currPPSDiff = -currPPSDiffAbs;
		}
		else  {
			currPPSDiffAbs = currPPSDiff = ppsMs;
		}
		NewTimeSec();		
	}
	lastGpsTime = dataHdr->gpsTOD;
		
	if( debug )  {
		if( ++testCnt >= 60 )  {
			testCnt = 0;
			owner->SendMsgFmt( ADC_MSG, "GPSRef: adjTm=%d tm=%d timeState=%d resetFlag=%d waitSecTime=%d HighNot=%d", 
				adjTimer, addSubTimer, timeState, resetTimeFlag, waitSecTime, highNotLockedFlag );
		}
	}
		
	if( adjTimer && ( timeState != 1 ) && !resetTimeFlag && !waitSecTime )
		NewTimeAdj();
}

void CGpsRef::NewTimeAdj()
{
	int adj;
	ULONG tmp;
	char tmstr[24], tm1str[24];
		
	if( currLockSts )  {
			
		if( currPPSDiffAbs >= 6 )  {
			owner->SendMsgFmt( ADC_MSG, "GPSRef: Reset High 1PPS Diff:%d", currPPSDiff );
			ResetTime();
			return;
		}
			
		if( recalcAdjTimer >= PTIME_2HOUR )  {
			recalcAdjTimer = 0;
			RecalcErrorRate();
		}		
		
		if( currPPSDiffAbs >= 1 )
			++notZeroCounter;
		else
			++zeroCounter;
		
		if( msAdjTimer >= msAdjTimerValue )  {
			if( debug )
				owner->SendMsgFmt( ADC_MSG, "GPSRef: AdjTimer - 1PPS Diff:%d Zero/NotZero:%d/%d", 
					currPPSDiffAbs, zeroCounter, notZeroCounter );
			msAdjTimer = 0; 
			if( notZeroCounter > zeroCounter )  {
				tmp = adjTimer;
				adj = CalcNewAdj();
				if( adjModeAdd )  {
					if( currPPSDiff < 0 )
						adjTimer -= adj;
					else if( currPPSDiff > 0 )
						adjTimer += adj;
				}
				else  {
					if( currPPSDiff > 0 )
						adjTimer -= adj;
					else if( currPPSDiff < 0 )
						adjTimer += adj;
				}
				if( adjTimer != tmp )  {
					CalcMsTimer();
					SaveTimeInfo( adjModeAdd, adjTimer );
					TimeStr( tmstr, adjTimer );
					TimeStr( tm1str, adj );
					if( debug )
						owner->SendMsgFmt( ADC_MSG, "GPSRef: New Timer Adj:%s NotZeroCount:%d ZeroCount:%d Adj:%s", 
							tmstr, notZeroCounter, zeroCounter, tm1str );
				}
		
				if( !extraAdjTimer && !zeroCounter && ( ( adjTimer - addSubTimer ) > 200 ) )  {
					extraAdjTimer = 100;
					if( !adjModeAdd )  {
						if( currPPSDiff < 0 )
							invertAdj = TRUE;
						else
							invertAdj = FALSE;
					}
					else  {
						if( currPPSDiff < 0 )
							invertAdj = FALSE;
						else
							invertAdj = TRUE;
					}
					if( debug )
						owner->SendMsgFmt( ADC_MSG, "GPSRef: Extra Timer Set Invert:%d", invertAdj );
				}
			}
			notZeroCounter = zeroCounter = 0;
		}
	}	
		
	if( extraAdjTimer )  {
		--extraAdjTimer;
		if( !extraAdjTimer )  {
			if( debug )
				owner->SendMsgFmt( ADC_MSG, "GPSRef: Extra Timer" );
			if( invertAdj )  {
				invertAdj = 0;
				if( adjModeAdd )
					SendAdjPacket( 's' );
				else			
					SendAdjPacket( 'a' );
			}
			else  {
				if( adjModeAdd )
					SendAdjPacket( 'a' );
				else			
					SendAdjPacket( 's' );
			}
		}
	}
		
	if( addSubTimer >= adjTimer )  {
		addSubTimer = 0;
		if( invertAdj )  {
			invertAdj = 0;
			if( adjModeAdd )  {
				if( !currLockSts || currPPSDiff >= 0 )
					SendAdjPacket( 's' );
				else if( debug )
					owner->SendMsgFmt( ADC_MSG, "GPSRef: Invert Skipped Subtracting Time 1PPS:%d LckSts:%d", 
						currPPSDiff, currLockSts );
			}
			else  {
				if( !currLockSts || currPPSDiff <= 0 )
					SendAdjPacket( 'a' );
				else if( debug )
					owner->SendMsgFmt( ADC_MSG, "GPSRef: Invert Skipped Adding Time 1PPS:%d LckSts:%d", 
						currPPSDiff, currLockSts );
			}
		}
		else  {
			if( adjModeAdd )  {
				if( !currLockSts || currPPSDiff <= 0 )
					SendAdjPacket( 'a' );
				else if( debug )
					owner->SendMsgFmt( ADC_MSG, "GPSRef: Skipped Adding Time 1PPS:%d LckSts:%d", 
						currPPSDiff, currLockSts );
			}
			else  {			
				if( !currLockSts || currPPSDiff >= 0 )
					SendAdjPacket( 's' );
				else if( debug )
					owner->SendMsgFmt( ADC_MSG, "GPSRef: Skipped Subtrating time 1PPS:%d LckSts:%d", 
						currPPSDiff, currLockSts );
			}
		}
	}
}		

BOOL CGpsRef::NewTimeSec()
{		
	char tmstr[24], adjstr[24], chr;
	double tm;
			
	if( waitSecTime )  {
		--waitSecTime;
		return 0;
	}
	
	if( !currLockSts )  {
		badTimeCount = 0;
		return 1;
	}		
		
	if( timeState && ( currPPSDiffAbs >= 6 ) )  {
		if( ++badTimeCount >= 5 )  {
			owner->SendMsgFmt( ADC_MSG, "GPSRef: Reset A/D Board Time - Sts:%d 1PPS Diff:%d", 
				timeState, currPPSDiff );
			ResetTime();
		}
		return 1;
	}
	badTimeCount = 0;	
	
	if( timeState == 2 )  {
		if( resetTimeFlag )  {
			if( debug )
				owner->SendMsgFmt( ADC_MSG, "GPSRef: ResetTimeFlag SendAdj 1PPS Diff:%d", currPPSDiff );
			if( currPPSDiff >= 1 )
				SendAdjPacket( 's' );
			else if( currPPSDiff <= -1 )
				SendAdjPacket( 'a' );
			else  {
				resetTimeFlag = 0;
				addSubTimer = msAdjPackets = msAdjTimer = msAdjCount = 0;
				extraAdjTimer = invertAdj = 0;
			}
		}
		return 0;
	}
		
	if( timeState == 1 )  {
		if( !currPPSDiffAbs )  {
			if( debug )  
				owner->SendMsgFmt( ADC_MSG, "GPSRef: Time State:2 Reset Timers" ); 
			timeState = 2;
			addSubTimer = msAdjCount = msAdjTimer = msAdjPackets = 0;
			recalcAdjTimer = extraAdjTimer = invertAdj = 0;
			resetTimeStart = lockStartTime = currPacketTime;
			highNotLockedFlag = 0;
			return 0;
		}
		if( currPPSDiff >= 1 )
			SendAdjPacket( 's' );
		else if( currPPSDiff <= -1 )
			SendAdjPacket( 'a' );
		return 0;
	}
		
	if( !timeState )  {
		tm = ( currSecDiff * 1000 ) + currPPSDiff;
		if( currPPSDiffAbs > 6 )  {
			owner->SendMsgFmt( ADC_MSG, "GPSRef: First Reset - Time Diff:%-6.3f", tm / 1000.0 );
			SendPacket( 'i' );
			waitSecTime = 20;
			calcState = 0;
			firstMsgFlag = 0;
			return 1;
		}
		else if( firstMsgFlag )  {
			owner->SendMsgFmt( ADC_MSG, "GPSRef: First Time Diff:%-6.3f", tm / 1000.0 );
			firstMsgFlag = 0;
		}
			
		if( adjTimer )  {
			if( debug )
				owner->SendMsgFmt( ADC_MSG, "GPSRef: Time State:0 New:1 AdjTimer" );
			calcState = 0;
			timeState = 1;
			return 0;
		}
		if( !calcState )  {
			if( !currPPSDiffAbs )  {
				if( debug )
					owner->SendMsgFmt( ADC_MSG, "GPSRef: Time State:0 CalcState:0 New:1 1PPS Diff:0" );
				calcState = 1;
				return 0;
			}
			if( currPPSDiff >= 1 )
				SendAdjPacket( 's' );
			else if( currPPSDiff <= -1 )
				SendAdjPacket( 'a' );
			return 0;
		}
		else if( calcState == 1 )  {
			if( currPPSDiffAbs >= 1 )  {
				if( debug )
					owner->SendMsgFmt( ADC_MSG, "GPSRef: Time state:0 CalcState:1 New:2 1PPS Diff:%d", 
						currPPSDiff );
				calcState = 2;
				calcTimer = currPacketTime;
				if( currPPSDiff >= 1 )
					SendAdjPacket( 's' );
				else if( currPPSDiff <= -1 )
					SendAdjPacket( 'a' );
				msAdjCount = 0;
			}
			return 0;
		}
		else if( calcState == 2 )  {
			int timeDiff = currPacketTime - calcTimer;
			int off = currPPSDiff + msAdjCount;
			int offAbs = abs(off);
			if( ( offAbs >= 6 ) || 
				( offAbs >= 5 && timeDiff >= PTIME_1HOUR ) ||  
				( offAbs >= 4 && timeDiff >= PTIME_2HOUR ) ||  
				( offAbs >= 3 && timeDiff >= PTIME_4HOUR ) || 
				( offAbs >= 2 && timeDiff >= PTIME_6HOUR ) )  {
				adjTimer = timeDiff / offAbs;
				if( adjTimer < MIN_ADJ_TIME )
					adjTimer = MIN_ADJ_TIME;
				CalcMsTimer();	
				if( off > 0 )  {
					adjModeAdd = 0;
					chr = 'S';
				}
				else  {
					adjModeAdd = 1;
					chr = 'A';
				}				
				timeState = 1;
				calcState = 0;
				TimeStr( adjstr, adjTimer );
				TimeStr( tmstr, timeDiff );
				owner->SendMsgFmt( ADC_MSG, "GPSRef: First AdjTm:%s %c TmDif:%s MsAdj:%d MsOff:%d 1PPS:%d", 
					adjstr, chr, tmstr, msAdjCount, off, currPPSDiff );
				SaveTimeInfo( adjModeAdd, adjTimer );
				msAdjCount = 0;
				return 0;
			}
			if( currPPSDiff >= 2 )
				SendAdjPacket( 's' );
			else if( currPPSDiff <= -2 )
				SendAdjPacket( 'a' );
			return 0;
		}
	}
	return 0;
}

CGpsRef::CGpsRef()
{ 
	owner = 0;
	ownerV2 = 0; 
	ownerVM = 0; 
	ownerSdr24 = 0; 
	debug = 0;
	Init();
}

void CGpsRef::SaveTimeInfo( int mode, ULONG timer )
{
	TimeInfo timeInfo;
		
	timeInfo.addDropMode = mode;
	timeInfo.addDropTimer = timer;
	timeInfo.pulseWidth = 0;
	owner->SendQueueData( ADC_SAVE_TIME_INFO, (BYTE *)&timeInfo, sizeof( TimeInfo ) );
}

/* Send a time adjust command to the A/D board and keep track of the adjustments */
void CGpsRef::SendAdjPacket( char chr )
{
	if( chr == 'a' )
		--msAdjCount;
	else 
		++msAdjCount;
	SendPacket( chr );
	if( debug )  {
		if( chr == 'a' )
			owner->SendMsgFmt( ADC_MSG, "GPSRef: Sending Add Time Command 1PPS Diff:%d MsAdj:%d", 
				currPPSDiff, msAdjCount );
		else
			owner->SendMsgFmt( ADC_MSG, "GPSRef: Sending Subtract Time Command 1PPS Diff:%d MsAdj:%d", 
				currPPSDiff, msAdjCount );
	}
}

void CGpsRef::MakeStatusStr( char *str )
{
	char chr, lockStr[24], adjTimerStr[24];
	LONG diff;
		
	if( adjTimer )  {
		if( adjModeAdd )
			chr = 'A';
		else
			chr = 'S';
	}
	else
		chr = '?';
		
	TimeStr( adjTimerStr, adjTimer );
	if( gpsLockStatus == GPS_LOCKED )  {
		if( timeState == 2 )
			diff = currPacketTime - lockStartTime;
		else
			diff = currPacketTime - calcTimer;
		TimeStrMin( lockStr, diff );
		sprintf( str, "Sts:%d Lck:%c Sats:%02d AdjTm:%s %c LckTm:%s MsAdj:%d MsOff:%d 1PPSDif:%d", 
			timeState, currGpsHdr.gpsLockSts, currGpsHdr.gpsSatNum, adjTimerStr, chr, lockStr, 
			msAdjCount, currPPSDiff + msAdjCount, currPPSDiff );
	}
	else  {
		TimeStrMin( lockStr, notLockedTimer );
		sprintf( str, "Sts:%d Lck:%c Sats:%02d AdjTm:%s %c NotLckTm:%s MsAdj:? MsOff:? 1PPSDif:?", 
			timeState, currGpsHdr.gpsLockSts, currGpsHdr.gpsSatNum, adjTimerStr, chr, lockStr );
	}
}

void CGpsRef::RecalcErrorRate()
{
	double errorRate;
	int iErrRate, iErrAbs, off, modeAdd;
	char tmstr[24], tm1str[24], chr;
		
	TimeStrMin( tm1str, msAdjPackets );	
	off = currPPSDiff + msAdjCount;
	if( abs( off ) < 6 )  {
		if( debug )
			owner->SendMsgFmt( ADC_MSG, "GPSRef: Recalc 1PPS Diff:%d MsAdj:%d MsOff:%d Time:%s (%d)", currPPSDiff, 
				msAdjCount, off, tm1str, msAdjPackets );
		return;
	}
	errorRate = (double)msAdjPackets / (double)( off );
	if( errorRate < 0.0 )
		iErrRate = (int)( errorRate - 0.5 );
	else
		iErrRate = (int)( errorRate + 0.5 );
	iErrAbs = abs( iErrRate );	
	
	if( iErrRate < 0 )  {
		modeAdd = TRUE; 
		chr = 'A';
	}
	else  {
		modeAdd = FALSE; 
		chr = 'S';
	}
				
	TimeStr( tmstr, iErrAbs );
	
	if( ( iErrAbs != (int)adjTimer ) || ( adjModeAdd != modeAdd ) )  {
		if( debug )
			owner->SendMsgFmt( ADC_MSG, 
				"GPSRef: New Recalc Diff:%d MsAdj:%d MsOff:%d Time:%s (%d) ErrRate:%s %c (%d)", 
				currPPSDiff, msAdjCount, off, tm1str, msAdjPackets, tmstr, chr, iErrRate );
		adjModeAdd = modeAdd;
		adjTimer = iErrAbs;
		if( adjTimer < MIN_ADJ_TIME )
			adjTimer = MIN_ADJ_TIME;
		CalcMsTimer();
		SaveTimeInfo( adjModeAdd, adjTimer );
	}
	else if( debug )
		owner->SendMsgFmt( ADC_MSG, 
			"GPSRef: Recalc Diff:%d MsAdj:%d MsOff:%d Time:%s (%d) ErrRate:%s %c (%d)", 
			currPPSDiff, msAdjCount, off, tm1str, msAdjPackets, tmstr, chr, iErrRate );
	msAdjPackets = msAdjTimer = msAdjCount = 0;
}

void CGpsRef::MakeTimeInfo( TimeInfo *info )
{
	info->addDropMode = adjModeAdd;
	info->addDropTimer = adjTimer;
	if( timeState == 2 )
		info->timeLocked = ( currPacketTime - lockStartTime ) / 10;
	info->adjustNumber = msAdjCount;
	info->timeDiff = (char)currPPSDiff;
}
			
void CGpsRef::CalcMsTimer()
{
	DWORD max;
	char str[16];
	
	if( currPPSDiffAbs <= 1 )  {
		msAdjTimerValue = adjTimer * 5;
		max = PTIME_15MIN;
	}
	else if( currPPSDiffAbs <= 2 )  {
		msAdjTimerValue = adjTimer * 3;
		max = PTIME_10MIN;
	}	
	else  {
		msAdjTimerValue = adjTimer * 2;
		max = PTIME_5MIN;
	}
	
	if( msAdjTimerValue >= max )
		msAdjTimerValue = max;
	
	if( debug )  {
		TimeStrMin( str, msAdjTimerValue );
		owner->SendMsgFmt( ADC_MSG, "GPSRef: CalcMsTimer 1PPS Diff:%d MsTimerVal:%s", currPPSDiff, str );
	}
}
	
int CGpsRef::CalcNewAdj()
{
	int adj;
	
	if( currPPSDiffAbs >= 3 )
		adj = (int)( (double)adjTimer * 0.025 );	
	else if( currPPSDiffAbs >= 2 )
		adj = (int)( (double)adjTimer * 0.01 );	
	else
		adj = (int)( (double)adjTimer * 0.005 );	
	return adj;
}

void CGpsRef::TimeStr( char *str, DWORD time )
{
	int hour, min, sec, tenth = time % 10; 
	time /= 10;
	hour = time / 3600;
	time %= 3600L;
	min = (int)(time / 60L);
	sec = (int)(time % 60L);
	if( hour )
		sprintf( str, "%d:%02d:%02d.%d", hour, min, sec, tenth );
	else if( min )
		sprintf( str, "%02d:%02d.%d", min, sec, tenth );
	else
		sprintf( str, "%d.%d", sec, tenth );
}

void CGpsRef::TimeStrHMS( char *str, DWORD time )
{
	int hour, min, sec; 
	hour = time / 3600;
	time %= 3600L;
	min = (int)(time / 60L);
	sec = (int)(time % 60L);
	sprintf( str, "%02d:%02d:%02d", hour, min, sec );
}

void CGpsRef::TimeStrMin( char *str, DWORD time )
{
	int days, hour, min; 
	time /= 10;
	days = time / 86400;
	time %= 86400;
	hour = time / 3600;
	time %= 3600L;
	min = (int)(time / 60L);
	if( days )
		sprintf( str, "%d %02d:%02d", days, hour, min );
	else
		sprintf( str, "%02d:%02d", hour, min );
}

/* Reset the time on the A/D board using the GPS time */
void CGpsRef::ResetTime()
{
	LONG diff, min, hour;
	char tmStr[40], adjStr[24];

	if( !highNotLockedFlag )  {
		owner->SendMsgFmt( ADC_MSG, "GPSRef: High Not Locked Flag Reset - 1PPS Diff:%d", currPPSDiff );
		ResetError( 'i' );
		adjTimer = timeState = calcState = resetTimeStart = 0;
		SaveTimeInfo( 0, 0 );
		return;	
	}
	
	/* If we have a large error reset time error using the 'i' command */
	if( currPPSDiffAbs >= 7 )  {
		owner->SendMsgFmt( ADC_MSG, "GPSRef: Resetting A/D Board Time - 1PPS Diff:%d", currPPSDiff );
		ResetError( 'i' );
		resetTimeStart = currPacketTime;
		if( highNotLockedFlag )  {
			ResetRef( FALSE );
			highNotLockedFlag = 0;
		}
		else  {
			ResetRef( TRUE );
			SaveTimeInfo( 0, 0 );
		}
		return;
	}
	owner->SendMsgFmt( ADC_MSG, "GPSRef: Reset Time 1PPS Diff:%d", currPPSDiff );
	
	/* We have a small error so adjust the time 1 ms at a time until the difference
	   is less then 1 ms */
	if( resetTimeStart )  {
		diff = ( currPacketTime - resetTimeStart ) / 10;
		min = diff / 60; hour = min / 60;
		sprintf( tmStr, "%d:%02d", hour, min % 60 );
		TimeStr( adjStr, adjTimer );
		owner->SendMsgFmt( ADC_MSG, "GPSRef: New Add/Drop Time AdjTm:%s Reset Time:%s", adjStr, tmStr );
	}
	resetTimeStart = currPacketTime;
	msAdjPackets = msAdjTimer = msAdjCount = badTimeCount = 0;
	extraAdjTimer = invertAdj = 0;
	resetTimeFlag = 1;
	calcTimer = 0;
	highNotLockedFlag = 0;
}
	
void CGpsRef::SendPacket( char chr )
{
	if( boardType == BOARD_V2 || boardType == BOARD_V3 )
		ownerV2->SendPacket( chr );
	else if( boardType == BOARD_VM )
		ownerVM->SendPacket( chr );
	else if( boardType == BOARD_SDR24 )
		ownerSdr24->SendPacket( chr );
}

void CGpsRef::ResetError( char commandType )
{		
	SendPacket( commandType );
	waitSecTime = 30;
	currSecDiff = currPPSDiff = currPPSDiffAbs = 0;
	addSubTimer = msAdjPackets = msAdjTimer = recalcAdjTimer = msAdjCount = badTimeCount = 0;
	extraAdjTimer = invertAdj = 0;
	lastPPSTime = lastGpsTime = -1;
	if( debug )
		owner->SendMsgFmt( ADC_MSG, "GPSRef: ResetError" );
}

/* Init the GPS time keeping process */
void CGpsRef::Init()
{
	ResetRef( TRUE );
	
	lastPPSTime = lastGpsTime = -1;
	noGpsDataCount = noGpsPPSCount = 0;
	currPPSDiff = currPPSDiffAbs = 0;
	waitSecTime = badTimeCount = skipPackets = 0;
	
	currLockSts = 0;
	gpsLockStatus = GPS_NOT_LOCKED;
	
	currOnCount = CURR_LOCK_WAIT; 
	lockOffCount = 0;
			
	goodGpsData = goodPPS = timeState = calcState = resetTimeFlag = wasLocked = 0;
	currPacketTime = addSubTimer = msAdjPackets = msAdjTimer = 0;
	adjModeAdd = 0;
	resetTimeStart = lockStartTime = 0;
	msAdjTimerValue = adjTimer = calcTimer = 0;
	notZeroCounter = zeroCounter = 0;
	invertAdj = extraAdjTimer = recalcAdjTimer = 0;
	logStrCount = 600;
	if( owner )  {
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
		CalcMsTimer();
	}
	currSecDiff = 0;
	notLockedTimer = 0;
	highNotLockedFlag = 0;
}

void CGpsRef::ResetRef( BOOL clearAll )
{
	gpsLockStatus = GPS_NOT_LOCKED;
	currOnCount = CURR_LOCK_WAIT; 
	
	if( clearAll )  {
		adjModeAdd = 0;
		adjTimer = 0;
	}
	firstMsgFlag = 1;
	resetTimeFlag = 0;
	waitSecTime = resetTimeStart = lockStartTime = 0; 
	msAdjTimerValue = currLockSts = lockOffCount = 0;
	lastPPSTime = lastGpsTime = -1;
	invertAdj = timeState = calcState = 0;
	extraAdjTimer = msAdjTimer = msAdjCount = 0;
	noGpsPPSCount = noGpsDataCount = 0;
	currSecDiff = currPPSDiff = currPPSDiffAbs = 0;
	currPacketTime = calcTimer = lockStartTime = addSubTimer = 0;
	msAdjPackets = msAdjTimer = recalcAdjTimer = 0;
	notZeroCounter = zeroCounter = currPPSDiff = currPPSDiffAbs = badTimeCount = 0;
	currSecDiff = 0;
	if( debug )
		owner->SendMsgFmt( ADC_MSG, "GPSRef: ResetRef" );
	if( adjTimer )
		CalcMsTimer();
}
		
/* Used to check if the GPS receiver is lock and giving out good time info */
void CGpsRef::CheckLockSts()
{
	/* Check for GPS lock */
	if( !goodPPS || currGpsHdr.gpsLockSts == '0' || currGpsHdr.gpsSatNum < 2 )  {
		if( ++notLockedTimer >= PTIME_5MIN )  {
			if( !highNotLockedFlag )  {
				highNotLockedFlag = TRUE;
				if( debug )
					owner->SendMsgFmt( ADC_MSG, "GPSRef: Setting High Not Locked Flag" );
			}
		}
		if( !currLockSts && !lockOffCount )
			return;
			
		if( lockOffCount )  {
			--lockOffCount;
			if( !lockOffCount )  {
				if( debug && ( gpsLockStatus != GPS_NOT_LOCKED ) )
					owner->SendMsgFmt( ADC_MSG, "GPSRef: Reference Not Locked" );
				gpsLockStatus = GPS_NOT_LOCKED;
				RestartRef();
			}
			else if( ( gpsLockStatus != GPS_WAS_LOCKED ) && ( lockOffCount < WAS_LOCK_WAIT ) )  {
				if( debug && (gpsLockStatus != GPS_WAS_LOCKED ) )
					owner->SendMsgFmt( ADC_MSG, "GPSRef: Reference Was Locked" );
				gpsLockStatus = GPS_WAS_LOCKED;
			}
		}
		return;
	}
	
	if( currLockSts && ( gpsLockStatus == GPS_LOCKED ) && !currOnCount )  {
		lockOffCount = UNLOCK_WAIT;
		return;
	}
			
	if( currLockSts && ( gpsLockStatus == GPS_WAS_LOCKED ) )  {
		if( debug && !currLockSts )
			owner->SendMsgFmt( ADC_MSG, "GPSRef: Reference Locked" );
		gpsLockStatus = GPS_LOCKED;
		lockOffCount = UNLOCK_WAIT;
		return;
	}
		
	if( currOnCount )  {
		--currOnCount;
		if( !currOnCount )  {
			if( debug && !currLockSts )
				owner->SendMsgFmt( ADC_MSG, "GPSRef: Reference Locked" );
			wasLocked = currLockSts = TRUE;
			gpsLockStatus = GPS_LOCKED;
			lockOffCount = UNLOCK_WAIT;
			notLockedTimer = 0;
		}
	}
}

void CGpsRef::SetUnlockCounter()
{			
	if( !goodGpsData || gpsLockStatus == GPS_NOT_LOCKED )
		return;
	
	noDataFlag = TRUE;
	lockOffCount = UNLOCK_WAIT;
	if( debug )
		owner->SendMsgFmt( ADC_MSG, "GPSRef: No Data Lock Off Count = %d seconds", lockOffCount / 10 );
}

void CGpsRef::CheckUnlockSts()
{
	if( adjTimer && ( addSubTimer >= adjTimer ) )  {
		addSubTimer = 0;
		if( adjModeAdd )  {
			SendAdjPacket( 'a' );
		}
		else  {			
			SendAdjPacket( 's' );
		}
	}
	
	if( gpsLockStatus == GPS_NOT_LOCKED || !lockOffCount )
		return;
		
	if( !adjTimer )  {
		lockOffCount = 0;
		gpsLockStatus = GPS_NOT_LOCKED;
		owner->SendMsgFmt( ADC_MSG, "GPSRef: Not Locked (No Data)");
		return;
	}
	
	--lockOffCount;
	if( !lockOffCount )  {
		if( debug && ( gpsLockStatus != GPS_NOT_LOCKED ) )
			owner->SendMsgFmt( ADC_MSG, "GPSRef: Reference Not Locked (No Data)" );
		gpsLockStatus = GPS_NOT_LOCKED;
	}
	else if( ( gpsLockStatus != GPS_WAS_LOCKED ) && ( lockOffCount < WAS_LOCK_WAIT ) )  {
		if( debug )
			owner->SendMsgFmt( ADC_MSG, "GPSRef: Reference Was Locked (No Data)" );
		gpsLockStatus = GPS_WAS_LOCKED;
	}
}

void CGpsRef::RestartRef()
{
	noDataFlag = 0;
	skipPackets = 30;
	ResetRef( FALSE );	
}
