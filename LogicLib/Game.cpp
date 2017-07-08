#include <chrono>

#include "Game.h"


namespace LogicLib
{
	/*Game::Game() {}

	Game::~Game() {}*/

	void Game::Clear()
	{
		_state = GameState::NONE;
	}

	bool Game::CheckSelectTime()
	{
		auto curTime = std::chrono::system_clock::now();
		auto curSecTime = std::chrono::system_clock::to_time_t(curTime);

		auto diff = curSecTime - _selectTime;
		if (diff >= 60)
		{
			//
			return true;
		}

		return false;
	}
}