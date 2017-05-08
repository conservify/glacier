// gpsref.cpp - GPS Timing Class

#include "PSNADBoard.h"

#define CURR_LOCK_WAIT		( 15 * 10 )

#define UNLOCK_WAIT			2 * 60 * 60 * 10 		// 2 hours
#define WAS_LOCK_WAIT		UNLOCK_WAIT / 2

/* Called every packet when the GPS time reference is enabled */
void CGpsRefVco::NewPacket( DataHdrV2 *dataHdr )
{
	char str[ 256 ];
	int ppsMs;
		
	++m_currPacketTime;
	
	if( m_waitTimeCount )
		--m_waitTimeCount;

	if( m_skipPackets )  {
		--m_skipPackets;
		return;
	}
	
	m_currGpsHdr = *dataHdr;

	if( m_goodGpsData )
		CheckLockSts();			// Check for GPS time lock
	else
		CheckUnlockSts();

	if( m_logStrCount )  {
		--m_logStrCount;
		if( !m_logStrCount )  {
			MakeStatusStr( str );
			m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: %s", str );
			if( m_debug )  {
				TimeStrMin( str, m_notLockedTimer );
				m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: GpsSts:%d OffCnt:%d OnCnt:%d CurLockSts:%d NotLockTm:%s", 
					m_gpsLockStatus, m_lockOffCount, m_currOnCount, m_currLockSts, str );
				m_logStrCount = 1200;
			}
			else
				m_logStrCount = 3000;
		}
	}

	if( ( (int)m_lastPPSTime == -1 ) || ( ++m_noGpsPPSCount >= 600 ) )  {
		if( ( (int)m_lastPPSTime != -1 ) && ( m_currGpsHdr.gpsLockSts == '0' || m_currGpsHdr.gpsSatNum <= 2 ) )  {
			m_noGpsPPSCount = m_goodPPS = 0;
			return;
		}
		if( ++m_noGpsPPSCount >= 600 )  {
			m_pAdcBoard->SendMsgFmt( ADC_ERROR, "GPSRef: No 1PPS Signal LockSts=%c NumSats=%d PPS=%d Last=%d", 
				m_currGpsHdr.gpsLockSts, m_currGpsHdr.gpsSatNum, dataHdr->ppsTick, m_lastPPSTime );
			ResetRef();
			m_pOwner->SendPacket( 'g' );
			m_skipPackets = 300;
			SetUnlockCounter();
		}
		m_goodGpsData = m_noGpsPPSCount = m_goodPPS = 0;
		m_lastPPSTime = (LONG)dataHdr->ppsTick;
		return;
	}

	if( (LONG)m_lastPPSTime != (LONG)dataHdr->ppsTick )  {
		m_goodPPS = TRUE;
		m_noGpsPPSCount = 0;
		m_lastPPSTime = (LONG)dataHdr->ppsTick;
		m_lastGpsTime = dataHdr->gpsTOD;
		return;
	}
	
	/* Check to see if the time of day header field is changing */
	if( ( (int)m_lastGpsTime == -1 ) || ( m_lastGpsTime == dataHdr->gpsTOD ) )  {
		if( m_currGpsHdr.gpsLockSts == '0' || m_currGpsHdr.gpsSatNum <= 2 )  {
			m_noGpsDataCount = 0;
			return;
		}
		if( ++m_noGpsDataCount >= 900 )  {			// wait 90 sec before displaying error
			m_noGpsDataCount = 0;
			m_pAdcBoard->SendMsgFmt( ADC_ERROR, "GPSRef: No Data from Receiver" );
			m_logStrCount = 1200;
			m_pOwner->SendPacket( 'g' );
			SetUnlockCounter();
			m_goodGpsData = m_goodPPS = 0;
			m_skipPackets = 300;
		}
		m_lastGpsTime = dataHdr->gpsTOD;
		return;
	}
		
	if( m_noDataFlag )  {
		ResetRef();
		return;
	}
	
	m_goodGpsData = TRUE;
	
	if( m_lastGpsTime != dataHdr->gpsTOD )  {
		m_noGpsDataCount = 0;
		ppsMs = dataHdr->ppsTick % 1000;
		if( m_currLockSts && m_currGpsHdr.gpsLockSts != '0')  {
			m_currSecDiff = (LONG)dataHdr->gpsTOD - ( (LONG)dataHdr->timeTick / 1000 );
			if( m_currSecDiff )  {
				m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: Seconds Error of %d Resetting Board Time", m_currSecDiff );
				m_pOwner->SendPacket( 'i' );
				m_waitSecTime = 20;
				ResetRef();
				return;
			}
		}
		if( ppsMs >= 500 && ppsMs <= 999 )  {
			m_currPPSDiffAbs = (1000 - ppsMs);
			m_currPPSDiff = -m_currPPSDiffAbs;
		}
		else  {
			m_currPPSDiffAbs = m_currPPSDiff = ppsMs;
		}
		
		NewTimeSec();		
	}
	
	m_lastGpsTime = dataHdr->gpsTOD;
}

