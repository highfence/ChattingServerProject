#pragma once

#include <time.h>
#include <chrono>
#include <vector>

#include <../ChattingServerProject/NetworkLib/NetworkLib/Logger.h>
#include <../ChattingServerProject/NetworkLib/NetworkLib/TcpNetwork.h>

namespace LogicLib
{
	struct ConnectedUser
	{
		void Clear()
		{
			_isLoginSuccess = false;
			_connectedTime = 0;
		}

		bool _isLoginSuccess = false;
		time_t _connectedTime = 0;
	};

	class ConnectedUserManager
	{
		using TcpNet = NetworkLib::ITcpNetwork;
		using Logger = NetworkLib::Logger;

	public:
		ConnectedUserManager() {}
		virtual ~ConnectedUserManager() {}

		void Init(const int maxSessionCount, TcpNet* pNetwork, NetworkLib::ServerConfig* pConfig, Logger* pLogger)
		{
			_refLogger = pLogger;
			_refNetwork = pNetwork;

			for (int i = 0; i < maxSessionCount; ++i)
			{
				_connectedUserList.emplace_back(ConnectedUser());
			}

			_isLoginCheck = pConfig->IsLoginCheck;
		}

		void SetConnectSession(const int sessionIndex)
		{
			time(&_connectedUserList[sessionIndex]._connectedTime);
		}

		void SetLogin(const int sessionIndex)
		{
			_connectedUserList[sessionIndex]._isLoginSuccess = true;
		}

		void SetDisConnectSession(const int sessionIndex)
		{
			_connectedUserList[sessionIndex].Clear();
		}

		void LoginCheck()
		{
			if (_isLoginCheck == false) {
				return;
			}

			auto curTime = std::chrono::system_clock::now();
			auto diffTime = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - _latestLoginCheckTime);

			// 60밀리초 마다 검사
			if (diffTime.count() < 60)
			{
				return;
			}
			else
			{
				_latestLoginCheckTime = curTime;
			}

			auto curSecTime = std::chrono::system_clock::to_time_t(curTime);

			const auto maxSessionCount = (int)_connectedUserList.size();

			if (_latestLogincheckIndex >= maxSessionCount) {
				_latestLogincheckIndex = -1;
			}

			++_latestLogincheckIndex;

			auto lastCheckIndex = _latestLogincheckIndex + 100;
			if (lastCheckIndex > maxSessionCount) {
				lastCheckIndex = maxSessionCount;
			}

			for (; _latestLogincheckIndex < lastCheckIndex; ++_latestLogincheckIndex)
			{
				auto i = _latestLogincheckIndex;

				if (_connectedUserList[i]._connectedTime == 0 ||
					_connectedUserList[i]._isLoginSuccess == false)
				{
					continue;
				}

				auto diff = curSecTime - _connectedUserList[i]._connectedTime;
				if (diff >= 180)
				{
					_refLogger->Write(NetworkLib::LOG_TYPE::L_WARN, "%s | Login Wait Time Over. sessionIndex(%d).", __FUNCTION__, i);
					_refNetwork->ForcingClose(i);
				}
			}
		}

	private:
		Logger* _refLogger;
		TcpNet* _refNetwork;

		std::vector<ConnectedUser> _connectedUserList;

		bool _isLoginCheck = false;

		std::chrono::system_clock::time_point _latestLoginCheckTime = std::chrono::system_clock::now();
		int _latestLogincheckIndex = -1;
	};
}