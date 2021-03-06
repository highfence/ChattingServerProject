#include "../Common/Packet.h"
#include "../NetworkLib/TcpNetwork.h"
#include "../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "LobbyManager.h"
#include "Lobby.h"
#include "Game.h"
#include "Room.h"
#include "PacketProcess.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace LogicLib
{
	ERROR_CODE PacketProcess::roomEnter(PacketInfo packetInfo)
	{
	CHECK_START
		auto reqPkt = (NCommon::PktRoomEnterReq*)packetInfo.pRefData;
		NCommon::PktRoomEnterRes resPkt;

		auto pUserRet = _refUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDoServerHostInLobby() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_DOServerHost);
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = _refLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
		}
		
		Room* pRoom = nullptr;
		
		// 룸을 만드는 경우라면 룸을 만든다
		if (reqPkt->IsCreate)
		{
			pRoom = pLobby->CreateRoom();
			if (pRoom == nullptr) {
				CHECK_ERROR(ERROR_CODE::ROOM_ENTER_EMPTY_ROOM);
			}

			auto ret = pRoom->CreateRoom(reqPkt->RoomTitle);
			if (ret != ERROR_CODE::NONE) {
				CHECK_ERROR(ret);
			}
		}
		else
		{
		    pRoom = pLobby->GetRoom(reqPkt->RoomIndex);
			if (pRoom == nullptr) {
				CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
			}
		}


		auto enterRet = pRoom->EnterUser(pUser);
		if (enterRet != ERROR_CODE::NONE) {
			CHECK_ERROR(enterRet);
		}
		
		// 유저 정보를 룸에 들어왔다고 변경한다.
		pUser->EnterRoom(lobbyIndex, pRoom->GetIndex());

		// 로비에 유저가 나갔음을 알린다
		pLobby->NotifyLobbyLeaveUserInfo(pUser);
		
		// 로비에 룸 정보를 통보한다.
		pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

		// 룸에 새 유저 들어왔다고 알린다
		pRoom->NotifyEnterUserInfo(pUser->GetIndex(), pUser->GetID().c_str());
		
		_refNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		_refNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt), (char*)&resPkt);
		return __result;
	}

	ERROR_CODE PacketProcess::roomLeave(PacketInfo packetInfo)
	{
	CHECK_START
		NCommon::PktRoomLeaveRes resPkt;

		auto pUserRet = _refUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);
		auto userIndex = pUser->GetIndex();

		if (pUser->IsCurDoServerHostInRoom() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_LEAVE_INVALID_DOServerHost);
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = _refLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
		}

		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

		auto leaveRet = pRoom->LeaveUser(userIndex);
		if (leaveRet != ERROR_CODE::NONE) {
			CHECK_ERROR(leaveRet);
		}
		
		// 유저 정보를 로비로 변경
		pUser->EnterLobby(lobbyIndex);

		// 룸에 유저가 나갔음을 통보
		pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());

		// 로비에 새로운 유저가 들어왔음을 통보
		pLobby->NotifyLobbyEnterUserInfo(pUser);

		// 로비에 바뀐 방 정보를 통보
		pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());
		
		_refNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		_refNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return __result;
	}

	ERROR_CODE PacketProcess::roomChat(PacketInfo packetInfo)
	{
	CHECK_START
		auto reqPkt = (NCommon::PktRoomChatReq*)packetInfo.pRefData;
		NCommon::PktRoomChatRes resPkt;

		auto pUserRet = _refUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);
		
		if (pUser->IsCurDoServerHostInRoom() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOServerHost);
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = _refLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);
		}

		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

		pRoom->NotifyChat(pUser->GetSessioIndex(), pUser->GetID().c_str(), reqPkt->Msg);
				
		_refNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		_refNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
		return __result;
	}

	ERROR_CODE PacketProcess::roomMasterGameStart(PacketInfo packetInfo)
	{
	CHECK_START
		NCommon::PktRoomMaterGameStartRes resPkt;

		auto pUserRet = _refUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDoServerHostInRoom() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_DOServerHost);
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = _refLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_LOBBY_INDEX);
		}

		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_ROOM_INDEX);
		}

		// 방장이 맞는지 확인
		if (pRoom->IsMaster(pUser->GetIndex()) == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_MASTER);
		}

		// 방의 인원이 2명인가?
		if (pRoom->GetUserCount() != 2) {
			CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_USER_COUNT);
		}

		// 방의 상태가 게임을 안하는 중인지?
		if (pRoom->GetGameObj()->GetState() != GameState::NONE) {
			CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_GAME_STATE);
		}

		// 방의 게임 상태 변경
		pRoom->GetGameObj()->SetState(GameState::STARTTING);

		//TODO: 로비의 유저에게 방의 상태 변경 통보

		// 방의 다른 유저에게 방장이 게임 시작 요청을 했음을 알리고
		pRoom->SendToAllUser((short)PACKET_ID::ROOM_MASTER_GAME_START_NTF, 
								0, 
								nullptr, 
								pUser->GetIndex());

		// 요청자에게 답변을 보낸다.
		_refNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_MASTER_GAME_START_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		_refNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_MASTER_GAME_START_RES, sizeof(resPkt), (char*)&resPkt);
		return __result;
	}

	ERROR_CODE PacketProcess::roomGameStart(PacketInfo packetInfo)
	{
	CHECK_START
		NCommon::PktRoomGameStartRes resPkt;

		auto pUserRet = _refUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDoServerHostInRoom() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_DOServerHost);
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = _refLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_LOBBY_INDEX);
		}

		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_ROOM_INDEX);
		}

		// 방의 상태가 게임을 안하는 중인지?
		if (pRoom->GetGameObj()->GetState() != GameState::STARTTING) {
			CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_GAME_STATE);
		}

		//TODO: 이미 게임 시작 요청을 했는가?

		//TODO: 방에서 게임 시작 요청한 유저 리스트에 등록

		// 방의 다른 유저에게 게임 시작 요청을 했음을 알리고

		// 요청자에게 답변을 보낸다.
		_refNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_GAME_START_RES, sizeof(resPkt), (char*)&resPkt);
		
		
		// 게임 시작 가능한가?
		// 시작이면 게임 상태 변경 GameState::ING
		// 게임 시작 패킷 보내기
		// 방의 상태 변경 로비에 알리고
		// 게임의 선택 시작 시간 설정
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		_refNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_GAME_START_RES, sizeof(resPkt), (char*)&resPkt);
		return __result;
	}
}