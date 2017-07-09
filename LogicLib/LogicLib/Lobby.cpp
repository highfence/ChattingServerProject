#include <memory>
#include "../../Common/ErrorCode.h"
#include "Room.h"
#include "Lobby.h"

namespace LogicLib
{
	Lobby::Lobby()
	{
	}

	Lobby::~Lobby()
	{
	}

	void Lobby::Init(const short lobbyIndex, const short maxLobbyUserCount, const short maxRoomCoutByLobby, const short maxRoomUserCount)
	{
		_lobbyIndex = lobbyIndex;
		_maxUserCount = static_cast<short>(maxLobbyUserCount);

		for (int i = 0; i < maxLobbyUserCount; ++i)
		{
			LobbyUser lobbyUser;
			lobbyUser.Index = static_cast<short>(i);
			lobbyUser.pUser = nullptr;

			_userList.emplace_back(std::move(lobbyUser));
		}

		for (int i = 0; i < maxRoomCoutByLobby; ++i)
		{
			_roomList.emplace_back(new Room());
			_roomList[i]->Init(static_cast<short>(i), maxRoomUserCount);
		}
	}

	void Lobby::Release()
	{
		for (int i = 0; i < static_cast<int>(_roomList.size()); ++i)
		{
			delete _roomList[i];
		}

		_roomList.clear();
	}

	void Lobby::SetNetwork(TcpNet * pNetwork, Logger * pLogger)
	{
		_refNetwork = pNetwork;
		_refLogger = pLogger;

		for (auto pRoom : _roomList)
		{
			pRoom->SetNetwork(pNetwork, pLogger);
		}
	}
	
	ERROR_CODE Lobby::EnterUser(User * pUser)
	{
		if (_userIndexDic.size() >= _maxUserCount)
		{
			return ERROR_CODE::LOBBY_ENTER_MAX_USER_COUNT;
		}

		if (findUser(pUser->GetIndex()) != nullptr)
		{
			return ERROR_CODE::LOBBY_ENTER_USER_DUPLICATION;
		}

		auto addRet = addUser(pUser);
		if (addRet != ERROR_CODE::NONE)
		{
			return addRet;
		}

		pUser->EnterLobby(_lobbyIndex);

		_userIndexDic.insert({ pUser->GetIndex(), pUser });
		_userIdDic.insert({ pUser->GetId().c_str(), pUser });

		return ERROR_CODE::NONE;
	}

	ERROR_CODE Lobby::LeaveUser(const int userIndex)
	{
		removeUser(userIndex);

		auto pUser = findUser(userIndex);

		if (pUser == nullptr)
		{
			return ERROR_CODE::LOBBY_LEAVE_USER_NVALID_UNIQUEINDEX;
		}

		pUser->LeaveLobby();

		_userIndexDic.erase(pUser->GetIndex());
		_userIdDic.erase(pUser->GetId().c_str());

		return ERROR_CODE::NONE;
	}
	
	short Lobby::GetUserCount()
	{
		return 0;
	}
	void Lobby::NotifyLobbyEnterUserInfo(User * pUser)
	{
	}
	ERROR_CODE Lobby::SendRoomList(const int sessionId, const short startRoomId)
	{
		return ERROR_CODE();
	}
	ERROR_CODE Lobby::SendUserList(const int sessionId, const short startUserIndex)
	{
		return ERROR_CODE();
	}
	void Lobby::NotifyLobbyLeaveUserInfo(User * pUser)
	{
	}
	Room * Lobby::CreateRoom()
	{
		return nullptr;
	}
	Room * Lobby::GetRoom(const short roomIndex)
	{
		return nullptr;
	}
	void Lobby::NotifyChangedRoomInfo(const short roomIndex)
	{
	}
	void Lobby::NotifyChat(const int sessionIndex, const char * pszUserId, const wchar_t * pszMsg)
	{
	}
	void Lobby::sendToAllUser(const short packetId, const short dataSize, char * pData, const int passUserIndex)
	{
	}
	User * Lobby::findUser(const int userIndex)
	{
		return nullptr;
	}
	ERROR_CODE Lobby::addUser(User * pUser)
	{
		return ERROR_CODE();
	}
	void Lobby::removeUser(const int userIndex)
	{
	}
}