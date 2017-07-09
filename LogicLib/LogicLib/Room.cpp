#include <algorithm>
#include "../../Common/ErrorCode.h"
#include "../../Common/Packet.h"
#include "../../Common/PacketID.h"
#include "../../NetworkLib/NetworkLib/TcpNetwork.h"
#include "Game.h"
#include "Room.h"

namespace LogicLib
{
	Room::Room() {}

	Room::~Room()
	{
		if (_game != nullptr)
		{
			delete _game;
		}
	}

	void Room::Init(const short index, const short maxUserCount)
	{
		_index = index;
		_maxUserCount = maxUserCount;
		_game = new Game();
	}

	void Room::SetNetwork(TcpNet * pNetwork, Logger * pLogger)
	{
		_refNetwork = pNetwork;
		_refLogger = pLogger;
	}

	void Room::Clear()
	{
		_isUsed = false;
		_title = L"";
		_userList.clear();
	}

	ERROR_CODE Room::CreateRoom(const wchar_t * pRoomTitle)
	{
		if (_isUsed)
		{
			return ERROR_CODE::ROOM_ENTER_CREATE_FAIL;
		}

		_isUsed = true;
		_title = pRoomTitle;

		return ERROR_CODE::NONE;
	}

	ERROR_CODE Room::EnterUser(User * pUser)
	{
		if (_isUsed == false)
		{
			return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
		}

		if (_userList.size() == _maxUserCount)
		{
			return ERROR_CODE::ROOM_ENTER_MEMBER_FULL;
		}

		_userList.push_back(pUser);
		return ERROR_CODE::NONE;
	}

	ERROR_CODE Room::LeaveUser(const short userIndex)
	{
		if (_isUsed == false)
		{
			return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
		}

		auto iter = std::find_if(std::begin(_userList), std::end(_userList), [userIndex](auto pUser) { return pUser->GetIndex() == userIndex; });
		if (iter == std::end(_userList))
		{
			return ERROR_CODE::ROOM_LEAVE_NOT_MEMBER;
		}

		_userList.erase(iter);

		if (_userList.empty())
		{
			Clear();
		}

		return ERROR_CODE::NONE;
	}

	bool Room::IsMaster(const short userIndex)
	{
		return _userList[0]->GetIndex() == userIndex ? true : false;
	}

	Game * Room::GetGameObj()
	{
		return _game;
	}

	void Room::Update()
	{
		if (_game->GetState() == GameState::ING)
		{
			if (_game->CheckSelectTime())
			{

			}
		}
	}

	void Room::SendToAllUser(const short packetId, const short dataSize, char * pData, const int passUserIndex)
	{
		for (auto pUser : _userList)
		{
			if (pUser->GetIndex() == passUserIndex)
			{
				continue;
			}

			_refNetwork->SendData(pUser->GetSessionIndex(), packetId, dataSize, pData);
		}
	}

	void Room::NotifyEnterUserInfo(const int userIndex, const char * pszUserId)
	{
		NCommon::PktRoomLeaveUserInfoNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserId, NCommon::MAX_USER_ID_SIZE);

		SendToAllUser(static_cast<short>(NCommon::PACKET_ID::ROOM_ENTER_USER_NTF), sizeof(pkt), reinterpret_cast<char*>(&pkt), userIndex);
	}

	void Room::NotifyLeaveUserInfo(const char * pszUserId)
	{
		if (_isUsed == false)
		{
			return;
		}

		NCommon::PktRoomLeaveUserInfoNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserId, NCommon::MAX_USER_ID_SIZE);

		SendToAllUser(static_cast<short>(NCommon::PACKET_ID::ROOM_LEAVE_USER_NTF), sizeof(pkt), reinterpret_cast<char*>(&pkt));
	}

	void Room::NotifyChat(const int sessionIndex, const char * pszUserId, const wchar_t * pszMsg)
	{
		NCommon::PktRoomChatNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserId, NCommon::MAX_USER_ID_SIZE);
		wcsncpy_s(pkt.Msg, NCommon::MAX_ROOM_CHAT_MSG_SIZE + 1, pszMsg, NCommon::MAX_ROOM_CHAT_MSG_SIZE);

		SendToAllUser(static_cast<short>(NCommon::PACKET_ID::ROOM_CHAT_NTF), sizeof(pkt), reinterpret_cast<char*>(&pkt), sessionIndex);
	}
}
