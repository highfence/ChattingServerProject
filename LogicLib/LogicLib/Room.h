#pragma once
#include <vector>
#include <string>
#include <memory>

#include "User.h"

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

	class Game;

	class Room
	{
	public :
		Room();
		virtual ~Room();

		void Init(const short index, const short maxUserCount);

		void SetNetwork(TcpNet * pNetwork, Logger * pLogger);

		void Clear();

		short GetIndex() { return _index; }

		bool IsUsed() { return _isUsed; }

		const wchar_t * GetTitle() { return _title.c_str(); }

		short GetMaxUserCount() { return _maxUserCount; }

		short GetUserCount() { return static_cast<short>(_userList.size()); }

		ERROR_CODE CreateRoom(const wchar_t* pRoomTitle);

		ERROR_CODE EnterUser(User * pUser);

		ERROR_CODE LeaveUser(const short userIndex);

		bool IsMaster(const short userIndex);

		Game* GetGameObj();

		void Update();

		void SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserIndex = -1);

		void NotifyEnterUserInfo(const int userIndex, const char * pszUserId);

		void NotifyLeaveUserInfo(const char* pszUserId);

		void NotifyChat(const int sessionIndex, const char * pszUserId, const wchar_t * pszMsg);

	private :
		Logger * _refLogger;
		TcpNet * _refNetwork;

		short _index = -1;
		short _maxUserCount;

		bool _isUsed = false;
		std::wstring _title;
		std::vector<User*> _userList;

		Game* _game = nullptr;
	};
}
