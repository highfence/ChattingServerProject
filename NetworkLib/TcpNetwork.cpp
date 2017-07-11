#include <stdio.h>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <functional>

#include "../Common/ObjectPool.h"
#include "ILog.h"
#include "TcpNetwork.h"


namespace NetworkLib
{
	TcpNetwork::TcpNetwork() {}
	
	TcpNetwork::~TcpNetwork() 
	{
	}

	NET_ERROR_CODE TcpNetwork::Init(const ServerConfig* pConfig, ILog* pLogger)
	{
#pragma region thread func

		auto startRunThread = [this]()
		{
			_isNetworkRunning = true;

			_runningThread = std::thread([this]() { Run(); });
		};

#pragma endregion

		memcpy(&_config, pConfig, sizeof(ServerConfig));

		_refLogger = pLogger;

		auto initRet = initServerSocket();
		if (initRet != NET_ERROR_CODE::NONE)
		{
			return initRet;
		}
		
		auto bindListenRet = bindListen(pConfig->Port, pConfig->BackLogCount);
		if (bindListenRet != NET_ERROR_CODE::NONE)
		{
			return bindListenRet;
		}

		FD_ZERO(&_readfds);
		FD_SET(_serverSockfd, &_readfds);
		
		auto sessionPoolSize = pConfig->MaxClientCount + pConfig->ExtraClientCount;
		_clientSessionPool.Init(sessionPoolSize);
			
		_refLogger->Write(LOG_TYPE::L_INFO, "%s | Session Pool Size: %d", __FUNCTION__, sessionPoolSize);

		startRunThread();

		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::Release()
	{
		_isNetworkRunning = false;
		_runningThread.join();
		_clientSessionPool.Release();
		WSACleanup();
	}

	RecvPacketInfo TcpNetwork::GetPacketInfo()
	{
		RecvPacketInfo packetInfo;

		std::lock_guard<std::mutex> _queueLock(_queueMutex);

		if (_packetQueue.empty() == false)
		{
			packetInfo = _packetQueue.front();
			_packetQueue.pop_front();
		}
				
		return packetInfo;
	}
		
	void TcpNetwork::ForcingClose(const int sessionIndex)
	{
		if (_clientSessionPool[sessionIndex].IsConnected() == false)
		{
			return;
		}

		closeSession(SOCKET_CLOSE_CASE::FORCING_CLOSE, static_cast<SOCKET>(_clientSessionPool[sessionIndex].SocketFD), sessionIndex);
	}

	void TcpNetwork::RegistEventHandle(HANDLE * eventHandle)
	{
		if (eventHandle == nullptr)
		{
			return;
		}

		_processEvent = eventHandle;
	}

	bool TcpNetwork::IsPacketTaskRemaining()
	{
		std::lock_guard<std::mutex> lock(_queueMutex);
		if (_packetQueue.empty())
		{
			return false;
		}

		return true;
	}

	void TcpNetwork::Run()
	{
		while (_isNetworkRunning)
		{
			auto read_set = _readfds;
			auto write_set = _readfds;

			timeval timeout{ 0, 1000 }; //tv_sec, tv_usec
			auto selectResult = select(0, &read_set, &write_set, 0, &timeout);

			auto isFDSetChanged = runCheckSelectResult(selectResult);
			if (isFDSetChanged == false)
			{
				continue;
			}

			// Accept
			if (FD_ISSET(_serverSockfd, &read_set))
			{
				newSession();
			}

			runCheckSelectClients(read_set, write_set);

			if (_processEvent != nullptr)
			{
				SetEvent(_processEvent);
			}
		}
	}

	bool TcpNetwork::runCheckSelectResult(const int result)
	{
		if (result == 0)
		{
			return false;
		}
		else if (result == -1)
		{
			// TODO:로그 남기기
			return false;
		}

		return true;
	}
	
	void TcpNetwork::runCheckSelectClients(fd_set& read_set, fd_set& write_set)
	{
		for (unsigned int i = 0; i < _clientSessionPool.GetSize(); ++i)
		{
			auto& session = _clientSessionPool[i];

			if (session.IsConnected() == false) {
				continue;
			}

			SOCKET fd = static_cast<SOCKET>(session.SocketFD);
			auto sessionIndex = session.Index;

			// check read
			auto retReceive = runProcessReceive(sessionIndex, fd, read_set);
			if (retReceive == false) {
				continue;
			}

			// check write
			runProcessWrite(sessionIndex, fd, write_set);
		}
	}

	bool TcpNetwork::runProcessReceive(const int sessionIndex, const SOCKET fd, fd_set& read_set)
	{
		if (!FD_ISSET(fd, &read_set))
		{
			return true;
		}

		auto ret = recvSocket(sessionIndex);
		if (ret != NET_ERROR_CODE::NONE)
		{
			closeSession(SOCKET_CLOSE_CASE::SOCKET_RECV_ERROR, fd, sessionIndex);
			return false;
		}

		ret = recvBufferProcess(sessionIndex);
		if (ret != NET_ERROR_CODE::NONE)
		{
			closeSession(SOCKET_CLOSE_CASE::SOCKET_RECV_BUFFER_PROCESS_ERROR, fd, sessionIndex);
			return false;
		}

		return true;
	}

	NET_ERROR_CODE TcpNetwork::SendData(const int sessionIndex, const short packetId, const short size, const char* pMsg)
	{
		auto& session = _clientSessionPool[sessionIndex];

		auto pos = session.SendSize;

		if ((pos + size + PACKET_HEADER_SIZE) > _config.MaxClientSendBufferSize ) {
			return NET_ERROR_CODE::CLIENT_SEND_BUFFER_FULL;
		}
				
		PacketHeader pktHeader{ packetId, size };
		memcpy(&session.pSendBuffer[pos], (char*)&pktHeader, PACKET_HEADER_SIZE);
		memcpy(&session.pSendBuffer[pos + PACKET_HEADER_SIZE], pMsg, size);
		session.SendSize += (size + PACKET_HEADER_SIZE);

		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::initServerSocket()
	{
		WORD wVersionRequested = MAKEWORD(2, 2);
		WSADATA wsaData;
		WSAStartup(wVersionRequested, &wsaData);

		_serverSockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_serverSockfd < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;
		}

		auto n = 1;
		if (setsockopt(_serverSockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n)) < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_SO_REUSEADDR_FAIL;
		}

		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::bindListen(short port, int backlogCount)
	{
		SOCKADDR_IN server_addr;
		ZeroMemory(&server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		server_addr.sin_port = htons(port);

		if (bind(_serverSockfd, (SOCKADDR*)&server_addr, sizeof(server_addr)) < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_BIND_FAIL;
		}
		
		unsigned long mode = 1;
		if (ioctlsocket(_serverSockfd, FIONBIO, &mode) == SOCKET_ERROR)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_FIONBIO_FAIL;
		}

		if (listen(_serverSockfd, backlogCount) == SOCKET_ERROR)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_LISTEN_FAIL;
		}

		_refLogger->Write(LOG_TYPE::L_INFO, "%s | Listen. ServerSockfd(%I64u)", __FUNCTION__, _serverSockfd);
		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::newSession()
	{
		auto tryCount = 0; // 너무 많이 accept를 시도하지 않도록 한다.

		do
		{
			++tryCount;

			SOCKADDR_IN client_addr;
			auto client_len = static_cast<int>(sizeof(client_addr));
			auto client_sockfd = accept(_serverSockfd, (SOCKADDR*)&client_addr, &client_len);
				//m_pRefLogger->Write(LOG_TYPE::L_DEBUG, "%s | client_sockfd(%I64u)", __FUNCTION__, client_sockfd);
			if (client_sockfd == INVALID_SOCKET)
			{
				if (WSAGetLastError() == WSAEWOULDBLOCK)
				{
					return NET_ERROR_CODE::ACCEPT_API_WSAEWOULDBLOCK;
				}

				_refLogger->Write(LOG_TYPE::L_ERROR, "%s | Wrong socket cannot accept", __FUNCTION__);
				return NET_ERROR_CODE::ACCEPT_API_ERROR;
			}

			auto newSessionIndex = _clientSessionPool.GetTag();
			if (newSessionIndex < 0)
			{
				_refLogger->Write(LOG_TYPE::L_WARN, "%s | client_sockfd(%I64u)  >= MAX_SESSION", __FUNCTION__, client_sockfd);

				// 더 이상 수용할 수 없으므로 바로 짜른다.
				closeSession(SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY, client_sockfd, -1);
				return NET_ERROR_CODE::ACCEPT_MAX_SESSION_COUNT;
			}


			char clientIP[MAX_IP_LEN] = { 0, };
			inet_ntop(AF_INET, &(client_addr.sin_addr), clientIP, MAX_IP_LEN - 1);

			setSockOption(client_sockfd);

			FD_SET(client_sockfd, &_readfds);
				_refLogger->Write(LOG_TYPE::L_DEBUG, "%s | client_sockfd(%I64u)", __FUNCTION__, client_sockfd);
			connectedSession(newSessionIndex, client_sockfd, clientIP);

		} while (tryCount < FD_SETSIZE);
		
		return NET_ERROR_CODE::NONE;
	}
	
	void TcpNetwork::connectedSession(const int sessionIndex, const SOCKET fd, const char* pIP)
	{
		++_connectSeq;

		auto& session = _clientSessionPool[sessionIndex];
		session.Seq = _connectSeq;
		session.SocketFD = fd;
		memcpy(session.IP, pIP, MAX_IP_LEN - 1);

		++_connectedSessionCount;

		addPacketQueue(sessionIndex, (short)PACKET_ID::NTF_SYS_CONNECT_SESSION, 0, nullptr);

		_refLogger->Write(LOG_TYPE::L_INFO, "%s | New Session. FD(%I64u), m_ConnectSeq(%d), IP(%s)", __FUNCTION__, fd, _connectSeq, pIP);
	}

	void TcpNetwork::setSockOption(const SOCKET fd)
	{
		linger ling;
		ling.l_onoff = 0;
		ling.l_linger = 0;
		setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

		int size1 = _config.MaxClientSockOptRecvBufferSize;
		int size2 = _config.MaxClientSockOptSendBufferSize;
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&size1, sizeof(size1));
		setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&size2, sizeof(size2));
	}

	void TcpNetwork::closeSession(const SOCKET_CLOSE_CASE closeCase, const SOCKET sockFD, const int sessionIndex)
	{
		if (closeCase == SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY)
		{
			closesocket(sockFD);
			FD_CLR(sockFD, &_readfds);
			return;
		}

		if (_clientSessionPool[sessionIndex].IsConnected() == false) {
			return;
		}

		closesocket(sockFD);
		FD_CLR(sockFD, &_readfds);

		_clientSessionPool[sessionIndex].Clear();
		--_connectedSessionCount;
		_clientSessionPool.ReleaseTag(sessionIndex);

		addPacketQueue(sessionIndex, (short)PACKET_ID::NTF_SYS_CLOSE_SESSION, 0, nullptr);
	}

	NET_ERROR_CODE TcpNetwork::recvSocket(const int sessionIndex)
	{
		auto& session = _clientSessionPool[sessionIndex];
		auto fd = static_cast<SOCKET>(session.SocketFD);

		if (session.IsConnected() == false)
		{
			return NET_ERROR_CODE::RECV_PROCESS_NOT_CONNECTED;
		}

		int recvPos = 0;
				
		if (session.ReServerHostingDataSize > 0)
		{
			memcpy(session.pRecvBuffer, &session.pRecvBuffer[session.PrevReadPosInRecvBuffer], session.ReServerHostingDataSize);
			recvPos += session.ReServerHostingDataSize;
		}

		auto recvSize = recv(fd, &session.pRecvBuffer[recvPos], (MAX_PACKET_BODY_SIZE * 2), 0);

		if (recvSize == 0)
		{
			return NET_ERROR_CODE::RECV_REMOTE_CLOSE;
		}

		if (recvSize < 0)
		{
			auto error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK)
			{
				return NET_ERROR_CODE::RECV_API_ERROR; 
			}
			else 
			{
				return NET_ERROR_CODE::NONE;
			}
		}

		session.ReServerHostingDataSize += recvSize;
		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::recvBufferProcess(const int sessionIndex)
	{
		auto& session = _clientSessionPool[sessionIndex];
		
		auto readPos = 0;
		const auto dataSize = session.ReServerHostingDataSize;
		PacketHeader* pPktHeader;
		
		while ((dataSize - readPos) >= PACKET_HEADER_SIZE)
		{
			pPktHeader = (PacketHeader*)&session.pRecvBuffer[readPos];
			readPos += PACKET_HEADER_SIZE;

			if (pPktHeader->BodySize > 0)
			{
				if (pPktHeader->BodySize > (dataSize - readPos))
				{
					readPos -= PACKET_HEADER_SIZE;
					break;
				}

				if (pPktHeader->BodySize > MAX_PACKET_BODY_SIZE)
				{
					// 더 이상 이 세션과는 작업을 하지 않을 예정. 클라이언트 보고 나가라고 하던가 직접 짤라야 한다.
					return NET_ERROR_CODE::RECV_CLIENT_MAX_PACKET;
				}
			}

			addPacketQueue(sessionIndex, pPktHeader->Id, pPktHeader->BodySize, &session.pRecvBuffer[readPos]);
			readPos += pPktHeader->BodySize;
		}
		
		session.ReServerHostingDataSize -= readPos;
		session.PrevReadPosInRecvBuffer = readPos;
		
		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::addPacketQueue(const int sessionIndex, const short pktId, const short bodySize, char* pDataPos)
	{
		RecvPacketInfo packetInfo;
		packetInfo.SessionIndex = sessionIndex;
		packetInfo.PacketId = pktId;
		packetInfo.PacketBodySize = bodySize;
		packetInfo.pRefData = pDataPos;

		std::lock_guard<std::mutex> _queueLock(_queueMutex);
		_packetQueue.push_back(packetInfo);
	}

	void TcpNetwork::runProcessWrite(const int sessionIndex, const SOCKET fd, fd_set& write_set)
	{
		if (!FD_ISSET(fd, &write_set))
		{
			return;
		}

		auto retsend = flushSendBuff(sessionIndex);
		if (retsend.Error != NET_ERROR_CODE::NONE)
		{
			closeSession(SOCKET_CLOSE_CASE::SOCKET_SEND_ERROR, fd, sessionIndex);
		}
	}

	NetError TcpNetwork::flushSendBuff(const int sessionIndex)
	{
		auto& session = _clientSessionPool[sessionIndex];
		auto fd = static_cast<SOCKET>(session.SocketFD);

		if (session.IsConnected() == false)
		{
			return NetError(NET_ERROR_CODE::CLIENT_FLUSH_SEND_BUFF_REMOTE_CLOSE);
		}

		auto result = sendSocket(fd, session.pSendBuffer, session.SendSize);

		if (result.Error != NET_ERROR_CODE::NONE) {
			return result;
		}

		auto sendSize = result.Value;
		if (sendSize < session.SendSize)
		{
			memmove(&session.pSendBuffer[0],
				&session.pSendBuffer[sendSize],
				session.SendSize - sendSize);

			session.SendSize -= sendSize;
		}
		else
		{
			session.SendSize = 0;
		}
		return result;
	}

	NetError TcpNetwork::sendSocket(const SOCKET fd, const char* pMsg, const int size)
	{
		NetError result(NET_ERROR_CODE::NONE);
		auto rfds = _readfds;

		// 접속 되어 있는지 또는 보낼 데이터가 있는지
		if (size <= 0)
		{
			return result;
		}

		result.Value = send(fd, pMsg, size, 0);

		if (result.Value <= 0)
		{
			result.Error = NET_ERROR_CODE::SEND_SIZE_ZERO;
		}

		return result;
	}

	
}