BOOL CGpsRefVco::NewTimeSec()
{		
	double tm;
			
	if( m_waitSecTime )  {
		--m_waitSecTime;
		return 0;
	}
	
	if( !m_currLockSts )  {
		m_badTimeCount = 0;
		return 1;
	}		
		
	if( m_timeState && ( m_currPPSDiffAbs >= 5 ) )  {
		if( ++m_badTimeCount >= 5 )  {
			m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: Reset A/D Board Time - Sts:%d 1PPS Diff:%d", 
				m_timeState, m_currPPSDiff );
			ResetRef();
		}
		return 1;
	}
	m_badTimeCount = 0;	
	
	if( m_timeState >= 1 )   {
		if( m_waitTimeCount )
			return 1;	
		AdjustVco();	
		return 1;
	}
		
	if( !m_timeState )  {
		tm = ( m_currSecDiff * 1000 ) + m_currPPSDiff;
		if( m_currPPSDiffAbs > 1 )  {
			m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: First Reset - Time Diff:%-6.3f", tm / 1000.0 );
			m_pOwner->SendPacket( 'i' );
		}
		else
			m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: First Time Diff:%-6.3f", tm / 1000.0 );
		m_waitSecTime = 20;
		m_timeState = 1;
		m_lockStartTime = m_currPacketTime; 
		m_vcoSetTime = m_currPacketTime;
	}
	
	return TRUE;
}

CGpsRefVco::CGpsRefVco()
{ 
	m_pOwner = 0;
	m_pAdcBoard = 0; 
	m_debug = 0;
}

void CGpsRefVco::Init()
{
	m_currPacketTime = 0;
	m_vcoOnOffPercent = 50;
	ResetRef();
}

/* Used to check if the GPS receiver is lock and giving out good time info */
void CGpsRefVco::CheckLockSts()
{
	/* Check for GPS lock */
	if( !m_goodPPS || m_currGpsHdr.gpsLockSts == '0' || m_currGpsHdr.gpsSatNum < 2 )  {
		if( ++m_notLockedTimer >= PTIME_5MIN )  {
			if( !m_highNotLockedFlag )  {
				m_highNotLockedFlag = TRUE;
				if( m_debug )
					m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: Setting High Not Locked Flag" );
			}
		}
		if( !m_currLockSts && !m_lockOffCount )
			return;
			
		if( m_lockOffCount )  {
			--m_lockOffCount;
			if( !m_lockOffCount )  {
				if( m_debug && ( m_gpsLockStatus != GPS_NOT_LOCKED ) )
					m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: Reference Not Locked" );
				m_gpsLockStatus = GPS_NOT_LOCKED;
				ResetRef();
			}
			else if( ( m_gpsLockStatus != GPS_WAS_LOCKED ) && ( m_lockOffCount < WAS_LOCK_WAIT ) )  {
				if( m_debug && (m_gpsLockStatus != GPS_WAS_LOCKED ) )
					m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: Reference Was Locked" );
				m_gpsLockStatus = GPS_WAS_LOCKED;
			}
		}
		return;
	}
	
	if( m_currLockSts && ( m_gpsLockStatus == GPS_LOCKED ) && !m_currOnCount )  {
		m_lockOffCount = UNLOCK_WAIT;
		return;
	}
			
	if( m_currLockSts && ( m_gpsLockStatus == GPS_WAS_LOCKED ) )  {
		if( m_debug && !m_currLockSts )
			m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: Reference Locked" );
		m_gpsLockStatus = GPS_LOCKED;
		m_lockOffCount = UNLOCK_WAIT;
		return;
	}
		
	if( m_currOnCount )  {
		--m_currOnCount;
		if( !m_currOnCount )  {
			if( m_debug && !m_currLockSts )
				m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: Reference Locked" );
			m_wasLocked = m_currLockSts = TRUE;
			m_gpsLockStatus = GPS_LOCKED;
			m_lockOffCount = UNLOCK_WAIT;
			m_notLockedTimer = 0;
		}
	}
}

