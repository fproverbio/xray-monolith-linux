#pragma once
#ifdef XR_NETSERVER_EXPORTS
#define XRNETSERVER_API
//__declspec(dllexport)
#else
	#define XRNETSERVER_API
//__declspec(dllimport)

#ifndef _EDITOR
		#pragma comment(lib,	"xrNetServer"	)
#endif
#endif

#include "../xrCore/NET_utils.h"

// Were real DirectPlay8 send-flags bits (from <dplay/dplay8.h>), used
// throughout this module (as default-parameter values and bitwise-
// combined in NET_Common.cpp/NET_Messages.h) purely as opaque flag bits
// - nothing inspects their real DirectPlay8 semantics anymore
// (Send()/SendTo_LL() are stubs), they just need to stay distinct.
// Must precede NET_Messages.h below, which uses them directly.
#define DPNSEND_NOCOMPLETE 0x0002
#define DPNSEND_NONSEQUENTIAL 0x0004
#define DPNSEND_GUARANTEED 0x0008
#define DPNSEND_PRIORITY_HIGH 0x0010

#include "NET_Messages.h"


#include "NET_Compressor.h"

XRNETSERVER_API extern ClientID BroadcastCID;

XRNETSERVER_API extern Flags32 psNET_Flags;
XRNETSERVER_API extern int psNET_ClientUpdate;
XRNETSERVER_API extern int get_psNET_ClientUpdate();
XRNETSERVER_API extern int psNET_ClientPending;
XRNETSERVER_API extern char psNET_Name[];
XRNETSERVER_API extern int psNET_ServerUpdate;
XRNETSERVER_API extern int get_psNET_ServerUpdate();
XRNETSERVER_API extern int psNET_ServerPending;

XRNETSERVER_API extern BOOL psNET_direct_connect;

// DirectPlay8 CLSID/IID externs and DP8REFIID removed - this whole block
// only ever existed to declare symbols for real DirectPlay8 COM calls
// (CoCreateInstance(CLSID_DirectPlay8Client, ...) etc.) in NET_Client.cpp/
// NET_Server.cpp, both of which are now dependency-free stubs (multiplayer
// dropped as a concept - see notes §11/§12/§13/§18). Nothing else in this
// tree ever referenced these symbols.

enum
{
	NETFLAG_MINIMIZEUPDATES = (1 << 0),
	NETFLAG_DBG_DUMPSIZE = (1 << 1),
	NETFLAG_LOG_SV_PACKETS = (1 << 2),
	NETFLAG_LOG_CL_PACKETS = (1 << 3),
};

IC u32 TimeGlobal(CTimer* timer) { return timer->GetElapsed_ms(); }
IC u32 TimerAsync(CTimer* timer) { return TimeGlobal(timer); }

class XRNETSERVER_API IClientStatistic
{
	// Was a single DPN_CONNECTION_INFO ci_last (a live DirectPlay8 struct
	// filled in by IClientStatistic::Update(), called from the real
	// message-handler code in NET_Client.cpp/NET_Server.cpp). Both of
	// those are dependency-free stubs now with no DirectPlay8 message
	// loop to call Update() from, so it's dropped entirely - these fields
	// stay zero-initialized (see ZeroMemory below), which is the correct
	// "no live connection" reading for every getter.
	u32 ci_dwRoundTripLatencyMS, ci_dwThroughputBPS, ci_dwPeakThroughputBPS;
	u32 ci_dwPacketsDropped, ci_dwPacketsRetried;
	u32 mps_recive, mps_receive_base;
	u32 mps_send, mps_send_base;
	u32 dwBaseTime;
	CTimer* device_timer;
public:
	IClientStatistic(CTimer* timer)
	{
		ZeroMemory(this, sizeof(*this));
		device_timer = timer;
		dwBaseTime = TimeGlobal(device_timer);
	}

	IC u32 getPing() { return ci_dwRoundTripLatencyMS; }
	IC u32 getBPS() { return ci_dwThroughputBPS; }
	IC u32 getPeakBPS() { return ci_dwPeakThroughputBPS; }
	IC u32 getDroppedCount() { return ci_dwPacketsDropped; }
	IC u32 getRetriedCount() { return ci_dwPacketsRetried; }
	IC u32 getMPS_Receive() { return mps_recive; }
	IC u32 getMPS_Send() { return mps_send; }
	IC u32 getReceivedPerSec() { return dwBytesReceivedPerSec; }
	IC u32 getSendedPerSec() { return dwBytesSendedPerSec; }


	IC void Clear()
	{
		CTimer* timer = device_timer;
		ZeroMemory(this, sizeof(*this));
		device_timer = timer;
		dwBaseTime = TimeGlobal(device_timer);
	}

	//-----------------------------------------------------------------------
	u32 dwTimesBlocked;

	u32 dwBytesSended;
	u32 dwBytesSendedPerSec;

	u32 dwBytesReceived;
	u32 dwBytesReceivedPerSec;
};
