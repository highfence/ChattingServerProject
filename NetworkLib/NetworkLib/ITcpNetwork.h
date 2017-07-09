#pragma once

#include "Define.h"
#include "ServerNetErrorCode.h"
#include "Logger.h"

namespace NetworkLib
{
	class ITcpNetwork
	{
	public :
		ITcpNetwork() {}
		virtual ~ITcpNetwork() {}

		virtual NET_ERROR_CODE Init(
			const ServerConfig * pConfig,
			Logger * pLogger)
		{
			return NET_ERROR_CODE::NONE;
		}

		virtual NET_ERROR_CODE SendData(
			const int sessionIndex,
			const short packetId,
			const short size,
			const char * pMsg)
		{
			return NET_ERROR_CODE::NONE;
		}

		virtual void Run() {}

		virtual RecvPacketInfo GetPacketInfo() { return RecvPacketInfo(); }

		virtual void Release() {}

		virtual int ClientSessionPoolSize() { return 0; }

		virtual void ForcingClose(const int sessionIndex) {}
	};

}