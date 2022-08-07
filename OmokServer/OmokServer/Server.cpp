#include <thread>
#include <chrono>

#include "NetLib/ServerNetErrorCode.h"
#include "NetLib/Define.h"
#include "NetLib/TcpNetwork.h"
#include "ConsoleLogger.h"
#include "RoomManager.h"
#include "PacketProcess.h"
#include "UserManager.h"
#include "Server.h"

using NET_ERROR_CODE = NServerNetLib::NET_ERROR_CODE;
using LOG_TYPE = NServerNetLib::LOG_TYPE;

Server::Server()
{
}

Server::~Server()
{
	Release();
}

ERROR_CODE Server::Init()
{
	// Server에 필요한 객체를 생성하고 모두 Init
	m_pLogger = std::make_unique<ConsoleLog>();

	LoadConfig(); // NetLib를 사용하기 위한 설정값

	m_pNetwork = std::make_unique<NServerNetLib::TcpNetwork>();
	auto result = m_pNetwork->Init(m_pServerConfig.get(), m_pLogger.get()); // TcpNetwork의 Init() 함수 호출

	if (result != NET_ERROR_CODE::NONE)
	{
		m_pLogger->Write(LOG_TYPE::L_ERROR, "%s | Init Fail. NetErrorCode(%s)", __FUNCTION__, (short)result);
		return ERROR_CODE::MAIN_INIT_NETWORK_INIT_FAIL;
	}


	m_pUserMgr = std::make_unique<UserManager>();
	m_pUserMgr->Init(m_pServerConfig->MaxClientCount);

	m_pRoomMgr = std::make_unique<RoomManager>();
	m_pRoomMgr->Init(m_pServerConfig->MaxRoomCount, m_pServerConfig->MaxRoomUserCount);
	m_pRoomMgr->SetNetwork(m_pNetwork.get(), m_pLogger.get());

	m_pPacketProc = std::make_unique<PacketProcess>();
	m_pPacketProc->Init(m_pNetwork.get(), m_pUserMgr.get(), m_pRoomMgr.get(), m_pServerConfig.get(), m_pLogger.get());

	m_IsRun = true;

	m_pLogger->Write(LOG_TYPE::L_INFO, "%s | Init Success. Server Run", __FUNCTION__);
	return ERROR_CODE::NONE;
}

void Server::Release()
{
	if (m_pNetwork) {
		m_pNetwork->Release();
	}
}

void Server::Stop()
{
	m_IsRun = false;
}

void Server::Run()
{
	while (m_IsRun) // 서버가 종료되기 전까지 계속 반복
	{
		m_pNetwork->Run(); // TcpNetwork의 Run() 함수 호출

		while (true)
		{				
			// 도착한 패킷이 있는지 받아오기 (반환값 형태 RecvPacketInfo)
			/*	struct RecvPacketInfo
				{
			 		int SessionIndex = 0;
					short PacketId = 0;
					short PacketBodySize = 0;
					char* pRefData = 0;
				}*/
			auto packetInfo = m_pNetwork->GetPacketInfo();

			if (packetInfo.PacketId == 0) // 도착한 패킷이 없는 경우
			{
				break;
			}
			else // 도착한 패킷이 있는 경우 패킷을 PacketProcess한테 넘기기
			{
				m_pPacketProc->Process(packetInfo); // PacketProcess에서의 Process 호출 (방금 받은 패킷을 인자로)
			}
		}
	}
}

ERROR_CODE Server::LoadConfig()
{
	m_pServerConfig = std::make_unique<NServerNetLib::ServerConfig>();

	m_pServerConfig->Port = 11021;
	m_pServerConfig->BackLogCount = 128;
	m_pServerConfig->MaxClientCount = 1000;

	m_pServerConfig->MaxClientSockOptRecvBufferSize = 10240;
	m_pServerConfig->MaxClientSockOptSendBufferSize = 10240;
	m_pServerConfig->MaxClientRecvBufferSize = 8192;
	m_pServerConfig->MaxClientSendBufferSize = 8192;

	m_pServerConfig->ExtraClientCount = 64;
	m_pServerConfig->MaxRoomCount = 20;
	m_pServerConfig->MaxRoomUserCount = 4;
	    
	m_pLogger->Write(NServerNetLib::LOG_TYPE::L_INFO, "%s | Port(%d), Backlog(%d)", __FUNCTION__, m_pServerConfig->Port, m_pServerConfig->BackLogCount);
	return ERROR_CODE::NONE;
}
		
