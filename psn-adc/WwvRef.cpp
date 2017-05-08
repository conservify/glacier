// WwvRef.cpp - WWV Timing Class

#include "PSNADBoard.h"

#define RESET_AVG_TIME				10
#define PULSE_AVG_NUMBER			15
#define MIN_ADD_DROP				200
#define WWV_PULSE_WIDTH				800

#define	PERIOD_CHECK_TIME			PTIME_15MIN

void CWWVRef::NewPacket()
{
	++currPacket;
	++addSubTimer;
		
	CheckLockStatus();
	
	/* If we have a timer value, adjust the time on the A/D board by 1 ms */
	if( adjTimer && ( addSubTimer >= adjTimer ) )  {
		addSubTimer = 0;
		if( adjModeAdd )
			SendAdjPacket( 'a' );
		else			
			SendAdjPacket( 's' );
	}
}

void CWWVRef::NewTimeInfo( int tickTime, int msCount )
{
	int diff;
	
	if( firstTime )  {
		firstTime = 0;
		timeOffset = owner->config.timeOffset;
		adjTimer = owner->config.addDropTimer;
		adjModeAdd = owner->config.addDropMode;
		if( owner->config.pulseWidth )
			pulseWidth = owner->config.pulseWidth;
		if( !adjTimer )  {
			adjTimer = ownerV2->adjTimeInfo.addDropTimer;
			adjModeAdd = ownerV2->adjTimeInfo.addTimeFlag;
		}
	}
	
	if( !CalcTimeDiff( &diff, tickTime, msCount ) )
		return;

	lastGoodPacket = currPacket;
	
	if( timeState == 2 )
		ProcLock( diff, tickTime, msCount );
	else if( timeState == 1 )
		ProcCalc( diff, tickTime, msCount );
	else
		ProcNotLock( diff, tickTime, msCount );
}

void CWWVRef::ProcNotLock( int diff, int tickTime, int msCount )
{
	char str[80];
	int avg;
	LONG lt, absDiff;
		
	if( !diffCnt )  {
		AddAvg( diff );
		if( debug == 1 )
			owner->SendMsgFmt( ADC_MSG, "WWVRef: Not Locked Count=%d Diff=%dms Width=%d", diffCnt, diff, msCount);
		return;
	}
	avg = CalcAvg();
	absDiff = abs( avg - diff );
	if( absDiff >= 10 )  {
		if( diffCnt >= 2 && badCount >= 2 )  {
			if( debug == 1 )
				owner->SendMsgFmt( ADC_MSG, "WWVRef: Not Locked Dropped: Count=%d Diff=%dms Avg=%dms Abs=%dms Width=%d",
					diffCnt, diff, avg, absDiff, msCount);
			++badCount;
			return;
		}
		diffCnt = curDiff = 0;
		AddAvg( diff );
		badCount = 0;
		if( debug == 1 )
			owner->SendMsgFmt( ADC_MSG, "WWVRef: Not Locked Reset Count=%d Diff=%dms Avg=%dms Abs=%dms Width=%d", 
				diffCnt, diff, avg, absDiff, msCount);
		return;
	}
	badCount = 0;
	avg = AddAvg( diff );
	
	if( diffCnt < MAX_WWV_DIFF_TBL )  {
		if( debug == 1 )
			owner->SendMsgFmt( ADC_MSG, "WWVRef: Not Locked Count=%d Diff=%dms Avg=%dms Abs=%dms Width=%d", 
				diffCnt, diff, avg, absDiff, msCount);
		return;
	}
	
	ownerV2->SetTimeDiff( -avg, 0 );
	
	if( adjTimer )  {
		firstLock = TRUE;
		timeState = 2;
	}
	else  {
		firstCalc = TRUE;
		timeState = 1;
	}
	wasLocked = goodLock = TRUE;
	
	diffCnt = curDiff = 0;
	wwvLockStatus = WWV_LOCKED;
	
	if( startNotLockPacket )  {
		lt = currPacket - startNotLockPacket;
		if( debug == 1 )  {
			StrTimeSec( str, lt / 10 );
			owner->SendMsgFmt( ADC_MSG, "WWVRef: New Lock Time=%s Count=%d Diff=%dms Avg=%dms Abs=%dms Width=%d", str, 
				diffCnt, diff, avg, absDiff, msCount );
		}
	}
	else if( debug == 1 )
		owner->SendMsgFmt( ADC_MSG, "WWVRef: First Lock Count=%d Diff=%dms Avg=%dms Abs=%dms Width=%d", 
			diffCnt, diff, avg, absDiff, msCount);
	addSubTimer = 0;
}

