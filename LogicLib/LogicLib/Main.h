#pragma once
#include <memory>
#include "../../Common/ErrorCode.h"

using ERROR_CODE = NCommon::ERROR_CODE;

namespace NetworkLib
{
	struct ServerConfig;
	class Logger;
	class ITcpNetwork;
}

namespace LogicLib
{
	class UserManager;
	class LobbyManager;
	class PacketProcess;

	class Main
	{
	public :
		Main();
		~Main();

		ERROR_CODE Init();

		void Run();

		void Stop();

	private :

		ERROR_CODE loadConfig();

		void release();

	private :
		bool _isRun = false;

		std::unique_ptr<NetworkLib::ServerConfig> _serverConfig;
		std::unique_ptr<NetworkLib::Logger> _logger;

		std::unique_ptr<NetworkLib::ITcpNetwork> _network;
		std::unique_ptr<UserManager> _userManager;
		std::unique_ptr<LobbyManager> _lobbyManager;
		std::unique_ptr<PacketProcess> _packetProcess;
	};

}