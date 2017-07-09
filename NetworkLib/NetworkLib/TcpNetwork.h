#pragma once
#define FD_SETSIZE 5096

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <WS2tcpip.h>

#include <vector>
#include <deque>
#include <unordered_map>

#include "ITcpNetwork.h"

namespace NetworkLib
{
	class TcpNetwork : public ITcpNetwork
	{
	public:
		TcpNetwork();
		virtual ~TcpNetwork();

		NET_ERROR_CODE Init(const ServerConfig * pConfig, Logger * pLogger) override;

		NET_ERROR_CODE SendData(const int sessionIndex, const short packetId, const short size, const char * pMsg) override;

		void Run() override;

		RecvPacketInfo GetPacketInfo() override;

		void Release() override;

		int ClientSessionPoolSize() override { return (int)_clientSessionPool.size(); };

		void ForcingClose(const int sessionIndex);

	protected :
		// 家南 备炼眉 技泼.
		NET_ERROR_CODE InitServerSocket();
		// 辑滚 家南 Bind & Listen
		NET_ERROR_CODE BindListen(short port, int backlogCount);

		int AllocClientSessionIndex();
		void ReleaseSessionIndex(const int index);

		int CreateSessionPool(const int maxClientCount);
		NET_ERROR_CODE NewSession();
		void SetSockOption(const SOCKET fd);
		void ConnectedSession(const int sessionIndex, const SOCKET fd, const char* pIP);

		void CloseSession(const SOCKET_CLOSE_CASE closeCase, const SOCKET sockFD, const int sessionIndex);

		NET_ERROR_CODE RecvSocket(const int sessionIndex);
		NET_ERROR_CODE RecvBufferProcess(const int sessionIndex);
		void AddPacketQueue(const int sessionIndex, const short pktId, const short bodySize, char* pDataPos);

		void RunProcessWrite(const int sessionIndex, const SOCKET fd, fd_set& write_set);
		NetError FlushSendBuff(const int sessionIndex);
		NetError SendSocket(const SOCKET fd, const char* pMsg, const int size);

		bool RunCheckSelectResult(const int result);
		void RunCheckSelectClients(fd_set& read_set, fd_set& write_set);
		bool RunProcessReceive(const int sessionIndex, const SOCKET fd, fd_set& read_set);

	protected :

		ServerConfig _config;

		SOCKET _serverSockfd;

		fd_set _readfds;
		size_t _connectedSessionCount = 0;

		int64_t _connectSeq = 0;

		std::vector<ClientSession> _clientSessionPool;
		std::deque<int> _clientSessionPoolIndex;

		std::deque<RecvPacketInfo> _packetQueue;

		Logger * _refLogger;

	private :

	};
}