void CWWVRef::ProcCalc( int diff, int tickTime, int msCount )
{
	int avg, avgAbs, iErrRate, avgDiff;
	double errorRate;
	ULONG testTime;
	char str[64];
			
	if( firstCalc )  {
		firstCalc = badCount = currPerLockCnt = adjTimer = 0;
		calcStartPacket = periodStartTimer = currPacket; 
	}
	
	avg = CalcAvg();
	avgDiff = abs( avg - diff );
	if(  avgDiff >= 12 )  {
		if( badCount <= 2 )  {
			owner->SendMsgFmt( ADC_MSG, "WWVRef: Calc Dropped: Count=%d Diff=%dms Avg=%d AdvDiff=%d Width=%d",
				diffCnt, diff, avg, avgDiff, msCount);
			++badCount;
			return;
		}
		firstCalc = TRUE;
		badCount = 0;
		AddAvg( diff );
		if( debug == 1 )
			owner->SendMsgFmt( ADC_MSG, "WWVRef: Calc Reset Count=%d Diff=%dms Avg=%dms Width=%d", 
				diffCnt, diff, avg, msCount);
		return;
	}
	badCount = 0;
	
	avg = AddAvg( diff );
	avgAbs = abs( avg );
	
	++currPerLockCnt;
		
	if( diffCnt < (MAX_WWV_DIFF_TBL-1) )
		return;
	
	if( ( currPacket - periodStartTimer ) >= PTIME_15MIN )  {
		if( currPerLockCnt < 10 )  {
			owner->SendMsgFmt( ADC_MSG, "WWVRef: Calibrate reset do to not enough good locks");
			if( currPerLockCnt >= 5 )  {
				ownerV2->SetTimeDiff( -avg, 0 );
				diffCnt = curDiff = 0;
				AddAvg( diff );
			}
			firstCalc = TRUE;
			return;
		}
		periodStartTimer = currPacket; 
		currPerLockCnt = 0;
	}
		
	testTime = currPacket - calcStartPacket;
	StrTimeSec( str, testTime / 10 );
	if( avg )  {
		errorRate = (double)testTime / (double)avg;
		owner->SendMsgFmt( ADC_MSG, "WWVRef: Error Rate=%g Time=%s Avg=%d Diff=%d", 
			errorRate, str, avg, diff );
		if( errorRate < 0.0 )
			iErrRate = (int)( errorRate - 0.5 );
		else
			iErrRate = (int)( errorRate + 0.5 );
		if( ( avgAbs > 20 ) || ( ( avgAbs > 10 ) && ( testTime > PTIME_1HOUR ) ) )  {
			timeState = 2;
			firstLock = TRUE;
			wwvLockStatus = WWV_LOCKED;
			adjTimer = abs( iErrRate );
			if( adjTimer < MIN_ADJ_TIME )
				adjTimer = MIN_ADJ_TIME;
			if( iErrRate > 0 )
				adjModeAdd = 0;
			else
				adjModeAdd = 1;
			ownerV2->SetTimeDiff( -avg, 0 );
			for(int i = 0; i != diffCnt; i++)
				diffTimeTbl[i] -= avg;
			SaveTimeInfo( adjModeAdd, adjTimer, pulseWidth );
			owner->SendMsgFmt( ADC_MSG, "WWVRef: New Time Adjust Timer=%d Add=%d", adjTimer, adjModeAdd );
			addSubTimer = 0;
		}
	}
	else
		owner->SendMsgFmt( ADC_MSG, "WWVRef: New Calc Mode Info Average:%dms Diff=%dms Time=%s", avg, diff, str );
}