void CGpsRefVco::SetUnlockCounter()
{			
	if( !m_goodGpsData || m_gpsLockStatus == GPS_NOT_LOCKED )
		return;
	
	m_noDataFlag = TRUE;
	m_lockOffCount = UNLOCK_WAIT;
	if( m_debug )
		m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: No Data Lock Off Count = %d seconds", m_lockOffCount / 10 );
}

void CGpsRefVco::CheckUnlockSts()
{
	if( m_gpsLockStatus == GPS_NOT_LOCKED || !m_lockOffCount )
		return;
		
	--m_lockOffCount;
	if( !m_lockOffCount )  {
		if( m_debug && ( m_gpsLockStatus != GPS_NOT_LOCKED ) )
			m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: Reference Not Locked (No Data)" );
		m_gpsLockStatus = GPS_NOT_LOCKED;
	}
	else if( ( m_gpsLockStatus != GPS_WAS_LOCKED ) && ( m_lockOffCount < WAS_LOCK_WAIT ) )  {
		if( m_debug )
			m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: Reference Was Locked (No Data)" );
		m_gpsLockStatus = GPS_WAS_LOCKED;
	}
}

void CGpsRefVco::ResetRef()
{
	m_goodGpsData =	m_goodPPS = m_noDataFlag = m_highNotLockedFlag = 0;
	m_notLockedTimer = m_lockOffCount = m_waitSecTime = 0;
	m_currLockSts = m_wasLocked = m_timeState = 0;
	m_currPPSDiff = m_currPPSDiffAbs = m_lockStartTime = 0;
	m_logStrCount =	m_noGpsDataCount = m_noGpsPPSCount = 0;
	m_currSecDiff = m_waitTimeCount = 0;	
	m_badTimeCount = m_resetTimeFlag = m_resetTimeStart = 0;
	m_vcoSetTime = m_vcoLastSetTime = 0;
	m_vcoDirection = m_vcoHighError = m_vco0to1Percent = 0;
	m_lastGpsTime = m_lastPPSTime = -1;
	m_logStrCount = 3000;
	m_gpsLockStatus = GPS_NOT_LOCKED;
	m_currOnCount = CURR_LOCK_WAIT; 
	m_skipPackets = 10;

	if( m_debug )
		m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: ResetRef" );
}
		
void CGpsRefVco::MakeTimeInfo( TimeInfo *info )
{
	info->addDropMode = 0;
	info->addDropTimer = 0;
	if( m_timeState >= 1 )
		info->timeLocked = ( m_currPacketTime - m_lockStartTime ) / 10;
	else
		info->timeLocked = 0;
	info->timeDiff = (char)m_currPPSDiff;
}
			
void CGpsRefVco::TimeStrHMS( char *str, DWORD time )
{
	int hour, min, sec; 
	hour = time / 3600;
	time %= 3600L;
	min = (int)(time / 60L);
	sec = (int)(time % 60L);
	sprintf( str, "%02d:%02d:%02d", hour, min, sec );
}

