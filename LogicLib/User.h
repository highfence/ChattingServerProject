#pragma once
#include <string>

namespace LogicLib
{
	class User
	{
	public :
		enum class DOMAIN_STATE {
			NONE = 0,
			LOGIN = 1,
			LOBBY = 2,
			ROOM = 3,
		};

	public:
		User() {}
		virtual ~User() {}

		void Init(const short index)
		{
			_index = index;
		}

		void Clear()
		{			
			_sessionIndex = 0;
			_id = "";
			_isConfirm = false;
			_curDomainState = DOMAIN_STATE::NONE;
			_lobbyIndex = -1;
			_roomIndex = -1;
		}

		void Set(const int sessionIndex, const char* pszID)
		{
			_isConfirm = true;
			_curDomainState = DOMAIN_STATE::LOGIN;

			_sessionIndex = sessionIndex;
			_id = pszID;

		}

		short GetIndex() { return _index; }

		int GetSessioIndex() { return _sessionIndex;  }

		std::string& GetID() { return _id;  }

		bool IsConfirm() { return _isConfirm;  }

		short GetLobbyIndex() { return _lobbyIndex; }

		void EnterLobby(const short lobbyIndex)
		{
			_lobbyIndex = lobbyIndex;
			_curDomainState = DOMAIN_STATE::LOBBY;
		}
		
		void LeaveLobby()
		{
			_curDomainState = DOMAIN_STATE::LOGIN;
		}

		
		short GetRoomIndex() { return _roomIndex; }

		void EnterRoom(const short lobbyIndex, const short roomIndex)
		{
			_lobbyIndex = lobbyIndex;
			_roomIndex = roomIndex;
			_curDomainState = DOMAIN_STATE::ROOM;
		}

		bool IsCurDomainInLogIn() {
			return _curDomainState == DOMAIN_STATE::LOGIN ? true : false;
		}
		
		bool IsCurDomainInLobby() {
			return _curDomainState == DOMAIN_STATE::LOBBY ? true : false;
		}

		bool IsCurDomainInRoom() {
			return _curDomainState == DOMAIN_STATE::ROOM ? true : false;
		}

		
	protected:
		short _index = -1;
		
		int _sessionIndex = -1;

		std::string _id;
		
		bool _isConfirm = false;
		
		DOMAIN_STATE _curDomainState = DOMAIN_STATE::NONE;

		short _lobbyIndex = -1;

		short _roomIndex = -1;
	};
}