#pragma once



namespace LogicLib
{
	enum class GameState 
	{
		NONE,
		STARTTING,
		ING,
		END
	};

	class Game
	{
	public:
		Game() {}
		virtual ~Game() {}

		void Clear();

		GameState GetState() { return _state;  }
		
		void SetState(const GameState state) { _state = state; }

		bool CheckSelectTime();

	private:
		
		GameState _state = GameState::NONE;

		__int64 _selectTime;
		int _gameSelect1; // ����(0), ����(1), ��(2)
		int _gameSelect2;
	};

}