void CGpsRefVco::TimeStrMin( char *str, DWORD time )
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

void CGpsRefVco::MakeStatusStr( char *str )
{
	char lockStr[ 64 ], lastStr[ 64 ], currentStr[ 64 ];
		
	if( m_timeState >= 1 )
		TimeStrMin( lockStr, m_currPacketTime - m_lockStartTime );
	else
		strcpy(lockStr, "00:00");

	if( m_vcoLastSetTime )
		TimeStrMin( lastStr, m_vcoLastSetTime );
	else
		strcpy( lastStr, "00:00");
		
	if( m_vcoSetTime )
		TimeStrMin( currentStr, m_currPacketTime - m_vcoSetTime );
	else
		strcpy(currentStr, "00:00");
	
	sprintf( str, "Sts:%d Lck:%c Sats:%02d LckTm:%s Vco:%d%% VcoChg:%s/%s PPSDif:%d", 
		m_timeState, m_currGpsHdr.gpsLockSts, m_currGpsHdr.gpsSatNum, lockStr, m_vcoOnOffPercent, currentStr, 
		lastStr, m_currPPSDiff );
}
		
void CGpsRefVco::AdjustVco()
{		
	int adj, diff, currOnOff = m_vcoOnOffPercent;
	
	if( !m_currPPSDiff && m_vcoDirection )  {
		if( m_debug) 
			m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: Reset Direction" );
		diff = currOnOff - m_vco0to1Percent;
		currOnOff = currOnOff - ( diff / 2 );
		m_vcoDirection = m_vcoHighError = m_vco0to1Percent = 0;		
	}
	
	if( m_currPPSDiffAbs )  {
		if( m_currPPSDiffAbs >= 1 )  {
			if( m_currPPSDiffAbs < m_vcoHighError )  {
				if( m_debug )
					m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: Error Now Going Down OnOff=%d 0to1=%d", 
						currOnOff, m_vco0to1Percent );
//				diff = currOnOff - m_vco0to1Percent;
//				currOnOff = currOnOff - ( diff / 2 );
				m_vcoDirection = -1;
			}
			else if( m_currPPSDiffAbs > m_vcoHighError )  {
				if( !m_vcoHighError )  {
					m_vco0to1Percent = currOnOff;
				}
				m_vcoHighError = m_currPPSDiffAbs; 
				if( m_debug )
					m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: Error Going Up" );
				m_vcoDirection = 1;
			}
		}
	}
	
	if( m_currPPSDiffAbs >= 4 )
		adj = 3;		
	else if( m_currPPSDiffAbs >= 3 )
		adj = 2;
	else
		adj = 1;
				
	if( m_currPPSDiffAbs && ( m_vcoDirection != -1 ) )  {
		if( m_currPPSDiff >= 1 )
			currOnOff -= adj;
		else
			currOnOff += adj;
	}
			
	if( currOnOff < 5 )
		currOnOff = 5;
	else if( currOnOff > 95 )
		currOnOff = 95;
		
	if( currOnOff != m_vcoOnOffPercent )  {
		m_vcoLastSetTime = m_currPacketTime - m_vcoSetTime;
		m_vcoOnOffPercent = currOnOff;
		m_pOwner->SetVcoFreq( m_vcoOnOffPercent );
		m_vcoSetTime = m_currPacketTime;
	}
	
	if( m_debug )
		m_pAdcBoard->SendMsgFmt( ADC_MSG, "GPSRef: Vco PPSDif=%d Cur=%d Dir=%d HiErr=%d", m_currPPSDiff, 
			m_vcoOnOffPercent, m_vcoDirection, m_vcoHighError );
	
	if( m_currPPSDiffAbs > 4 )
		m_waitTimeCount = 300;
	else if( m_currPPSDiffAbs > 1 )
		m_waitTimeCount = 600;
	else
		m_waitTimeCount = 1200;
}
