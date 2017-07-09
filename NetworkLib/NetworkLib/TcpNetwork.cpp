#include <stdio.h>
#include "../../Common/PacketID.h"
#include "../../Common/Packet.h"
#include "Define.h"
#include "TcpNetwork.h"


namespace NetworkLib
{
	TcpNetwork::TcpNetwork()
	{
	}

	TcpNetwork::~TcpNetwork()
	{
		for (auto& client : _clientSessionPool)
		{
			if (client.pRecvBuffer)
			{
				delete[] client.pRecvBuffer;
			}

			if (client.pSendBuffer)
			{
				delete[] client.pRecvBuffer;
			}
		}
	}

	NET_ERROR_CODE TcpNetwork::Init(const ServerConfig * pConfig, Logger * pLogger)
	{
		memcpy(&_config, pConfig, sizeof(ServerConfig));

		_refLogger = pLogger;

		auto initRet = InitServerSocket();
		if (initRet != NET_ERROR_CODE::NONE)
		{
			return initRet;
		}

		auto bindListenRet = BindListen(pConfig->Port, pConfig->BackLogCount);
		if (bindListenRet != NET_ERROR_CODE::NONE)
		{
			return bindListenRet;
		}

		FD_ZERO(&_readfds);
		FD_SET(_serverSockfd, &_readfds);

		auto sessionPoolSize =
			CreateSessionPool(
				pConfig->MaxClientCount + pConfig->ExtraClientCount);

		_refLogger->Write(LOG_TYPE::L_INFO, "%s | Session Pool Size: %d", __FUNCTION__, sessionPoolSize);

		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::SendData(const int sessionIndex, const short packetId, const short size, const char * pMsg)
	{
		return NET_ERROR_CODE();
	}

	void TcpNetwork::Run()
	{
		auto read_set = _readfds;
		auto write_set = _readfds;

		timeval timeout{ 0, 1000 };
		auto selectResult = select(0, &read_set, &write_set, 0, &timeout);

		auto isFDSetChanged = RunCheckSelectResult(selectResult);
		if (isFDSetChanged == false)
		{
			return;
		}

		// Accept
		if (FD_ISSET(_serverSockfd, &read_set))
		{
			NewSession();
		}

		RunCheckSelectClients(read_set, write_set);
	}

	RecvPacketInfo TcpNetwork::GetPacketInfo()
	{
		RecvPacketInfo packetInfo;

		if (_packetQueue.empty() == false)
		{
			packetInfo = _packetQueue.front();
			_packetQueue.pop_front();
		}

		return packetInfo;
	}

	void TcpNetwork::Release()
	{
		WSACleanup();
	}

	void TcpNetwork::ForcingClose(const int sessionIndex)
	{
		if (_clientSessionPool[sessionIndex].IsConnected() == false)
		{
			return;
		}

		CloseSession(SOCKET_CLOSE_CASE::FORCING_CLOSE, _clientSessionPool[sessionIndex].SocketFD, sessionIndex);
	}

	NET_ERROR_CODE TcpNetwork::InitServerSocket()
	{
		WORD wVersionRequested = MAKEWORD(2, 2);
		WSADATA wsaData;
		WSAStartup(wVersionRequested, &wsaData);

		_serverSockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_serverSockfd < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;
		}

		// 소켓 REUSEADDR 설정.
		auto n = 1;
		if (setsockopt(_serverSockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n)) < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_SO_REUSEADDR_FAIL;
		}

		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::BindListen(short port, int backlogCount)
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