void CWWVRef::ProcLock(int diff, int tickTime, int msCount )
{
	int avg, avgAbs, msError, iErrRate, absDiff;
	ULONG lockTime;
	double errorRate;
	char locStr[64];
	BOOL goodSignal = 0;
				
	if( firstLock )  {
		firstLock = 0;
		currPerLockCnt = lastPerLockCnt = badCount = msAdjCount = addSubTimer = 0;
		periodLockTime = 0;
		lockStartPacket = currPacket;
	}
	
	if( !diffCnt )  {
		avg = AddAvg( diff );
		owner->SendMsgFmt( ADC_MSG, "WWVRef: First Lock Diff = %dms", diff );
		return;
	}
	
	avg = CalcAvg();
	absDiff = abs( avg - diff );
	if( absDiff >= 10 )  {
		owner->SendMsgFmt( ADC_MSG, "WWVRef: Locked Dropped: Count=%d Diff=%dms Avg=%dms Abs=%dms Width=%d",
			diffCnt, diff, avg, absDiff, msCount);
		if( badCount < 10 )  {
			++badCount;
			return;
		}
		owner->SendMsgFmt( ADC_MSG, "WWVRef: Locked Reset Count=%d Diff=%dms Avg=%dms Abs=%dms Width=%d", 
			diffCnt, diff, avg, absDiff, msCount);
		diffCnt = curDiff = 0;
		timeState = 0;
		AddAvg( diff );
		badCount = 0;
		return;
	}
	badCount = 0;
	
	avg = AddAvg( diff );
	if( diffCnt < MAX_WWV_DIFF_TBL )  {
		owner->SendMsgFmt( ADC_MSG, "WWVRef: Locked Count=%d Avg=%dms Diff=%dms Abs=%dms Timer=%d %d", 
			diffCnt, avg, diff, absDiff, adjTimer, adjModeAdd );
		return;
	}
	avgAbs = abs( avg );
	lockTime = currPacket - lockStartPacket;
	StrTimeSec( locStr, lockTime / 10 );
		
	if( !periodLockTime )  {
		periodLockTime = currPacket;
		lastPerLockCnt = currPerLockCnt = 0;
	}
	else if( ( currPacket - periodLockTime ) >= PERIOD_CHECK_TIME ) {
		lastPerLockCnt = currPerLockCnt;
		currPerLockCnt = 0;
		periodLockTime = currPacket;
	}
	++currPerLockCnt;
		
	msError = avg + msAdjCount;
	if( currPerLockCnt >= 9 && lastPerLockCnt >= 9 )
		goodSignal = TRUE;
	wwvLockStatus = WWV_LOCKED;
	if( msError && goodSignal && ( avgAbs >= 15 ) && ( lockTime > PTIME_2HOUR ) )  {
		errorRate = (double)lockTime/ (double)msError;
		if( errorRate < 0.0 )
			iErrRate = (int)( errorRate - 0.5 );
		else
			iErrRate = (int)( errorRate + 0.5 );
		adjTimer = abs( iErrRate );
		if( adjTimer < MIN_ADJ_TIME )
			adjTimer = MIN_ADJ_TIME;
		if( iErrRate > 0 )
			adjModeAdd = 0;
		else
			adjModeAdd = 1;
		owner->SendMsgFmt( ADC_MSG, "WWVRef: Sending Time Adj=%d",  -avg );
		ownerV2->SetTimeDiff( -avg, 0 );
		for(int i = 0; i != diffCnt; i++)
			diffTimeTbl[i] -= avg;
		SaveTimeInfo( adjModeAdd, adjTimer, pulseWidth );
		owner->SendMsgFmt( ADC_MSG, "WWVRef: New Time Adjust Timer=%d Add=%d", adjTimer, adjModeAdd );
		msAdjCount = addSubTimer = 0;
		lockStartPacket = currPacket;
	}
	else
		owner->SendMsgFmt( ADC_MSG, "WWVRef: Locked Avg=%dms Diff=%dms Abs=%dms Timer=%d %d Lcks=%d/%d LockTime=%s", 
			avg, diff, absDiff, adjTimer, adjModeAdd, currPerLockCnt, lastPerLockCnt, locStr );
}
	
