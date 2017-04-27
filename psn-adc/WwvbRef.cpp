// WwvbRef.cpp - WWVB Timing Class

#include "PSNADBoard.h"

#define RESET_AVG_TIME				15
#define PULSE_AVG_NUMBER			15
#define MIN_ADD_DROP				200
#define WWV_PULSE_WIDTH				800
#define	PERIOD_CHECK_TIME			PTIME_15MIN

int monTbl[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
int leapMonTbl[12] = { 31,29,31,30,31,30,31,31,30,31,30,31 };

void CWWVBRef::NewPacket()
{
	int sts;
	LONG diff;
	
	++currPacket;
	++addSubTimer;
	++recalcAdjTimer;
	
	if( finishTimer )  {
		--finishTimer;
		if( !finishTimer && tickCount )
			tickCount = 0;
	}
	
	CheckLockStatus();
	
	/* If we have a timer value, adjust the time on the A/D board by 1 ms */
	if( adjTimer && ( addSubTimer >= adjTimer ) )  {
		addSubTimer = 0;
		diff = currPacket - lastGoodPacket;
		if( diff <= PTIME_2MIN )
			sts = 1;
		else
			sts = 0;
		if( adjModeAdd )  {
			if( !sts || avgDiff <= 0 )
				SendAdjPacket( 'a' );
			else if( debug )
				owner->SendMsgFmt( ADC_MSG, "WWVBRef: Skipped Adding Time Diff:%d Sts:%d", avgDiff, sts );
		}
		else  {			
			if( !sts || avgDiff >= 0 )
				SendAdjPacket( 's' );
			else if( debug )
				owner->SendMsgFmt( ADC_MSG, "WWVBRef: Skipped Subtrating Time Diff:%d Sts:%d", avgDiff, sts );
		}	
	}
}

void CWWVBRef::NewFrame()
{
	FrameInfo *fi = &frames[ frameCount ];
	int clearFrames = 0;
	char str[24];
			
	if( !GoodTicks() )  {
		if( debug > 1 )
			owner->SendMsgFmt( ADC_MSG, "WWVBRef: Bad Frame Mark" );
		return;
	}
	if( !GetTime() )  {
		if( debug > 1 )
			owner->SendMsgFmt( ADC_MSG, "WWVBRef: Bad GetTime" );
		return;
	}	
		
	fi->diff = currDiff = CalcTimeDiff();
	absDiff = abs( currDiff );
	fi->tod = MakeLTime( currYear, currMon, currDay, currHour, currMin, 0 );
	fi->startTime = ticks[0].data;
	
	if( debug )  {
		TimeStr( str, recalcAdjTimer );
		owner->SendMsgFmt( ADC_MSG, "WWVBRef: Good Frame %02d/%02d/%02d %02d:%02d CalcTmr:%s MsCnt:%d Err:%d Diff:%d ", 
			currMon, currDay, currYear % 100, currHour, currMin, str, msAdjCount, currDiff+msAdjCount, currDiff );
	}		
	 if( ++frameCount >= NUM_FRAMES )  {
		if( GoodFrames() )  {
			lastGoodPacket = currPacket;
			if( lockStatus == WWV_LOCKED )  {
				if( absAvgDiff > 4 )  {
					if( avgDiff >= 0 )
						SendAdjPacket( 's' );
					else
						SendAdjPacket( 'a' );
				}
				CheckAdjTimer();
			}
			else {
				if( debug )
					owner->SendMsgFmt( ADC_MSG, "WWVBRef: First Lock Diff = %d", avgDiff );					
				ownerV2->SetTimeDiff( -avgDiff, 0 );
				lockStatus = WWV_LOCKED;
				lockStartPacket = currPacket;
				addSubTimer = recalcAdjTimer = msAdjCount = frameCount = startNotLockPacket = 0;
				frameCount = 0;
				clearFrames = TRUE;
			}
		}
		if( !clearFrames )  {
			frameCount = 1;
			memcpy( &frames[0], &frames[1], sizeof( FrameInfo ) );
		}
	}
}

BOOL CWWVBRef::GoodFrames()
{
	time_t diff;
	FrameInfo *fi = &frames[0];
	FrameInfo *fi1 = &frames[1];
	
	diff = fi1->tod - fi->tod;
	if( ( diff != 60 ) && ( diff != 120 ) )  {
		if( debug > 1 )
			owner->SendMsgFmt( ADC_MSG, "WWVBRef: Bad Time of Day Diff:%d", diff );
		return 0;
	}
	
	diff = fi1->diff - fi->diff;
	if( abs( (long)diff ) > 5 )  {
		if( debug )
			owner->SendMsgFmt( ADC_MSG, "WWVBRef: Bad Frames Time Diff:%d", diff );
		return 0;
	}
	avgDiff = ( fi1->diff + fi->diff ) / 2;
	absAvgDiff = abs( avgDiff );
	return TRUE;
}

void CWWVBRef::NewTimeInfo( int tickTime, int msCount )
{
	if( firstTime )  {
		firstTime = 0;
		timeOffset = owner->config.timeOffset;
		adjTimer = owner->config.addDropTimer;
		adjModeAdd = owner->config.addDropMode;
		if( !adjTimer )  {
			adjTimer = ownerV2->adjTimeInfo.addDropTimer;
			adjModeAdd = ownerV2->adjTimeInfo.addTimeFlag;
		}
	}
	
	if( debug > 1)
		owner->SendMsgFmt( ADC_MSG, "WWVBRef: Count:%d  Length:%d TickTm:%d", 
			tickCount, msCount, tickTime );
	
	if( tickCount )  {
		if( msCount >= 770 && msCount <= 830 && lastMsCount >= 770 && lastMsCount <= 830 )  {
			if( debug > 1 )
				owner->SendMsgFmt( ADC_MSG, "WWVBRef: Minute mark found Count:%d Length:%d", tickCount, msCount );
			tickCount = 0;
			ticks[tickCount].data = tickTime;
			ticks[tickCount].msCount = msCount;
			++tickCount;
			finishTimer = 650;
		}	
		else  {
			ticks[tickCount].data = tickTime;
			ticks[tickCount].msCount = msCount;
			if( debug > 1 )
				owner->SendMsgFmt( ADC_MSG, "WWVBRef: Count:%d  Length:%d", tickCount, msCount );
			if( ++tickCount >= 60 )  {
				NewFrame();
				tickCount = 0;
			}
		}
	}
	else if( !tickCount && msCount >= 770 && msCount <= 830 && lastMsCount >= 770 && lastMsCount <= 830 )  {
		if( debug > 1 )
			owner->SendMsgFmt( ADC_MSG, "WWVBRef: First minute mark found Length:%d", msCount );
		ticks[tickCount].data = tickTime;
		ticks[tickCount].msCount = msCount;
		tickCount = 1;
		finishTimer = 650;
	}
	lastMsCount = msCount;
	lastTime = tickTime;
}

void CWWVBRef::CheckAdjTimer()
{
	char str[24], str1[24], chr;
	ULONG oldTimer = adjTimer;
	int oldMode = adjModeAdd, off = avgDiff + msAdjCount;
	int offAbs = abs(off);
	
	if( ( ( absAvgDiff >= 4 ) && ( offAbs >= 10 ) ) || ( ( offAbs >= 8 && recalcAdjTimer >= PTIME_2HOUR ) ) ||  
			( ( offAbs >= 7 && recalcAdjTimer >= PTIME_3HOUR ) ) )  {  
		adjTimer = recalcAdjTimer / offAbs;
		if( debug > 1 )
			owner->SendMsgFmt( ADC_MSG, "WWVBRef: wait=%d avgDiff=%d msAdj=%d off=%d abs=%d timer=%d", recalcAdjTimer, 
				avgDiff, msAdjCount, off, offAbs, adjTimer ); 
		if( adjTimer < MIN_ADJ_TIME )
			adjTimer = MIN_ADJ_TIME;
		if( off > 0 )
			adjModeAdd = 0;
		else
			adjModeAdd = 1;
		if( absAvgDiff >= 2 )
			ownerV2->SetTimeDiff( -avgDiff, 0 );
		
		if( ( adjTimer != oldTimer ) || ( adjModeAdd != oldMode ) )
			SaveTimeInfo( adjModeAdd, adjTimer );
			
		if( debug )  {
			if( adjModeAdd )
				chr = 'A';
			else
				chr = 'S';
			TimeStr( str, recalcAdjTimer );
			TimeStr( str1, adjTimer );
			owner->SendMsgFmt( ADC_MSG, "WWVBRef: New Adjust Time:%s Err:%d AdjTimer:%s Mode:%c", 
				str, off, str1, chr ); 
		}
		recalcAdjTimer = 0;
		avgDiff = absAvgDiff = currDiff = absDiff = msAdjCount = 0;
		frameCount = 0;
	}
}

LONG CWWVBRef::CalcTimeDiff()
{
	LONG refTm, avg = 0, cnt = 0;
			
	refTm = ( ( currHour * 3600 ) + ( currMin * 60 ) ) * 1000;	
	for( int i = 0; i != 59; i++ )  {
		avg += ( ticks[i].data - refTm );
		++cnt;
		refTm += 1000;
	}
	return (avg / cnt) - timeOffset;
}

BOOL CWWVBRef::GetTime()
{
	BYTE low, high, higher;
		
	low = GetNible( 8, 4 );
	if( low == 0xff )
		return 0;
	high = GetNible( 3, 3 );
	if( high == 0xff )
		return 0;
	currMin = (high * 10 + low);
	if( currMin > 59 )
		return 0;
	
	low = GetNible( 18, 4 );
	if( low == 0xff )
		return 0;
	high = GetNible( 13, 2 );
	if( high == 0xff )
		return 0;
	currHour = (high * 10) + low;
	if( currHour > 23 )
		return 0;
		
	low = GetNible( 33, 4 );
	if( low == 0xff )
		return 0;
	high = GetNible( 28, 4 );
	if( high == 0xff )
		return 0;
	higher = GetNible( 23, 2 );
	if( higher == 0xff )
		return 0;
	currYearDay = (higher * 100) + ( high * 10 ) + low;
	if( currYearDay > 366 )
		return 0;
	low = GetNible( 53, 4 );
	if( low == 0xff )
		return 0;
	high = GetNible( 48, 4 );
	if( high == 0xff )
		return 0;
	currYear = ( ( high * 10 ) + low ) + 2000;
	if( currYear > 2050 )
		return 0;
	GetMonthDay( &currMon, &currDay, currYearDay, currYear );
	return TRUE;
}

BYTE CWWVBRef::GetNible( int start, int len )
{
	BYTE ret = 0, mask = 1;
	int type;
	
	while( len-- )  {
		type = PulseType( ticks[start--].msCount );
		if( type == 2 )
			return 0xff;
		if( type )	
			ret |= mask;
		mask <<= 1;
	}
	return ret;
}

void CWWVBRef::UnLock(int dsp)
{
	badCount = wasLocked = msAdjCount = 0;
	startNotLockPacket = currPacket;
	lockStatus = WWV_NOT_LOCKED;
	lastGoodPacket = 0;
	if(dsp)
		owner->SendMsgFmt( ADC_MSG, "WWVBRef: Not locked");
}

BOOL CWWVBRef::GoodTicks()
{
	int i = 9, cnt = 5;
	while( cnt-- )  {
		if( PulseType( ticks[i].msCount ) != 2 )
			return 0;
		i += 10;
	} 
	return TRUE;
}

void CWWVBRef::StrTimeSec( char *to, LONG tm )
{
	int min, hour, days;
	
	days = (int)(tm / 86400L);
	tm %= 86400L;
	hour = (int)(tm / 3600L); 
	tm %= 3600L;
	min = (int)(tm / 60L);
	if(days)
		sprintf(to, "%d %02d:%02d", days, hour, min);
	else
		sprintf(to, "%02d:%02d", hour, min);
}

void CWWVBRef::SendAdjPacket( char chr )
{
	if( chr == 'a' )
		--msAdjCount;
	else 
		++msAdjCount;
	ownerV2->SendPacket( chr );
	if( debug > 1 )  {
		if( chr == 'a' )
			owner->SendMsgFmt( ADC_MSG, "WWVBRef: Sending Add Time Command MsAdj:%d", msAdjCount );
		else
			owner->SendMsgFmt( ADC_MSG, "WWVBRef: Sending Subtract Time Command MsAdj:%d", msAdjCount );
	}
}


void CWWVBRef::SaveTimeInfo( int mode, ULONG timer )
{
	TimeInfo timeInfo;
		
	timeInfo.addDropMode = mode;
	timeInfo.addDropTimer = timer;
	timeInfo.pulseWidth = 0;
	owner->SendQueueData( ADC_SAVE_TIME_INFO, (BYTE *)&timeInfo, sizeof( TimeInfo ) );
}

CWWVBRef::CWWVBRef()
{ 
	owner = 0;
	ownerV2 = 0; 
	Init();
}

void CWWVBRef::Init()
{
	recalcAdjTimer = tickCount = 0;
	currPacket = lastGoodPacket = lockStartPacket = startNotLockPacket = 0;
	goodLock = badCount = firstLock = firstCalc = 0;
	adjModeAdd = addSubTimer = adjTimer = msAdjCount = 0; 
	wasLocked = frameCount = 0;
	lockStatus = WWV_NOT_LOCKED;
	debug = 1;
	firstTime = TRUE;
}

void CWWVBRef::CheckLockStatus()
{
	if( !lockStatus )
		return;
	
	int diff = currPacket - lastGoodPacket; 
	if( diff > PTIME_48HOUR )
		UnLock( 1 );
}

void CWWVBRef::MakeTimeInfo( TimeInfo *info )
{
	info->addDropMode = adjModeAdd;
	info->addDropTimer = adjTimer;
	if( lockStartPacket )
		info->timeLocked = ( currPacket - lockStartPacket ) / 10;
	info->adjustNumber = msAdjCount;
	info->timeDiff = (char)currDiff;
	info->timeOffset = timeOffset; 
	info->pulseWidth = 0;
}

int CWWVBRef::PulseType( int msLen )
{
	if( msLen >= 170 && msLen <= 230 )
		return 0;
	if( msLen >= 470 && msLen <= 530 )
		return 1;
	return 2;
}

void CWWVBRef::GetMonthDay( int *month, int *day, int yDay, int year )
{
	int *tbl, acc = 1, i;
	
	if( _isleap(year) )
		tbl = leapMonTbl;
	else
		tbl = monTbl;
	for( i = 0; i != 12; i++ )  {
		if( (acc + *tbl) > yDay )
			break;
		acc += *tbl++;
	}
	*day = ( yDay - acc ) + 1;
	*month = i+1;
}

void CWWVBRef::TimeStrHMS( char *str, DWORD time )
{
	int hour, min, sec; 
	hour = time / 3600;
	time %= 3600L;
	min = (int)(time / 60L);
	sec = (int)(time % 60L);
	sprintf( str, "%02d:%02d:%02d", hour, min, sec );
}
	
void CWWVBRef::TimeStr( char *str, DWORD time )
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
