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
		ILog* m_pRefLogger;
		TcpNet* m_pRefNetwork;
				
		UserManager* m_pRefUserMgr;
		LobbyManager* m_pRefLobbyMgr;

		std::unique_ptr<ConnectedUserManager> m_pConnectedUserManager;
						
	private:
		ERROR_CODE NtfSysConnctSession(PacketInfo packetInfo);
		ERROR_CODE NtfSysCloseSession(PacketInfo packetInfo);
		
		ERROR_CODE Login(PacketInfo packetInfo);
		
		ERROR_CODE LobbyList(PacketInfo packetInfo);

		ERROR_CODE LobbyEnter(PacketInfo packetInfo);

		ERROR_CODE LobbyRoomList(PacketInfo packetInfo);

		ERROR_CODE LobbyUserList(PacketInfo packetInfo);

		ERROR_CODE LobbyChat(PacketInfo packetInfo);

		ERROR_CODE LobbyLeave(PacketInfo packetInfo);

		ERROR_CODE RoomEnter(PacketInfo packetInfo);

		ERROR_CODE RoomLeave(PacketInfo packetInfo);

		ERROR_CODE RoomChat(PacketInfo packetInfo);

		ERROR_CODE RoomMasterGameStart(PacketInfo packetInfo);

		ERROR_CODE RoomGameStart(PacketInfo packetInfo);



		ERROR_CODE DevEcho(PacketInfo packetInfo);
	};
}