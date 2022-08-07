#pragma once
#include <memory>

#include "Packet.h"
#include "ErrorCode.h"


namespace NServerNetLib
{
	struct ServerConfig;
	class ILog;
	class ITcpNetwork;
}


class UserManager;
class RoomManager;
class PacketProcess;

class Server
{
public:
	Server();
	~Server();

	ERROR_CODE Init();

	void Run();

	void Stop();


private:
	ERROR_CODE LoadConfig();

	void Release();


private:
	bool m_IsRun = false;

	std::unique_ptr<NServerNetLib::ServerConfig> m_pServerConfig; // 서버 설정 정보
	std::unique_ptr<NServerNetLib::ILog> m_pLogger; // 로그 정보

	std::unique_ptr<NServerNetLib::ITcpNetwork> m_pNetwork; // TcpNetwork 클래스 (NetLib)
	
	std::unique_ptr<PacketProcess> m_pPacketProc; // PacketProcess 클래스
	
	std::unique_ptr<UserManager> m_pUserMgr; // 로그인된 클라이언트 관리
	
	std::unique_ptr<RoomManager> m_pRoomMgr; // 룸 관리
		
};

