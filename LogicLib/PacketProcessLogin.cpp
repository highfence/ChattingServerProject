#include "../Common/Packet.h"
#include "../NetworkLib/TcpNetwork.h"
#include "../Common/ErrorCode.h"
#include "ConnectedUserManager.h"
#include "User.h"
#include "UserManager.h"
#include "LobbyManager.h"
#include "PacketProcess.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace LogicLib
{
	ERROR_CODE PacketProcess::login(PacketInfo packetInfo)
	{
	CHECK_START
		//TODO: 받은 데이터가 PktLogInReq 크기만큼인지 조사해야 한다.
		// 패스워드는 무조건 pass 해준다.
		// ID 중복이라면 에러 처리한다.

		NCommon::PktLogInRes resPkt;
		auto reqPkt = (NCommon::PktLogInReq*)packetInfo.pRefData;

		auto addRet = _refUserMgr->AddUser(packetInfo.SessionIndex, reqPkt->szID);

		if (addRet != ERROR_CODE::NONE) {
			CHECK_ERROR(addRet);
		}

		_connectedUserManager->SetLogin(packetInfo.SessionIndex);

		resPkt.ErrorCode = (short)addRet;
		_refNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOGIN_IN_RES, sizeof(NCommon::PktLogInRes), (char*)&resPkt);

		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		_refNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOGIN_IN_RES, sizeof(NCommon::PktLogInRes), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::lobbyList(PacketInfo packetInfo)
	{
	CHECK_START
		// 인증 받은 유저인가?
		// 아직 로비에 들어가지 않은 유저인가?
		
		auto pUserRet = _refUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}
	
		auto pUser = std::get<1>(pUserRet);
		
		if (pUser->IsCurDoServerHostInLogIn() == false) {
			CHECK_ERROR(ERROR_CODE::LOBBY_LIST_INVALID_DOServerHost);
		}
		
		_refLobbyMgr->SendLobbyListInfo(packetInfo.SessionIndex);
		
		return ERROR_CODE::NONE;

	CHECK_ERR:
		NCommon::PktLobbyListRes resPkt;
		resPkt.SetError(__result);
		_refNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOBBY_LIST_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}
}