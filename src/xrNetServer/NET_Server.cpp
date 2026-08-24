// Multiplayer dropped as a concept (notes §11/§12/§13) - this is a
// dependency-free stub in place of the real DirectPlay8 server
// implementation, adapted from OpenXRay's own xrNetServer/empty/
// NET_Server.cpp (their exact same swap, for the same reason - see
// notes §18e) back onto Monolith's declared IPureServer/IClient API
// (BOOL/LPCSTR/DWORD*, not OpenXRay's refactored bool/pcstr/u32*).
// Ban-list persistence, IP filtering and client statistics are real,
// working logic carried over as-is - none of it is DirectPlay8-specific,
// it's ordinary file I/O and bookkeeping.
#include "stdafx.h"
#pragma hdrstop

#include "NET_Server.h"
#include "NET_Log.h"

static INetLog* pSvNetLog = NULL;

XRNETSERVER_API int psNET_ServerUpdate = 30; // FPS
XRNETSERVER_API int psNET_ServerPending = 3;

XRNETSERVER_API ClientID BroadcastCID(0xffffffff);

void ip_address::set(LPCSTR src_string)
{
	u32 buff[4];
	int cnt = sscanf(src_string, "%d.%d.%d.%d", &buff[0], &buff[1], &buff[2], &buff[3]);
	if (cnt == 4)
	{
		m_data.a1 = u8(buff[0] & 0xff);
		m_data.a2 = u8(buff[1] & 0xff);
		m_data.a3 = u8(buff[2] & 0xff);
		m_data.a4 = u8(buff[3] & 0xff);
	}
	else
	{
		Msg("! Bad ipAddress format [%s]", src_string);
		m_data.data = 0;
	}
}

xr_string ip_address::to_string() const
{
	string128 res;
	xr_sprintf(res, sizeof(res), "%d.%d.%d.%d", m_data.a1, m_data.a2, m_data.a3, m_data.a4);
	return res;
}

void IBannedClient::Load(CInifile& ini, const shared_str& sect)
{
	HAddr.set(sect.c_str());

	tm _tm_banned;
	const shared_str& time_to = ini.r_string(sect, "time_to");
	sscanf(time_to.c_str(), "%02d.%02d.%d_%02d:%02d:%02d", &_tm_banned.tm_mday, &_tm_banned.tm_mon,
		&_tm_banned.tm_year, &_tm_banned.tm_hour, &_tm_banned.tm_min, &_tm_banned.tm_sec);

	_tm_banned.tm_mon -= 1;
	_tm_banned.tm_year -= 1900;

	BanTime = mktime(&_tm_banned);
}

void IBannedClient::Save(CInifile& ini)
{
	ini.w_string(HAddr.to_string().c_str(), "time_to", BannedTimeTo().c_str());
}

