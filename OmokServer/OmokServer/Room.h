#pragma once

#include <vector>
#include <string>
#include <memory>

#include "User.h"
#include "OmokManager.h"


namespace NServerNetLib { class ITcpNetwork; }
namespace NServerNetLib { class ILog; }


	
using TcpNet = NServerNetLib::ITcpNetwork;
using ILog = NServerNetLib::ILog;

class Game;

class Room
{
public:
	enum class ROOM_STATE {
		NONE = 0,
		GAME = 1,
		FINISH = 2,
	};

	Room();
	virtual ~Room();

	void Init(const short index, const short maxUserCount);
	
	void SetNetwork(TcpNet* pNetwork, ILog* pLogger);

	void Clear();
		
	short GetIndex() { return m_Index;  }

	bool IsUsed() { return m_IsUsed; }
		
	short MaxUserCount() { return m_MaxUserCount; }

	short GetUserCount() { return (short)m_UserList.size(); }
	
	void Enter(User* user);

	void Leave(User* user);

	void ReadyCheck();

	void EndGame();

	void ExitUser(User* exitUser);

	void SendToAllUser(const short packetId, const short dataSize, char* pData);

	void NotifyEnterUserInfo(const std::string userID);

	void NotifyLeaveUserInfo(const std::string userID);

	void NotifyChat(const std::string userID, const short msg_len, const wchar_t* msg);

	void NotifyReady(const std::string userID);

	void NotifyCancel(const std::string userID);

	void NotifyGameStart(const std::string turn_userID);

	void NotifyPutAL(const int x, const int y, const OmokManager::STONE stone);

	void NotifyEndGame(const std::string win_userID);

	std::vector<User*>& GetUserLIst() { return m_UserList; }

	OmokManager* GetOmokManager() { return m_pOmokMgr; }

	ROOM_STATE GetRoomState() { return m_roomState; }

private:
	ILog* m_pRefLogger;
	TcpNet* m_pRefNetwork;

	OmokManager* m_pOmokMgr;

	short m_Index = -1;
	short m_MaxUserCount;
		
	bool m_IsUsed = false;
	std::vector<User*> m_UserList;

	ROOM_STATE m_roomState = ROOM_STATE::NONE;
	//Game* m_pGame = nullptr;
};