void CWWVRef::UnLock(int dsp)
{
	timeState = badCount = diffCnt = curDiff = wasLocked = msAdjCount = 0;
	startNotLockPacket = currPacket;
	wwvLockStatus = WWV_NOT_LOCKED;
	lastGoodPacket = 0;
	if(dsp)
		owner->SendMsgFmt( ADC_MSG, "WWVRef: Not locked");
}

int CWWVRef::AddAvg( int diff )
{
	diffTimeTbl[ curDiff ] = diff;
	if( ++curDiff >= MAX_WWV_DIFF_TBL )
		curDiff = 0;
	if( diffCnt < MAX_WWV_DIFF_TBL )
		++diffCnt;
	return CalcAvg();
}

int CWWVRef::CalcAvg()
{
	double avg = 0.0;
	
	if( !diffCnt )
		return 0;
	for(int i = 0; i != diffCnt; i++)
		avg += (LONG)diffTimeTbl[i];
	avg /= (double)diffCnt;
	if( avg < 0.0 )
		return (int)( avg - 0.5);
	return (int)( avg + 0.5);
}

int CWWVRef::CalcTimeDiff(int *diff, LONG tm, int msCount )
{
	int millsec, sec, min, hour, lowTest, hiTest;
	LONG tmp;
	
	if( timeState == 2 )  {
		lowTest = pulseWidth-20;
		hiTest = pulseWidth+20;
	}
	else  {
		lowTest = pulseWidth-30;
		hiTest = pulseWidth+30;
	}
	if( msCount < lowTest || msCount > hiTest )  {
		owner->SendMsgFmt( ADC_MSG, "WWVRef: Pulse Width (%d) too Long or Short", msCount );
		return 0;
	}	
	
	if( calcWidth )
		CalcPulseAvg( msCount );
	
	*diff = tm % 60000;
	if( *diff >= 30000 )
		*diff = -( 60000 - *diff );
	
	*diff = *diff - ( pulseWidth - msCount ) - timeOffset;
	
	millsec = (int)(tm % 1000L); tm /= 1000L;
	hour = (int)(tm / 3600L); tmp = tm % 3600L;
	min = (int)(tmp / 60L);
	sec = (int)(tmp % 60L);
	
	if( !timeState )  {
		if( ( sec >= 15 ) && ( sec <= 45 ) )  {
			if( debug == 1 )
				owner->SendMsgFmt( ADC_MSG, "WWVRef: Not Locked Bad CalcDiff=%02d:%02d:%02d.%03d", 
					hour, min, sec, millsec);
			return 0;
		}
	}
	else if( ( sec >= 1 ) && ( sec < 59 ) )  {
		if( debug == 1 )
			owner->SendMsgFmt( ADC_MSG, "WWVRef: Lock Bad CalcDiff=%02d:%02d:%02d.%03d", 
				hour, min, sec, millsec);
		return 0;
	}
	return TRUE;
}

void CWWVRef::StrTimeSec( char *to, LONG tm )
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

void CWWVRef::ResetNotLock()
{
	timeState = curDiff = diffCnt = 0;
}

void CWWVRef::SendAdjPacket( char chr )
{
	if( chr == 'a' )
		--msAdjCount;
	else 
		++msAdjCount;
	ownerV2->SendPacket( chr );
}


