#include <algorithm>

#include "../NetworkLib/ILog.h"
#include "../NetworkLib/TcpNetwork.h"
#include "../Common/Packet.h"
#include "../Common/PacketID.h"
#include "../Common/ErrorCode.h"

#include "User.h"
#include "Game.h"
#include "Room.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace LogicLib
{
	Room::Room() {}

	Room::~Room()
	{
		if (_game != nullptr) {
			delete _game;
		}
	}
	
	void Room::Init(const short index, const short maxUserCount)
	{
		_index = index;
		_maxUserCount = maxUserCount;

		_game = new Game;
	}

	void Room::SetNetwork(TcpNet* pNetwork, ILog* pLogger)
	{
		_refLogger = pLogger;
		_refNetwork = pNetwork;
	}

	void Room::Clear()
	{
		_isUsed = false;
		_title = L"";
		_userList.clear();
	}

	Game* Room::GetGameObj()
	{
		return _game;
	}

	ERROR_CODE Room::CreateRoom(const wchar_t* pRoomTitle)
	{
		if (_isUsed) {
			return ERROR_CODE::ROOM_ENTER_CREATE_FAIL;
		}

		_isUsed = true;
		_title = pRoomTitle;

		return ERROR_CODE::NONE;
	}

	ERROR_CODE Room::EnterUser(User* pUser)
	{
		if (_isUsed == false) {
			return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
		}

		if (_userList.size() == static_cast<unsigned int>(_maxUserCount)) {
			return ERROR_CODE::ROOM_ENTER_MEMBER_FULL;
		}

		_userList.push_back(pUser);
		return ERROR_CODE::NONE;
	}

	bool Room::IsMaster(const short userIndex)
	{
		return _userList[0]->GetIndex() == userIndex ? true : false;
	}

	ERROR_CODE Room::LeaveUser(const short userIndex)
	{
		if (_isUsed == false) {
			return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
		}

		auto iter = std::find_if(std::begin(_userList), std::end(_userList), [userIndex](auto pUser) { return pUser->GetIndex() == userIndex; });
		if (iter == std::end(_userList)) {
			return ERROR_CODE::ROOM_LEAVE_NOT_MEMBER;
		}
		
		_userList.erase(iter);

		if (_userList.empty()) 
		{
			Clear();
		}

		return ERROR_CODE::NONE;
	}

	void Room::SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex)
	{
		for (auto pUser : _userList)
		{
			if (pUser->GetIndex() == passUserindex) {
				continue;
			}

			_refNetwork->SendData(pUser->GetSessioIndex(), packetId, dataSize, pData);
		}
	}

	void Room::NotifyEnterUserInfo(const int userIndex, const char* pszUserID)
	{
		NCommon::PktRoomEnterUserInfoNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);

		SendToAllUser((short)PACKET_ID::ROOM_ENTER_USER_NTF, sizeof(pkt), (char*)&pkt, userIndex);
	}

	void Room::NotifyLeaveUserInfo(const char* pszUserID)
	{
		if (_isUsed == false) {
			return;
		}

		NCommon::PktRoomLeaveUserInfoNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);

		SendToAllUser((short)PACKET_ID::ROOM_LEAVE_USER_NTF, sizeof(pkt), (char*)&pkt);
	}

	void Room::NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg)
	{
		NCommon::PktRoomChatNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);
		wcsncpy_s(pkt.Msg, NCommon::MAX_ROOM_CHAT_MSG_SIZE + 1, pszMsg, NCommon::MAX_ROOM_CHAT_MSG_SIZE);

		SendToAllUser((short)PACKET_ID::ROOM_CHAT_NTF, sizeof(pkt), (char*)&pkt, sessionIndex);
	}

	void Room::Update()
	{
		if (_game->GetState() == GameState::ING)
		{
			if (_game->CheckSelectTime())
			{
				//선택 안하는 사람이 지도록 한
			}
		}
	}
}