		// 넌 블로킹 소켓으로 설정.
		unsigned long mode = 1;
		if (ioctlsocket(_serverSockfd, FIONBIO, &mode) == SOCKET_ERROR)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_FIONBIO_FAIL;
		}

		// backlogCount ==> 소켓 큐 개수?
		if (listen(_serverSockfd, backlogCount) == SOCKET_ERROR)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_LISTEN_FAIL;
		}

		_refLogger->Write(LOG_TYPE::L_INFO, "%s | Listen. ServerSockfd(%I64u)", __FUNCTION__, _serverSockfd);
		return NET_ERROR_CODE::NONE;
	}

	int TcpNetwork::AllocClientSessionIndex()
	{
		if (_clientSessionPoolIndex.empty()) {
			return -1;
		}

		int index = _clientSessionPoolIndex.front();
		_clientSessionPoolIndex.pop_front();
		return index;
	}

	void TcpNetwork::ReleaseSessionIndex(const int index)
	{
		_clientSessionPoolIndex.push_back(index);
		_clientSessionPool[index].Clear();
	}

	int TcpNetwork::CreateSessionPool(const int maxClientCount)
	{
		for (int i = 0; i < maxClientCount; ++i)
		{
			ClientSession session;
			ZeroMemory(&session, sizeof(session));
			session.Index = i;
			session.pRecvBuffer = new char[_config.MaxClientRecvBufferSize];
			session.pSendBuffer = new char[_config.MaxClientSendBufferSize];

			_clientSessionPool.push_back(session);
			_clientSessionPoolIndex.push_back(session.Index);
		}

		return maxClientCount;
	}

	NET_ERROR_CODE TcpNetwork::NewSession()
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

			auto newSessionIndex = AllocClientSessionIndex();
			if (newSessionIndex < 0)
			{
				_refLogger->Write(LOG_TYPE::L_WARN, "%s | client_sockfd(%I64u)  >= MAX_SESSION", __FUNCTION__, client_sockfd);

				// 더 이상 수용할 수 없으므로 바로 짜른다.
				CloseSession(SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY, client_sockfd, -1);
				return NET_ERROR_CODE::ACCEPT_MAX_SESSION_COUNT;
			}


			char clientIP[maxIpLen] = { 0, };
			inet_ntop(AF_INET, &(client_addr.sin_addr), clientIP, maxIpLen - 1);

			SetSockOption(client_sockfd);

			FD_SET(client_sockfd, &_readfds);
			_refLogger->Write(LOG_TYPE::L_DEBUG, "%s | client_sockfd(%I64u)", __FUNCTION__, client_sockfd);
			ConnectedSession(newSessionIndex, client_sockfd, clientIP);

		} while (tryCount < FD_SETSIZE);

		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::SetSockOption(const SOCKET fd)
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

	void TcpNetwork::ConnectedSession(const int sessionIndex, const SOCKET fd, const char * pIP)
	{
		++_connectSeq;

		auto& session = _clientSessionPool[sessionIndex];
		session.Seq = _connectSeq;
		session.SocketFD = fd;
		memcpy(session.IP, pIP, maxIpLen - 1);

		++_connectedSessionCount;

		AddPacketQueue(sessionIndex, (short)PACKET_ID::NTF_SYS_CONNECT_SESSION, 0, nullptr);

		_refLogger->Write(LOG_TYPE::L_INFO, "%s | New Session. FD(%I64u), m_ConnectSeq(%d), IP(%s)", __FUNCTION__, fd, _connectSeq, pIP);
	}

	void TcpNetwork::CloseSession(const SOCKET_CLOSE_CASE closeCase, const SOCKET sockFD, const int sessionIndex)
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
		ReleaseSessionIndex(sessionIndex);

		AddPacketQueue(sessionIndex, (short)PACKET_ID::NTF_SYS_CLOSE_SESSION, 0, nullptr);
	}

	NET_ERROR_CODE TcpNetwork::RecvSocket(const int sessionIndex)
	{
		auto& session = _clientSessionPool[sessionIndex];
		auto fd = static_cast<SOCKET>(session.SocketFD);

		if (session.IsConnected() == false)
		{
			return NET_ERROR_CODE::RECV_PROCESS_NOT_CONNECTED;
		}

		int recvPos = 0;

		if (session.RemainingDataSize > 0)
		{
			memcpy(session.pRecvBuffer, &session.pRecvBuffer[session.PrevReadPosInRecvBuffer], session.RemainingDataSize);
			recvPos += session.RemainingDataSize;
		}

		auto recvSize = recv(fd, &session.pRecvBuffer[recvPos], (maxPacketBodySize * 2), 0);

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

		session.RemainingDataSize += recvSize;
		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::RecvBufferProcess(const int sessionIndex)
	{
		auto& session = _clientSessionPool[sessionIndex];

		auto readPos = 0;
		const auto dataSize = session.RemainingDataSize;
		PacketHeader* pPktHeader;

		while ((dataSize - readPos) >= PACKET_HEADER_SIZE)
		{
			pPktHeader = reinterpret_cast<PacketHeader*>(&session.pRecvBuffer[readPos]);
			readPos += PACKET_HEADER_SIZE;

			if (pPktHeader->BodySize > 0)
			{
				if (pPktHeader->BodySize > (dataSize - readPos))
				{
					readPos -= PACKET_HEADER_SIZE;
					break;
				}

				if (pPktHeader->BodySize > maxPacketBodySize)
				{
					return NET_ERROR_CODE::RECV_CLIENT_MAX_PACKET;
				}
			}

			AddPacketQueue(sessionIndex, pPktHeader->Id, pPktHeader->BodySize, &session.pRecvBuffer[readPos]);
			readPos += pPktHeader->BodySize;
		}

		session.RemainingDataSize -= readPos;
		session.PrevReadPosInRecvBuffer = readPos;

		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::AddPacketQueue(const int sessionIndex, const short pktId, const short bodySize, char * pDataPos)
	{
		RecvPacketInfo packetInfo;
		packetInfo.SessionIndex = sessionIndex;
		packetInfo.PacketId = pktId;
		packetInfo.PacketBodySize = bodySize;
		packetInfo.pRefData = pDataPos;

		_packetQueue.push_back(packetInfo);
	}

	void TcpNetwork::RunProcessWrite(const int sessionIndex, const SOCKET fd, fd_set & write_set)
	{
		if (!FD_ISSET(fd, &write_set))
		{
			return;
		}

		auto retsend = FlushSendBuff(sessionIndex);
		if (retsend.Error != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_SEND_ERROR, fd, sessionIndex);
		}
	}

	NetError TcpNetwork::FlushSendBuff(const int sessionIndex)
	{
		auto& session = _clientSessionPool[sessionIndex];
		auto fd = static_cast<SOCKET>(session.SocketFD);

		if (session.IsConnected() == false)
		{
			return NetError(NET_ERROR_CODE::CLIENT_FLUSH_SEND_BUFF_REMOTE_CLOSE);
		}

		auto result = SendSocket(fd, session.pSendBuffer, session.SendSize);

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

	NetError TcpNetwork::SendSocket(const SOCKET fd, const char * pMsg, const int size)
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

	bool TcpNetwork::RunCheckSelectResult(const int result)
	{
		if (result == 0 || result == -1)
		{
			return false;
		}

		return true;
	}

	void TcpNetwork::RunCheckSelectClients(fd_set & read_set, fd_set & write_set)
	{
		for (int i = 0; i < _clientSessionPool.size(); ++i)
		{
			auto& session = _clientSessionPool[i];

			if (session.IsConnected() == false)
			{
				continue;
			}

			SOCKET fd = session.SocketFD;
			auto sessionIndex = session.Index;

			// check read
			auto retReceive = RunProcessReceive(sessionIndex, fd, read_set);
			if (retReceive == false)
			{
				continue;
			}

			// check write
			RunProcessWrite(sessionIndex, fd, write_set);
		}
	}
	
	bool TcpNetwork::RunProcessReceive(const int sessionIndex, const SOCKET fd, fd_set & read_set)
	{
		if (!FD_ISSET(fd, &read_set))
		{
			return true;
		}

		auto ret = RecvSocket(sessionIndex);
		if (ret != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_ERROR, fd, sessionIndex);
			return false;
		}

		ret = RecvBufferProcess(sessionIndex);
		if (ret != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_BUFFER_PROCESS_ERROR, fd, sessionIndex);
			return false;
		}

		return true;
	}
}