void CWWVRef::SaveTimeInfo( int mode, ULONG timer, WORD pulseWidth )
{
	TimeInfo timeInfo;
		
	timeInfo.addDropMode = mode;
	timeInfo.addDropTimer = timer;
	timeInfo.pulseWidth = pulseWidth;
	owner->SendQueueData( ADC_SAVE_TIME_INFO, (BYTE *)&timeInfo, sizeof( TimeInfo ) );
}

void CWWVRef::CalcPulseAvg( int msCount )
{
	if( lastPerLockCnt < 12 )  {
		pulseAvg = pulseAvgCount = 0;
		return;
	}
	pulseAvg += msCount;
	if( ++pulseAvgCount >= PULSE_AVG_NUMBER && currPerLockCnt > 12 )  {
		pulseWidth = pulseAvg / pulseAvgCount;
		owner->SendMsgFmt( ADC_MSG, "WWVRef: New Pulse Width Average=%d", pulseWidth );
		SaveTimeInfo( adjModeAdd, adjTimer, pulseWidth );
		calcWidth = 0;
	}
}

CWWVRef::CWWVRef()
{ 
	owner = 0;
	ownerV2 = 0; 
	Init();
}

void CWWVRef::Init()
{
	currPacket = lastGoodPacket = lockStartPacket = startNotLockPacket = 0;
	timeState = goodLock = curDiff = diffCnt = badCount = firstLock = firstCalc = 0;
	adjModeAdd = addSubTimer = adjTimer = msAdjCount = 0; 
	pulseAvg = pulseAvgCount = wasLocked = 0;
	wwvLockStatus = WWV_NOT_LOCKED;
	pulseWidth = WWV_PULSE_WIDTH;
	calcWidth = TRUE;
	debug = TRUE;
	firstTime = TRUE;
}

void CWWVRef::CheckLockStatus()
{
	int diff = currPacket - lastGoodPacket; 
	if( lastGoodPacket && ( !timeState || timeState == 1 ) && ( diff > PTIME_1HOUR ) )  {
		if( diffCnt )  {
			owner->SendMsgFmt( ADC_MSG, "WWVRef: Resetting Not Locked Status timeState=%d diff=%d diffCnt=%d",
				timeState, diff, diffCnt );
			ResetNotLock();
		}
		timeState = 0;
		lastGoodPacket = 0;
		wwvLockStatus = WWV_NOT_LOCKED;
		return;
	}
	
	if( !timeState && !wwvLockStatus )
		return;
		
	if( diff < PTIME_12HOUR && timeState == 2 )  {
//	if( diff < PTIME_2HOUR && timeState == 2 )  {
		wwvLockStatus = WWV_LOCKED;
		return;
	}
	if( ( diff > PTIME_12HOUR ) && ( wwvLockStatus == WWV_LOCKED ) )  {
//	if( ( diff > PTIME_2HOUR ) && ( wwvLockStatus == WWV_LOCKED ) )  {
		wwvLockStatus = WWV_WAS_LOCKED;
		owner->SendMsgFmt( ADC_MSG, "WWVRef: Setting Time Status to Was Locked" );
		return;
	}
	if( ( diff > PTIME_24HOUR ) && ( !wwvLockStatus || ( wwvLockStatus == WWV_WAS_LOCKED ) ) )  {
//	if( ( diff > PTIME_4HOUR ) && ( !wwvLockStatus || ( wwvLockStatus == WWV_WAS_LOCKED ) ) )  {
		UnLock( 1 );
		return;
	}
}

void CWWVRef::MakeTimeInfo( TimeInfo *info )
{
	info->addDropMode = adjModeAdd;
	info->addDropTimer = adjTimer;
	if( lockStartPacket )
		info->timeLocked = ( currPacket - lockStartPacket ) / 10;
	info->adjustNumber = msAdjCount;
	info->timeDiff = (char)CalcAvg();
	info->timeOffset = timeOffset; 
	info->pulseWidth = pulseWidth;
}
