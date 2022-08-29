#pragma once
#include <string>


class User
{
public :
	enum class DOMAIN_STATE {
		NONE = 0,
		LOGIN = 1,
		ROOM = 2,
		READY = 3,
		GAME = 4,
	};

public:
	User() {}
	virtual ~User() {}

	void Init(const short index)
	{
		m_Index = index;
	}

	void Clear()
	{			
		m_SessionIndex = 0;
		m_ID = "";
		m_IsConfirm = false;
		m_CurDomainState = DOMAIN_STATE::NONE;
		m_RoomIndex = -1;
	}

	void Set(const int sessionIndex, const char* pszID)
	{
		m_IsConfirm = true;
		m_CurDomainState = DOMAIN_STATE::LOGIN;

		m_SessionIndex = sessionIndex;
		m_ID = pszID;
	}

	short GetIndex() { return m_Index; }

	int GetSessioIndex() { return m_SessionIndex;  }

	std::string& GetID() { return m_ID;  }

	bool IsConfirm() { return m_IsConfirm;  }
		
	short GetRoomIndex() { return m_RoomIndex; }

	void EnterRoom(const short roomIndex)
	{
		m_RoomIndex = roomIndex;
		m_CurDomainState = DOMAIN_STATE::ROOM;
	}

	void LeaveRoom()
	{
		m_RoomIndex = -1;
		m_CurDomainState = DOMAIN_STATE::LOGIN;
	}

	void GameReady()
	{
		m_CurDomainState = DOMAIN_STATE::READY;
	}

	void CancelGameReady()
	{
		m_CurDomainState = DOMAIN_STATE::ROOM;
	}

	void GameStart()
	{
		m_CurDomainState = DOMAIN_STATE::GAME;
	}

	void EndGame()
	{
		m_CurDomainState = DOMAIN_STATE::ROOM;
	}

	bool IsReady() { return m_CurDomainState == DOMAIN_STATE::READY; }

	DOMAIN_STATE GetUserState() { return m_CurDomainState; }

	bool IsCurDomainInLogIn() {
		return m_CurDomainState == DOMAIN_STATE::LOGIN ? true : false;
	}

	bool IsCurDomainInRoom() {
		return m_CurDomainState == DOMAIN_STATE::ROOM ? true : false;
	}

		
protected:
	short m_Index = -1; // Pooling되어 있는 User 객체 Index
		
	int m_SessionIndex = -1; // 서버 네트워크에서의 Session Index

	std::string m_ID; // User ID
		
	bool m_IsConfirm = false;
		
	DOMAIN_STATE m_CurDomainState = DOMAIN_STATE::NONE; // User 상태 (로비에 있는 상태인지, 방에 들어간 상태인지 등)

	short m_RoomIndex = -1; // Room Index
};
