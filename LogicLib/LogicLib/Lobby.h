#pragma once

#include <vector>
#include <unordered_map>

namespace NetworkLib
{
	class ITcpNetwork;
	class Logger;
}

namespace NCommon
{
	enum class ERROR_CODE : short;
}

using ERROR_CODE = NCommon::ERROR_CODE;

namespace LogicLib
{
	using TcpNet = NetworkLib::ITcpNetwork;
	using Logger = NetworkLib::Logger;

	class User;
	class Room;

	struct LobbyUser
	{
		short Index = 0;
		User * pUser = nullptr;
	};

	class Lobby
	{
	public :
		Lobby();
		virtual ~Lobby();

		void Init(
			const short lobbyIndex,
			const short maxLobbyUserCount,
			const short maxRoomCoutByLobby,
			const short maxRoomUserCount);

		void Release();

		void SetNetwork(TcpNet * pNetwork, Logger * pLogger);

		short GetIndex() { return _lobbyIndex; }

		ERROR_CODE EnterUser(User * pUser);
		ERROR_CODE LeaveUser(const int userIndex);

		short GetUserCount();

		void NotifyLobbyEnterUserInfo(User * pUser);

		ERROR_CODE SendRoomList(const int sessionId, const short startRoomId);

		ERROR_CODE SendUserList(const int sessionId, const short startUserIndex);

		void NotifyLobbyLeaveUserInfo(User * pUser);

		Room * CreateRoom();

		Room * GetRoom(const short roomIndex);

		void NotifyChangedRoomInfo(const short roomIndex);

		short GetMaxUserCount() { return static_cast<short>(_maxUserCount); }
		
		short GetMaxRoomCount() { return static_cast<short>(_roomList.size()); }

		void NotifyChat(const int sessionIndex, const char * pszUserId, const wchar_t * pszMsg);

	protected :

		void sendToAllUser(const short packetId, const short dataSize, char * pData, const int passUserIndex = -1);

		User * findUser(const int userIndex);

		ERROR_CODE addUser(User * pUser);

		void removeUser(const int userIndex);

	protected :

		Logger * _refLogger;
		TcpNet * _refNetwork;

		short _lobbyIndex = 0;
		short _maxUserCount = 0;

		std::vector<LobbyUser> _userList;
		std::unordered_map<int, User*> _userIndexDic;
		std::unordered_map<const char*, User*> _userIdDic;

		std::vector<Room*> _roomList;
	};
}
