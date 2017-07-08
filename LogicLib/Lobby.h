#pragma once

#include <vector>
#include <unordered_map>

//#include "Room.h"

namespace NetworkLib
{
	class ITcpNetwork;
}

namespace NetworkLib
{
	class ILog;
}

namespace NCommon
{
	enum class ERROR_CODE :short;
}
using ERROR_CODE = NCommon::ERROR_CODE;

namespace LogicLib
{
	using TcpNet = NetworkLib::ITcpNetwork;
	using ILog = NetworkLib::ILog;

	class User;
	class Room;
	
	struct LobbyUser
	{
		short Index = 0;
		User* pUser = nullptr;
	};

	class Lobby
	{
	public:
		Lobby();
		virtual ~Lobby();

		void Init(const short lobbyIndex, const short maxLobbyUserCount, const short maxRoomCountByLobby, const short maxRoomUserCount);
		
		void Release();

		void SetNetwork(TcpNet* pNetwork, ILog* pLogger);

		short GetIndex() { return _lobbyIndex; }


		ERROR_CODE EnterUser(User* pUser);
		ERROR_CODE LeaveUser(const int userIndex);
		
		short GetUserCount();

		
		void NotifyLobbyEnterUserInfo(User* pUser);
		
		ERROR_CODE SendRoomList(const int sessionId, const short startRoomId);

		ERROR_CODE SendUserList(const int sessionId, const short startUserIndex);

		void NotifyLobbyLeaveUserInfo(User* pUser);

		Room* CreateRoom();

		Room* GetRoom(const short roomIndex);

		void NotifyChangedRoomInfo(const short roomIndex);

		auto MaxUserCount() { return (short)_maxUserCount; }

		auto MaxRoomCount() { return (short)_roomList.size(); }

		void NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg);

	protected:

		void sendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex = -1);

	protected:

		User* findUser(const int userIndex);

		ERROR_CODE addUser(User* pUser);

		void removeUser(const int userIndex);


	protected:
		ILog* _refLogger;
		TcpNet* _refNetwork;


		short _lobbyIndex = 0;

		short _maxUserCount = 0;
		std::vector<LobbyUser> _userList;
		std::unordered_map<int, User*> _userIndexDic;
		std::unordered_map<const char*, User*> _userIDDic;

		std::vector<Room*> _roomList;
	};
}

