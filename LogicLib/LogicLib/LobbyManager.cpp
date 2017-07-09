#include "Lobby.h"
#include "LobbyManager.h"

namespace LogicLib
{
	LobbyManager::LobbyManager()
	{
	}
	LobbyManager::~LobbyManager()
	{
	}

	void LobbyManager::Init(const LobbyManagerConfig config, TcpNet * pNetwork, Logger * pLogger)
	{
	}

	Lobby * LobbyManager::GetLobby(short lobbyId)
	{
		return nullptr;
	}

	void LobbyManager::SendLobbyListInfo(const int sessionIndex)
	{
	}
}
