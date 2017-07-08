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

		ERROR_CODE loadConfig();

		void release();

	private:

		bool _isRun = false;

		std::unique_ptr<NetworkLib::ServerConfig> _serverConfig;
		std::unique_ptr<NetworkLib::ILog> _logger;

		std::unique_ptr<NetworkLib::ITcpNetwork> _network;
		std::unique_ptr<PacketProcess> _packetProc;
		std::unique_ptr<UserManager> _userMgr;
		std::unique_ptr<LobbyManager> _lobbyMgr;
		
	};
}
