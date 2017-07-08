#pragma once

#include <memory>
#include "../Common/Packet.h"
#include "../NetworkLib/Define.h"
#include "../Common/ErrorCode.h"

using ERROR_CODE = NCommon::ERROR_CODE;

namespace NetworkLib
{
	class ITcpNetwork;
	class ILog;
}

namespace LogicLib
{	
	class ConnectedUserManager;
	class UserManager;
	class LobbyManager;

	using ServerConfig = NetworkLib::ServerConfig;


	#define CHECK_START  ERROR_CODE __result=ERROR_CODE::NONE;
	#define CHECK_ERROR(f) __result=f; goto CHECK_ERR;

	class PacketProcess
	{
		using PacketInfo = NetworkLib::RecvPacketInfo;
		typedef ERROR_CODE(PacketProcess::*PacketFunc)(PacketInfo);
		PacketFunc PacketFuncArray[(int)NCommon::PACKET_ID::MAX];

		using TcpNet = NetworkLib::ITcpNetwork;
		using ILog = NetworkLib::ILog;

	public:
		PacketProcess();
		~PacketProcess();

		void Init(TcpNet* pNetwork, UserManager* pUserMgr, LobbyManager* pLobbyMgr, ServerConfig* pConfig, ILog* pLogger);

		void Process(PacketInfo packetInfo);

		void StateCheck();
	
	private:
		ILog* _refLogger;
		TcpNet* _refNetwork;
				
		UserManager* _refUserMgr;
		LobbyManager* _refLobbyMgr;

		std::unique_ptr<ConnectedUserManager> _connectedUserManager;
						
	private:
		ERROR_CODE ntfSysConnctSession(PacketInfo packetInfo);
		ERROR_CODE ntfSysCloseSession(PacketInfo packetInfo);
		
		ERROR_CODE login(PacketInfo packetInfo);
		
		ERROR_CODE lobbyList(PacketInfo packetInfo);

		ERROR_CODE lobbyEnter(PacketInfo packetInfo);

		ERROR_CODE lobbyRoomList(PacketInfo packetInfo);

		ERROR_CODE lobbyUserList(PacketInfo packetInfo);

		ERROR_CODE lobbyChat(PacketInfo packetInfo);

		ERROR_CODE lobbyLeave(PacketInfo packetInfo);

		ERROR_CODE roomEnter(PacketInfo packetInfo);

		ERROR_CODE roomLeave(PacketInfo packetInfo);

		ERROR_CODE roomChat(PacketInfo packetInfo);

		ERROR_CODE roomMasterGameStart(PacketInfo packetInfo);

		ERROR_CODE roomGameStart(PacketInfo packetInfo);



		ERROR_CODE devEcho(PacketInfo packetInfo);
	};
}