#pragma once
#include <vector>
#include <unordered_map>

namespace NetworkLib
{
	class TcpNetwork;
}

namespace NetworkLib
{
	class ILog;
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
		using ILog = NetworkLib::ILog;

	public:

		LobbyManager();
		virtual ~LobbyManager();

		void Init(const LobbyManagerConfig config, TcpNet* pNetwork, ILog* pLogger);

		Lobby* GetLobby(short lobbyId);


	public:

		void SendLobbyListInfo(const int sessionIndex);

	private:

		ILog* _refLogger;
		TcpNet* _refNetwork;

		std::vector<Lobby> _lobbyList;
		
	};
}