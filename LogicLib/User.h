#pragma once
#include <string>

namespace LogicLib
{
	class User
	{
	public :
		enum class DOServerHost_STATE {
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
			_curDoServerHostState = DOServerHost_STATE::NONE;
			_lobbyIndex = -1;
			_roomIndex = -1;
		}

		void Set(const int sessionIndex, const char* pszID)
		{
			_isConfirm = true;
			_curDoServerHostState = DOServerHost_STATE::LOGIN;

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
			_curDoServerHostState = DOServerHost_STATE::LOBBY;
		}
		
		void LeaveLobby()
		{
			_curDoServerHostState = DOServerHost_STATE::LOGIN;
		}

		
		short GetRoomIndex() { return _roomIndex; }

		void EnterRoom(const short lobbyIndex, const short roomIndex)
		{
			_lobbyIndex = lobbyIndex;
			_roomIndex = roomIndex;
			_curDoServerHostState = DOServerHost_STATE::ROOM;
		}

		bool IsCurDoServerHostInLogIn() {
			return _curDoServerHostState == DOServerHost_STATE::LOGIN ? true : false;
		}
		
		bool IsCurDoServerHostInLobby() {
			return _curDoServerHostState == DOServerHost_STATE::LOBBY ? true : false;
		}

		bool IsCurDoServerHostInRoom() {
			return _curDoServerHostState == DOServerHost_STATE::ROOM ? true : false;
		}

		
	protected:
		short _index = -1;
		
		int _sessionIndex = -1;

		std::string _id;
		
		bool _isConfirm = false;
		
		DOServerHost_STATE _curDoServerHostState = DOServerHost_STATE::NONE;

		short _lobbyIndex = -1;

		short _roomIndex = -1;
	};
}