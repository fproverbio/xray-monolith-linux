// Multiplayer dropped as a concept (notes §11/§12/§13) - this is a
// dependency-free stub in place of the real DirectPlay8 client
// implementation, following OpenXRay's exact precedent (their own
// xrNetServer/empty/NET_Client.cpp does the same swap for the same
// reason - see notes §18e). Real INetQueue packet-queue bookkeeping is
// kept (it's genuinely used, e.g. by singleplayer's internal client-
// server loopback - see notes §13c), everything that talked to a real
// DirectPlay8 connection is reduced to an honest no-op.
#include "stdafx.h"
#pragma hdrstop

#include "NET_Client.h"
#include "NET_Server.h"

INetQueue::INetQueue()
{
	unused.reserve(128);
	for (int i = 0; i < 16; i++)
		unused.push_back(xr_new<NET_Packet>());
}

INetQueue::~INetQueue()
{
	cs.Enter();
	for (u32 it = 0; it < unused.size(); it++)
		xr_delete(unused[it]);
	for (u32 it = 0; it < ready.size(); it++)
		xr_delete(ready[it]);
	cs.Leave();
}

NET_Packet* INetQueue::Create()
{
	NET_Packet* P;
	if (unused.empty())
	{
		ready.push_back(xr_new<NET_Packet>());
		P = ready.back();
	}
	else
	{
		ready.push_back(unused.back());
		unused.pop_back();
		P = ready.back();
	}
	return P;
}

NET_Packet* INetQueue::Create(const NET_Packet& _other)
{
	cs.Enter();
	NET_Packet* P = Create();
	CopyMemory(P, &_other, sizeof(NET_Packet));
	cs.Leave();
	return P;
}

NET_Packet* INetQueue::Retreive()
{
	if (ready.empty())
		return NULL;
	return ready.front();
}

void INetQueue::Release()
{
	VERIFY(!ready.empty());
	ready.front()->B.count = 0;
	unused.push_back(ready.front());
	ready.pop_front();
}

//==============================================================================

XRNETSERVER_API Flags32 psNET_Flags = { 0 };
XRNETSERVER_API int psNET_ClientUpdate = 30; // FPS
XRNETSERVER_API int psNET_ClientPending = 2;
XRNETSERVER_API char psNET_Name[32] = "Player";
XRNETSERVER_API BOOL psNET_direct_connect = FALSE;

IPureClient::IPureClient(CTimer* tm) :
	net_Statistic(tm)
{
	device_timer = tm;
	net_TimeDelta_User = 0;
	net_Time_LastUpdate = 0;
	net_TimeDelta = 0;
	net_TimeDelta_Calculated = 0;
	net_Connected = EnmConnectionWait;
	net_Syncronised = FALSE;
	net_Disconnected = FALSE;
}

IPureClient::~IPureClient()
{
	psNET_direct_connect = FALSE;
}

HRESULT IPureClient::net_Handler(u32 dwMessageType, PVOID pMessage)
{
	return S_OK;
}

BOOL IPureClient::Connect(LPCSTR server_name)
{
	// Real DirectPlay8 session enumeration/connect dropped - see file
	// header. Singleplayer's client-server loopback needs no real
	// transport and must still succeed here, mirroring
	// IPureServer::Connect's psNET_direct_connect check in
	// NET_Server.cpp - only a genuine multiplayer connect attempt fails.
	if (psNET_direct_connect)
	{
		net_Connected = EnmConnectionCompleted;
		return TRUE;
	}

	Msg("! IPureClient::Connect: multiplayer networking is not built in "
		"this port (options '%s' ignored)", server_name ? server_name : "");
	net_Connected = EnmConnectionFails;
	return FALSE;
}

void IPureClient::Disconnect()
{
	net_csEnumeration.Enter();
	net_Hosts.clear();
	net_csEnumeration.Leave();

	net_Connected = EnmConnectionWait;
	net_Syncronised = FALSE;
}

void IPureClient::SendTo_LL(void* data, u32 size, u32 dwFlags, u32 dwTimeout)
{
	if (net_Disconnected)
		return;
	net_Statistic.dwBytesSended += size;
}

void IPureClient::_SendTo_LL(const void* data, u32 size, u32 flags, u32 timeout)
{
	IPureClient::SendTo_LL(const_cast<void*>(data), size, flags, timeout);
}

void IPureClient::_Recieve(const void* data, u32 data_size, u32 /*param*/)
{
	net_Statistic.dwBytesReceived += data_size;
	if (net_Connected == EnmConnectionCompleted)
		OnMessage(const_cast<void*>(data), data_size);
}

void IPureClient::Send(NET_Packet& P, u32 dwFlags, u32 dwTimeout)
{
	SendPacket(P.B.data, P.B.count, dwFlags, dwTimeout);
}

void IPureClient::Flush_Send_Buffer()
{
	FlushSendBuffer(0);
}

void IPureClient::OnMessage(void* data, u32 size)
{
	net_Queue.Lock();
	NET_Packet* P = net_Queue.Create();
	P->construct(data, size);
	P->timeReceive = timeServer_Async();
	u16 m_type;
	P->r_begin(m_type);
	net_Queue.Unlock();
}

BOOL IPureClient::net_HasBandwidth()
{
	if (net_Disconnected)
		return FALSE;

	u32 dwTime = TimeGlobal(device_timer);
	u32 dwInterval = (psNET_ClientUpdate != 0) ? (1000 / psNET_ClientUpdate) : 0;
	if (psNET_Flags.test(NETFLAG_MINIMIZEUPDATES))
		dwInterval = 1000;

	if (0 != psNET_ClientUpdate && (dwTime - net_Time_LastUpdate) > dwInterval)
	{
		net_Time_LastUpdate = dwTime;
		return TRUE;
	}
	return FALSE;
}

void IPureClient::ClearStatistic()
{
	net_Statistic.Clear();
}

void IPureClient::UpdateStatistic()
{
}

void IPureClient::Sync_Thread()
{
}

void IPureClient::Sync_Average()
{
}

void IPureClient::net_Syncronize()
{
	net_Syncronised = FALSE;
}

BOOL IPureClient::net_IsSyncronised()
{
	return net_Syncronised;
}

IC void IPureClient::timeServer_Correct(u32 sv_time, u32 cl_time)
{
}

bool IPureClient::GetServerAddress(ip_address& pAddress, DWORD* pPort)
{
	return true;
}