xr_string IBannedClient::BannedTimeTo() const
{
	tm* t = localtime(&BanTime);
	string256 res;
	if (t)
		xr_sprintf(res, sizeof(res), "%02d.%02d.%d_%02d:%02d:%02d", t->tm_mday, t->tm_mon + 1,
			t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
	else
		res[0] = 0;
	return res;
}

//------------------------------------------------------------------------------

IClient::IClient(CTimer* timer) :
	stats(timer)
{
	server = NULL;
	dwTime_LastUpdate = 0;
	flags.bLocal = FALSE;
	flags.bConnected = FALSE;
	flags.bReconnect = FALSE;
	flags.bVerified = TRUE;
}

IClient::~IClient()
{
}

void IClient::_SendTo_LL(const void* data, u32 size, u32 flags_, u32 timeout)
{
	R_ASSERT(server);
	server->SendTo_LL(ID, const_cast<void*>(data), size, flags_, timeout);
}

//------------------------------------------------------------------------------

IClient* IPureServer::ID_to_client(ClientID ID, bool ScanAll)
{
	if (0 == ID.value())
		return NULL;
	return GetClientByID(ID);
}

void IPureServer::_Recieve(const void* data, u32 data_size, u32 param)
{
	NET_Packet packet;
	ClientID id;

	id.set(param);
	packet.construct(data, data_size);

	csMessage.Enter();
	if (psNET_Flags.test(NETFLAG_LOG_SV_PACKETS))
	{
		if (!pSvNetLog)
			pSvNetLog = xr_new<INetLog>("logs/net_sv_log.log", TimeGlobal(device_timer));
		if (pSvNetLog)
			pSvNetLog->LogPacket(TimeGlobal(device_timer), &packet, true);
	}
	u32 result = OnMessage(packet, id);
	csMessage.Leave();

	if (result)
		SendBroadcast(id, packet, result);
}

//==============================================================================

IPureServer::IPureServer(CTimer* timer, BOOL Dedicated)
{
	device_timer = timer;
	m_bDedicated = Dedicated;
	stats.clear();
	stats.dwSendTime = TimeGlobal(device_timer);
	SV_Client = NULL;
	pSvNetLog = NULL;
#ifdef DEBUG
	sender_functor_invoked = false;
#endif
}

IPureServer::~IPureServer()
{
	for (u32 it = 0; it < BannedAddresses.size(); it++)
		xr_delete(BannedAddresses[it]);
	BannedAddresses.clear();

	SV_Client = NULL;
	xr_delete(pSvNetLog);
	psNET_direct_connect = FALSE;
}

HRESULT IPureServer::net_Handler(u32 dwMessageType, PVOID pMessage)
{
	return S_OK;
}

IPureServer::EConnect IPureServer::Connect(LPCSTR options, GameDescriptionData& game_descr)
{
	// Real DirectPlay8 host/enumerate dropped - see file header. No
	// transport exists to host through.
	connect_options = options;
	psNET_direct_connect = strstr(options, "/single") ? TRUE : FALSE;

	if (!psNET_direct_connect)
	{
		Msg("! IPureServer::Connect: multiplayer networking is not built "
			"in this port (options '%s' ignored)", options);
		BannedList_Load();
		IpList_Load();
	}

	return ErrNoError;
}

void IPureServer::Disconnect()
{
	if (!psNET_direct_connect)
	{
		BannedList_Save();
		IpList_Unload();
	}
}

void IPureServer::Flush_Clients_Buffers()
{
	struct LocalSenderFunctor
	{
		static void FlushBuffer(IClient* client) { client->FlushSendBuffer(0); }
	};
	net_players.ForEachClientDo(LocalSenderFunctor::FlushBuffer);
}

void IPureServer::SendTo_Buf(ClientID ID, void* data, u32 size, u32 dwFlags, u32 dwTimeout)
{
	IClient* tmp_client = net_players.GetFoundClient(ClientIdSearchPredicate(ID));
	VERIFY(tmp_client);
	tmp_client->SendPacket(data, size, dwFlags, dwTimeout);
}

void IPureServer::SendTo_LL(ClientID ID, void* data, u32 size, u32 dwFlags, u32 dwTimeout)
{
	if (psNET_Flags.test(NETFLAG_LOG_SV_PACKETS))
	{
		if (!pSvNetLog)
			pSvNetLog = xr_new<INetLog>("logs/net_sv_log.log", TimeGlobal(device_timer));
		if (pSvNetLog)
			pSvNetLog->LogData(TimeGlobal(device_timer), data, size);
	}
}

void IPureServer::SendTo(ClientID ID, NET_Packet& P, u32 dwFlags, u32 dwTimeout)
{
	SendTo_LL(ID, P.B.data, P.B.count, dwFlags, dwTimeout);
}

void IPureServer::SendBroadcast_LL(ClientID exclude, void* data, u32 size, u32 dwFlags)
{
	struct ClientExcluderPredicate
	{
		ClientID id_to_exclude;
		ClientExcluderPredicate(ClientID exclude_) : id_to_exclude(exclude_) {}
		bool operator()(IClient* client) const
		{
			return (client->ID != id_to_exclude) && client->flags.bConnected;
		}
	};

	struct ClientSenderFunctor
	{
		IPureServer* m_owner;
		void* m_data;
		u32 m_size;
		u32 m_dwFlags;
		ClientSenderFunctor(IPureServer* owner, void* data, u32 size, u32 dwFlags) :
			m_owner(owner), m_data(data), m_size(size), m_dwFlags(dwFlags) {}
		void operator()(IClient* client) { m_owner->SendTo_LL(client->ID, m_data, m_size, m_dwFlags); }
	};

	ClientSenderFunctor temp_functor(this, data, size, dwFlags);
	net_players.ForFoundClientsDo(ClientExcluderPredicate(exclude), temp_functor);
}

void IPureServer::SendBroadcast(ClientID exclude, NET_Packet& P, u32 dwFlags)
{
	SendBroadcast_LL(exclude, P.B.data, P.B.count, dwFlags);
}

u32 IPureServer::OnMessage(NET_Packet& P, ClientID sender)
{
	return 0;
}

void IPureServer::OnCL_Connected(IClient* CL)
{
	Msg("* Player 0x%08x connected.\n", CL->ID.value());
}

void IPureServer::OnCL_Disconnected(IClient* CL)
{
	Msg("* Player 0x%08x disconnected.\n", CL->ID.value());
}

BOOL IPureServer::HasBandwidth(IClient* C)
{
	u32 dwTime = TimeGlobal(device_timer);

	if (psNET_direct_connect)
	{
		UpdateClientStatistic(C);
		C->dwTime_LastUpdate = dwTime;
		return TRUE;
	}

	u32 dwInterval = (psNET_ServerUpdate != 0) ? (1000 / psNET_ServerUpdate) : 0;
	if (psNET_Flags.test(NETFLAG_MINIMIZEUPDATES))
		dwInterval = 1000;

	if (psNET_ServerUpdate != 0 && (dwTime - C->dwTime_LastUpdate) > dwInterval)
	{
		UpdateClientStatistic(C);
		C->dwTime_LastUpdate = dwTime;
		return TRUE;
	}
	return FALSE;
}

void IPureServer::UpdateClientStatistic(IClient* C)
{
}

void IPureServer::ClearStatistic()
{
	stats.clear();
	struct StatsClearFunctor
	{
		static void Clear(IClient* client) { client->stats.Clear(); }
	};
	net_players.ForEachClientDo(StatsClearFunctor::Clear);
}

bool IPureServer::DisconnectClient(IClient* C, LPCSTR Reason)
{
	return C != NULL;
}

bool IPureServer::DisconnectAddress(const ip_address& Address, LPCSTR reason)
{
	xr_vector<IClient*> to_disconnect;

	struct ToDisconnectFillerFunctor
	{
		IPureServer* m_owner;
		xr_vector<IClient*>* dest;
		ip_address const* address_to_disconnect;
		ToDisconnectFillerFunctor(IPureServer* owner, xr_vector<IClient*>* dest_, ip_address const* addr) :
			m_owner(owner), dest(dest_), address_to_disconnect(addr) {}
		void operator()(IClient* client)
		{
			ip_address tmp_address;
			m_owner->GetClientAddress(client->ID, tmp_address);
			if (*address_to_disconnect == tmp_address)
				dest->push_back(client);
		}
	};

	ToDisconnectFillerFunctor tmp_functor(this, &to_disconnect, &Address);
	net_players.ForEachClientDo(tmp_functor);

	for (u32 i = 0; i < to_disconnect.size(); i++)
		DisconnectClient(to_disconnect[i], reason);
	return true;
}

bool IPureServer::GetClientAddress(ClientID ID, ip_address& Address, DWORD* pPort)
{
	IClient* C = ID_to_client(ID, true);
	if (!C)
		return false;
	Address = C->m_cAddress;
	if (pPort)
		*pPort = C->m_dwPort;
	return true;
}

IBannedClient* IPureServer::GetBannedClient(const ip_address& Address)
{
	for (u32 it = 0; it < BannedAddresses.size(); it++)
	{
		if (BannedAddresses[it]->HAddr == Address)
			return BannedAddresses[it];
	}
	return NULL;
}

void IPureServer::BanClient(IClient* C, u32 BanTime)
{
	ip_address ClAddress;
	GetClientAddress(C->ID, ClAddress);
	BanAddress(ClAddress, BanTime);
}

void IPureServer::BanAddress(const ip_address& Address, u32 BanTimeSec)
{
	if (GetBannedClient(Address))
	{
		Msg("Already banned");
		return;
	}

	IBannedClient* pNewClient = xr_new<IBannedClient>();
	pNewClient->HAddr = Address;
	time(&pNewClient->BanTime);
	pNewClient->BanTime += BanTimeSec;
	BannedAddresses.push_back(pNewClient);
	BannedList_Save();
}

void IPureServer::UnBanAddress(const ip_address& Address)
{
	for (u32 it = 0; it < BannedAddresses.size(); it++)
	{
		if (BannedAddresses[it]->HAddr == Address)
		{
			xr_delete(BannedAddresses[it]);
			BannedAddresses.erase(BannedAddresses.begin() + it);
			BannedList_Save();
			return;
		}
	}
	Msg("! Can't find address %s in ban list.", Address.to_string().c_str());
}

void IPureServer::Print_Banned_Addreses()
{
	Msg("- ----banned ip list begin-------");
	for (u32 i = 0; i < BannedAddresses.size(); i++)
	{
		IBannedClient* pBClient = BannedAddresses[i];
		Msg("- %s to %s", pBClient->HAddr.to_string().c_str(), pBClient->BannedTimeTo().c_str());
	}
	Msg("- ----banned ip list end-------");
}

LPCSTR IPureServer::GetBannedListName()
{
	return "banned_list_ip.ltx";
}

void IPureServer::BannedList_Save()
{
	string_path temp;
	FS.update_path(temp, "$app_data_root$", GetBannedListName());
	CInifile ini(temp, FALSE, FALSE, TRUE);
	for (u32 it = 0; it < BannedAddresses.size(); it++)
		BannedAddresses[it]->Save(ini);
}

void IPureServer::BannedList_Load()
{
	string_path temp;
	FS.update_path(temp, "$app_data_root$", GetBannedListName());
	CInifile ini(temp);

	for (auto it = ini.sections().begin(); it != ini.sections().end(); ++it)
	{
		IBannedClient* Cl = xr_new<IBannedClient>();
		Cl->Load(ini, it->Name);
		BannedAddresses.push_back(Cl);
	}
}

void IPureServer::IpList_Load()
{
	Msg("* Initializing IP filter.");
	m_ip_filter.load();
}

void IPureServer::IpList_Unload()
{
	Msg("* Deinitializing IP filter.");
	m_ip_filter.unload();
}

bool IPureServer::IsPlayerIPDenied(u32 ip_address_val)
{
	return !m_ip_filter.is_ip_present(ip_address_val);
}

static bool banned_client_comparer(IBannedClient* C1, IBannedClient* C2)
{
	return C1->BanTime > C2->BanTime;
}

void IPureServer::UpdateBannedList()
{
	if (BannedAddresses.empty())
		return;
	std::sort(BannedAddresses.begin(), BannedAddresses.end(), banned_client_comparer);

	time_t T;
	time(&T);

	IBannedClient* Cl = BannedAddresses.back();
	if (Cl->BanTime < T)
		UnBanAddress(Cl->HAddr);
}
