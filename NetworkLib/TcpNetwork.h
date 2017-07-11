#pragma once

#define FD_SETSIZE 5096 

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>

#include <vector>
#include <deque>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <functional>

#include "ITcpNetwork.h"


namespace NetworkLib
{
	class TcpNetwork : public ITcpNetwork
	{
	public:
		TcpNetwork();
		virtual ~TcpNetwork();

		NET_ERROR_CODE Init(const ServerConfig* pConfig, ILog* pLogger) override;
		
		NET_ERROR_CODE SendData(const int sessionIndex, const short packetId, const short size, const char* pMsg) override;
		
		void Run() override;
		
		RecvPacketInfo GetPacketInfo() override;

		void Release() override;

		int ClientSessionPoolSize() override { return (int)_clientSessionPool.size(); }

		void ForcingClose(const int sessionIndex);
		
		void RegistEventHandle(HANDLE* eventHandle);

		bool IsPacketTaskRemaining();

	protected:

		NET_ERROR_CODE initServerSocket();
		NET_ERROR_CODE bindListen(short port, int backlogCount);
		
		int allocClientSessionIndex();
		void releaseSessionIndex(const int index);

		int createSessionPool(const int maxClientCount);
		NET_ERROR_CODE newSession();
		void setSockOption(const SOCKET fd);
		void connectedSession(const int sessionIndex, const SOCKET fd, const char* pIP);
		
		void closeSession(const SOCKET_CLOSE_CASE closeCase, const SOCKET sockFD, const int sessionIndex);
		
		NET_ERROR_CODE recvSocket(const int sessionIndex);
		NET_ERROR_CODE recvBufferProcess(const int sessionIndex);
		void addPacketQueue(const int sessionIndex, const short pktId, const short bodySize, char* pDataPos);
		
		void runProcessWrite(const int sessionIndex, const SOCKET fd, fd_set& write_set);
		NetError flushSendBuff(const int sessionIndex);
		NetError sendSocket(const SOCKET fd, const char* pMsg, const int size);

		bool runCheckSelectResult(const int result);
		void runCheckSelectClients(fd_set& read_set, fd_set& write_set);
		bool runProcessReceive(const int sessionIndex, const SOCKET fd, fd_set& read_set);
		
	protected:

		ServerConfig               _config;
				
		SOCKET                     _serverSockfd;

		fd_set					   _readfds;
		size_t					   _connectedSessionCount = 0;
		
		int64_t						_connectSeq = 0;
		
		std::vector<ClientSession> _clientSessionPool;
		std::deque<int>            
			_clientSessionPoolIndex;
		
		bool	                   _isNetworkRunning = false;
		std::thread                _runningThread;
		std::mutex                 _queueMutex;
		std::deque<RecvPacketInfo> _packetQueue;
		HANDLE*                    _processEvent = nullptr;

		ILog*                      _refLogger;
	};
}
