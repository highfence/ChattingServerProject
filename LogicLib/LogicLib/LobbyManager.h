#pragma once
#include <vector>
#include <unordered_map>

namespace NetworkLib
{
	class ITcpNetwork;
	class Logger;
}


namespace LogicLib
{
	struct LobbyManagerConfig
	{
		int MaxLobbyCount;
		int MaxLobbyUserCount;
		int MaxRoomCountByLobby;
		int MaxRoomUserCount;
	};

	struct LobbySmallInfo
	{
		short Num;
		short UserCount;
	};

	class Lobby;

	class LobbyManager
	{
		using TcpNet = NetworkLib::ITcpNetwork;
		using Logger = NetworkLib::Logger;

	public :
		LobbyManager();
		virtual ~LobbyManager();

		void Init(const LobbyManagerConfig config, TcpNet * pNetwork, Logger * pLogger);

		Lobby * GetLobby(short lobbyId);

		void SendLobbyListInfo(const int sessionIndex);

	private :

		Logger * _refLogger;
		TcpNet * _refNetwork;

		std::vector<Lobby> _lobbyList;

	};
}