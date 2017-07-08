#pragma once

#include <vector>
#include <string>
#include <memory>

#include "User.h"


namespace NetworkLib { class ITcpNetwork; }
namespace NetworkLib { class ILog; }
namespace NCommon { enum class ERROR_CODE :short;}

using ERROR_CODE = NCommon::ERROR_CODE;

namespace LogicLib
{	
	using TcpNet = NetworkLib::ITcpNetwork;
	using ILog = NetworkLib::ILog;

	class Game;

	class Room
	{
	public:
		Room();
		virtual ~Room();

		void Init(const short index, const short maxUserCount);

		void SetNetwork(TcpNet* pNetwork, ILog* pLogger);

		void Clear();
		
		short GetIndex() { return _index;  }

		bool IsUsed() { return _isUsed; }

		const wchar_t* GetTitle() { return _title.c_str(); }

		short MaxUserCount() { return _maxUserCount; }

		short GetUserCount() { return (short)_userList.size(); }

		ERROR_CODE CreateRoom(const wchar_t* pRoomTitle);

		ERROR_CODE EnterUser(User* pUser);

		ERROR_CODE LeaveUser(const short userIndex);

		bool IsMaster(const short userIndex);

		Game* GetGameObj();

		void Update();

		void SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex = -1);

		void NotifyEnterUserInfo(const int userIndex, const char* pszUserID);

		void NotifyLeaveUserInfo(const char* pszUserID);

		void NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg);

	private:

		ILog* _refLogger;
		TcpNet* _refNetwork;

		short _index = -1;
		short _maxUserCount;
		
		bool _isUsed = false;
		std::wstring _title;
		std::vector<User*> _userList;

		Game* _game = nullptr;
	};
}