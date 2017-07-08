#pragma once
#include <memory>

#include "../Common/Packet.h"
#include "../Common/ErrorCode.h"

using ERROR_CODE = NCommon::ERROR_CODE;

namespace NetworkLib
{
	struct ServerConfig;
	class ILog;
	class ITcpNetwork;
}

namespace LogicLib
{
	class UserManager;
	class LobbyManager;
	class PacketProcess;

	class Main
	{
	public:
		Main();
		~Main();

		ERROR_CODE Init();

		void Run();

		void Stop();


	private:
		ERROR_CODE LoadConfig();

		void Release();


	private:
		bool m_IsRun = false;

		std::unique_ptr<NetworkLib::ServerConfig> m_pServerConfig;
		std::unique_ptr<NetworkLib::ILog> m_pLogger;

		std::unique_ptr<NetworkLib::ITcpNetwork> m_pNetwork;
		std::unique_ptr<PacketProcess> m_pPacketProc;
		std::unique_ptr<UserManager> m_pUserMgr;
		std::unique_ptr<LobbyManager> m_pLobbyMgr;
		
	};
}
