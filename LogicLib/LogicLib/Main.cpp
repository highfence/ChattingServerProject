

#include "../../NetworkLib/NetworkLib/ServerNetErrorCode.h"
#include "../../NetworkLib/NetworkLib/Logger.h"
#include "../../NetworkLib/NetworkLib/Define.h"
#include "../../NetworkLib/NetworkLib/TcpNetwork.h"
#include "../../Common/ErrorCode.h"

#include "UserManager.h"
#include "LobbyManager.h"
#include "PacketProcess.h"
#include "ConsoleLogger.h"
#include "Main.h"

using NET_ERROR_CODE = NetworkLib::NET_ERROR_CODE;
using LOG_TYPE = NetworkLib::LOG_TYPE;

namespace LogicLib
{
	Main::Main()
	{
	}

	Main::~Main()
	{
		release();
	}

	ERROR_CODE Main::Init()
	{
		_logger = std::make_unique<ConsoleLogger>();

		loadConfig();

		_network = std::make_unique<NetworkLib::TcpNetwork>();
		auto result = _network->Init(_serverConfig.get(), _logger.get());

		if (result != NET_ERROR_CODE::NONE)
		{
			_logger->Write(LOG_TYPE::L_ERROR, "%s | Init Fail. NetErrorCode(%s)", __FUNCTION__, (short)result);
			return ERROR_CODE::MAIN_INIT_NETWORK_INIT_FAIL;
		}
		
		_userManager = std::make_unique<UserManager>();
		_userManager->Init(_serverConfig->MaxClientCount);

		_lobbyManager = std::make_unique<LobbyManager>();
		_lobbyManager->Init({ _serverConfig->MaxLobbyCount,
			_serverConfig->MaxLobbyUserCount,
			_serverConfig->MaxRoomCountByLobby,
			_serverConfig->MaxRoomUserCount },
			_network.get(), _logger.get());

		_packetProcess = std::make_unique<PacketProcess>();
		_packetProcess->Init(_network.get(), _userManager.get(), _lobbyManager.get(), _serverConfig.get(), _logger.get());

		_isRun = true;

		_logger->Write(LOG_TYPE::L_INFO, "%s | Init Success. Server Run", __FUNCTION__);

		return ERROR_CODE::NONE;
	}

	void Main::Run()
	{
		while (_isRun)
		{
			_network->Run();

			while (true)
			{
				auto packetInfo = _network->GetPacketInfo();

				if (packetInfo.PacketId == 0)
				{
					break;
				}
				else
				{
					_packetProcess->Process(packetInfo);
				}
			}

			_packetProcess->StateCheck();
		}
	}

	void Main::Stop()
	{
		_isRun = false;
	}

	ERROR_CODE Main::loadConfig()
	{
		_serverConfig = std::make_unique<NetworkLib::ServerConfig>();

		wchar_t sPath[MAX_PATH] = { 0, };
		::GetCurrentDirectory(MAX_PATH, sPath);

		wchar_t inipath[MAX_PATH] = { 0, };
		_snwprintf_s(inipath, _countof(inipath), _TRUNCATE, L"%s\\ServerConfig.ini", sPath);

		_serverConfig->Port = (unsigned short)GetPrivateProfileInt(L"Config", L"Port", 0, inipath);
		_serverConfig->BackLogCount = GetPrivateProfileInt(L"Config", L"BackLogCount", 0, inipath);
		_serverConfig->MaxClientCount = GetPrivateProfileInt(L"Config", L"MaxClientCount", 0, inipath);

		_serverConfig->MaxClientSockOptRecvBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientSockOptRecvBufferSize", 0, inipath);
		_serverConfig->MaxClientSockOptSendBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientSockOptSendBufferSize", 0, inipath);
		_serverConfig->MaxClientRecvBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientRecvBufferSize", 0, inipath);
		_serverConfig->MaxClientSendBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientSendBufferSize", 0, inipath);

		_serverConfig->IsLoginCheck = GetPrivateProfileInt(L"Config", L"IsLoginCheck", 0, inipath) == 1 ? true : false;

		_serverConfig->ExtraClientCount = GetPrivateProfileInt(L"Config", L"ExtraClientCount", 0, inipath);
		_serverConfig->MaxLobbyCount = GetPrivateProfileInt(L"Config", L"MaxLobbyCount", 0, inipath);
		_serverConfig->MaxLobbyUserCount = GetPrivateProfileInt(L"Config", L"MaxLobbyUserCount", 0, inipath);
		_serverConfig->MaxRoomCountByLobby = GetPrivateProfileInt(L"Config", L"MaxRoomCountByLobby", 0, inipath);
		_serverConfig->MaxRoomUserCount = GetPrivateProfileInt(L"Config", L"MaxRoomUserCount", 0, inipath);

		_logger->Write(NetworkLib::LOG_TYPE::L_INFO, "%s | Port(%d), Backlog(%d)", __FUNCTION__, _serverConfig->Port, _serverConfig->BackLogCount);
		_logger->Write(NetworkLib::LOG_TYPE::L_INFO, "%s | IsLoginCheck(%d)", __FUNCTION__, _serverConfig->IsLoginCheck);
		return ERROR_CODE::NONE;;
	}

	void Main::release()
	{
		if (_network)
		{
			_network->Release();
		}
	}
}
