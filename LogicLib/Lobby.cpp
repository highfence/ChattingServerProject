#include <algorithm>

#include "../NetworkLib/ILog.h"
#include "../NetworkLib/TcpNetwork.h"
#include "../Common/Packet.h"
#include "../Common/ErrorCode.h"
#include "User.h"
#include "Room.h"
#include "Lobby.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace LogicLib
{
	Lobby::Lobby() {}

	Lobby::~Lobby() {}

	void Lobby::Init(const short lobbyIndex, const short maxLobbyUserCount, const short maxRoomCountByLobby, const short maxRoomUserCount)
	{
		_lobbyIndex = lobbyIndex;
		_maxUserCount = (short)maxLobbyUserCount;

		for (int i = 0; i < maxLobbyUserCount; ++i)
		{
			LobbyUser lobbyUser;
			lobbyUser.Index = (short)i;
			lobbyUser.pUser = nullptr;

			_userList.push_back(lobbyUser);
		}

		for (int i = 0; i < maxRoomCountByLobby; ++i)
		{
			_roomList.emplace_back(new Room());
			_roomList[i]->Init((short)i, maxRoomUserCount);
		}
	}

	void Lobby::Release()
	{
		for (int i = 0; i < (int)_roomList.size(); ++i)
		{
			delete _roomList[i];
		}

		_roomList.clear();
	}

	void Lobby::SetNetwork(TcpNet* pNetwork, ILog* pLogger)
	{
		_refLogger = pLogger;
		_refNetwork = pNetwork;

		for (auto pRoom : _roomList)
		{
			pRoom->SetNetwork(pNetwork, pLogger);
		}
	}

	ERROR_CODE Lobby::EnterUser(User* pUser)
	{
		if (_userIndexDic.size() >= static_cast<unsigned int>(_maxUserCount)) {
			return ERROR_CODE::LOBBY_ENTER_MAX_USER_COUNT;
		}

		if (findUser(pUser->GetIndex()) != nullptr) {
			return ERROR_CODE::LOBBY_ENTER_USER_DUPLICATION;
		}

		auto addRet = addUser(pUser);
		if (addRet != ERROR_CODE::NONE) {
			return addRet;
		}

		pUser->EnterLobby(_lobbyIndex);

		_userIndexDic.insert({ pUser->GetIndex(), pUser });
		_userIDDic.insert({ pUser->GetID().c_str(), pUser });

		return ERROR_CODE::NONE;
	}

	ERROR_CODE Lobby::LeaveUser(const int userIndex)
	{
		removeUser(userIndex);

		auto pUser = findUser(userIndex);

		if (pUser == nullptr) {
			return ERROR_CODE::LOBBY_LEAVE_USER_NVALID_UNIQUEINDEX;
		}

		pUser->LeaveLobby();

		_userIndexDic.erase(pUser->GetIndex());
		_userIDDic.erase(pUser->GetID().c_str());
		
		return ERROR_CODE::NONE;
	}
		
	User* Lobby::findUser(const int userIndex)
	{
		auto findIter = _userIndexDic.find(userIndex);

		if (findIter == _userIndexDic.end()) {
			return nullptr;
		}

		return (User*)findIter->second;
	}

	ERROR_CODE Lobby::addUser(User* pUser)
	{
		auto findIter = std::find_if(std::begin(_userList), std::end(_userList), [](auto& lobbyUser) { return lobbyUser.pUser == nullptr; });
		
		if (findIter == std::end(_userList)) {
			return ERROR_CODE::LOBBY_ENTER_EMPTY_USER_LIST;
		}

		findIter->pUser = pUser;
		return ERROR_CODE::NONE;
	}

	void Lobby::removeUser(const int userIndex)
	{
		auto findIter = std::find_if(std::begin(_userList), std::end(_userList), [userIndex](auto& lobbyUser) 
		{
			return lobbyUser.pUser != nullptr && lobbyUser.pUser->GetIndex() == userIndex;
		});

		if (findIter == std::end(_userList)) {
			return;
		}

		findIter->pUser = nullptr;
	}

	short Lobby::GetUserCount()
	{ 
		return static_cast<short>(_userIndexDic.size()); 
	}


	void Lobby::NotifyLobbyEnterUserInfo(User* pUser)
	{
		NCommon::PktLobbyNewUserInfoNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pUser->GetID().c_str(), NCommon::MAX_USER_ID_SIZE);

		sendToAllUser((short)PACKET_ID::LOBBY_ENTER_USER_NTF, sizeof(pkt), (char*)&pkt, pUser->GetIndex());
	}

	void Lobby::NotifyLobbyLeaveUserInfo(User* pUser)
	{
		NCommon::PktLobbyLeaveUserInfoNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pUser->GetID().c_str(), NCommon::MAX_USER_ID_SIZE);

		sendToAllUser((short)PACKET_ID::LOBBY_LEAVE_USER_NTF, sizeof(pkt), (char*)&pkt, pUser->GetIndex());
	}

	ERROR_CODE Lobby::SendRoomList(const int sessionId, const short startRoomId)
	{
		if (startRoomId < 0 || static_cast<unsigned int>(startRoomId) >= (_roomList.size() - 1)) {
			return ERROR_CODE::LOBBY_ROOM_LIST_INVALID_START_ROOM_INDEX;
		}

		NCommon::PktLobbyRoomListRes pktRes;
		short roomCount = 0;
		int lastCheckedIndex = 0;

		for (unsigned int i = startRoomId; i < _roomList.size(); ++i)
		{
			auto pRoom = _roomList[i];
			lastCheckedIndex = i;

			if (pRoom->IsUsed() == false) {
				continue;
			}

			pktRes.RoomInfo[roomCount].RoomIndex = pRoom->GetIndex();
			pktRes.RoomInfo[roomCount].RoomUserCount = pRoom->GetUserCount();
			wcsncpy_s(pktRes.RoomInfo[roomCount].RoomTitle, NCommon::MAX_ROOM_TITLE_SIZE + 1, pRoom->GetTitle(), NCommon::MAX_ROOM_TITLE_SIZE);
			
			++roomCount;

			if (roomCount >= NCommon::MAX_NTF_LOBBY_ROOM_LIST_COUNT) {
				break;
			}
		}

		pktRes.Count = roomCount;

		if (roomCount <= 0 || static_cast<unsigned int>(lastCheckedIndex + 1) == _roomList.size()) {
			pktRes.IsEnd = true;
		}

		_refNetwork->SendData(sessionId, (short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_RES, sizeof(pktRes), (char*)&pktRes);

		return ERROR_CODE::NONE;
	}

	ERROR_CODE Lobby::SendUserList(const int sessionId, const short startUserIndex)
	{
		if (startUserIndex < 0 || static_cast<unsigned int>(startUserIndex) >= (_userList.size() - 1)) {
			return ERROR_CODE::LOBBY_USER_LIST_INVALID_START_USER_INDEX;
		}

		int lastCheckedIndex = 0;
		NCommon::PktLobbyUserListRes pktRes;
		short userCount = 0;

		for (unsigned int i = startUserIndex; i < _userList.size(); ++i)
		{
			auto& lobbyUser = _userList[i];
			lastCheckedIndex = i;

			if (lobbyUser.pUser == nullptr || lobbyUser.pUser->IsCurDoServerHostInLobby() == false) {
				continue;
			}

			pktRes.UserInfo[userCount].LobbyUserIndex = (short)i;
			strncpy_s(pktRes.UserInfo[userCount].UserID, NCommon::MAX_USER_ID_SIZE + 1, lobbyUser.pUser->GetID().c_str(), NCommon::MAX_USER_ID_SIZE);

			++userCount;

			if (userCount >= NCommon::MAX_SEND_LOBBY_USER_LIST_COUNT) {
				break;
			}
		}

		pktRes.Count = userCount;

		if (userCount <= 0 || static_cast<unsigned int>(lastCheckedIndex + 1) == _userList.size()) {
			pktRes.IsEnd = true;
		}

		_refNetwork->SendData(sessionId, (short)PACKET_ID::LOBBY_ENTER_USER_LIST_RES, sizeof(pktRes), (char*)&pktRes);

		return ERROR_CODE::NONE;
	}

	void Lobby::sendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex)
	{
		for (auto& pUser : _userIndexDic)
		{
			if (pUser.second->GetIndex() == passUserindex) {
				continue;
			}

			if (pUser.second->IsCurDoServerHostInLobby() == false) {
				continue;
			}

			_refNetwork->SendData(pUser.second->GetSessioIndex(), packetId, dataSize, pData);
		}
	}

	Room* Lobby::CreateRoom()
	{
		for (int i = 0; i < (int)_roomList.size(); ++i)
		{
			if (_roomList[i]->IsUsed() == false) {
				return _roomList[i];
			}
		}
		return nullptr;
	}

	Room* Lobby::GetRoom(const short roomIndex)
	{
		if (roomIndex < 0 || static_cast<unsigned int>(roomIndex) >= _roomList.size()) {
			return nullptr;
		}

		return _roomList[roomIndex];
	}

	void Lobby::NotifyChangedRoomInfo(const short roomIndex)
	{
		NCommon::PktChangedRoomInfoNtf pktNtf;
				
		auto pRoom = _roomList[roomIndex];
		
		pktNtf.RoomInfo.RoomIndex = pRoom->GetIndex();
		pktNtf.RoomInfo.RoomUserCount = pRoom->GetUserCount();

		if (_roomList[roomIndex]->IsUsed()) {
			wcsncpy_s(pktNtf.RoomInfo.RoomTitle, NCommon::MAX_ROOM_TITLE_SIZE + 1, pRoom->GetTitle(), NCommon::MAX_ROOM_TITLE_SIZE);
		}
		else {
			pktNtf.RoomInfo.RoomTitle[0] = L'\0';
		}

		sendToAllUser((short)PACKET_ID::ROOM_CHANGED_INFO_NTF, sizeof(pktNtf), (char*)&pktNtf);
	}

	void Lobby::NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg)
	{
		NCommon::PktLobbyChatNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);
		wcsncpy_s(pkt.Msg, NCommon::MAX_LOBBY_CHAT_MSG_SIZE + 1, pszMsg, NCommon::MAX_LOBBY_CHAT_MSG_SIZE);

		sendToAllUser((short)PACKET_ID::LOBBY_CHAT_NTF, sizeof(pkt), (char*)&pkt, sessionIndex);
	}
}
