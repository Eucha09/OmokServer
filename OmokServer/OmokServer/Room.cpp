#include <algorithm>
#include <cstring>
#include <wchar.h>
#include <random>

#include "NetLib/ILog.h"
#include "NetLib/TcpNetwork.h"
#include "Packet.h"
#include "ErrorCode.h"
#include "User.h"
#include "Room.h"



Room::Room() {}

Room::~Room()
{
}
	
void Room::Init(const short index, const short maxUserCount)
{
	m_Index = index;
	m_MaxUserCount = maxUserCount;

	m_pOmokMgr = new OmokManager();
	m_pOmokMgr->Init(this);
}

void Room::SetNetwork(TcpNet* pNetwork, ILog* pLogger)
{
	m_pRefLogger = pLogger;
	m_pRefNetwork = pNetwork;
}

void Room::Clear()
{
	m_IsUsed = false;
	m_UserList.clear();
}

void Room::Enter(User* user)
{
	m_UserList.push_back(user);
}

void Room::Leave(User* user)
{
	for (int i = 0; i < m_UserList.size(); i++)
		if (m_UserList[i] == user)
			m_UserList.erase(m_UserList.begin() + i);
}

void Room::ReadyCheck()
{
	bool allReady = true;

	if (m_UserList.size() != 2)
		return;

	for (int i = 0; i < m_UserList.size(); i++)
		if (!m_UserList[i]->IsReady())
			allReady = false;

	if (allReady)
	{
		m_roomState = ROOM_STATE::GAME;
		for (int i = 0; i < m_UserList.size(); i++)
			m_UserList[i]->GameStart();

		// 게임 선 결정 (랜덤으로)
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dis(0, 1);
		int r = dis(gen);
		m_pOmokMgr->PlayerSet(m_UserList[r]->GetID(), m_UserList[1 - r]->GetID());
		NotifyGameStart(m_UserList[r]->GetID());
	}
}

void Room::EndGame()
{
	m_roomState = ROOM_STATE::NONE;
	for (int i = 0; i < m_UserList.size(); i++)
		m_UserList[i]->EndGame();
}

void Room::ExitUser(User* exitUser)
{
	Leave(exitUser);
	NotifyLeaveUserInfo(exitUser->GetID());

	if (m_roomState == ROOM_STATE::GAME)
	{
		NotifyEndGame(m_UserList[0]->GetID());
		m_pOmokMgr->Clear();
	}
}

void Room::SendToAllUser(const short packetId, const short dataSize, char* pData)
{
	for (auto pUser : m_UserList)
		m_pRefNetwork->SendData(pUser->GetSessioIndex(), packetId, dataSize, pData);
}

void Room::NotifyEnterUserInfo(const std::string userID)
{
	PktRoomEnterUserInfoNtf ntfPkt;

	ntfPkt.UserID_len = userID.size();
	for (int i = 0; i < userID.size(); i++)
		ntfPkt.UserID[i] = userID[i];
	SendToAllUser((short)PACKET_ID::ROOM_ENTER_NEW_USER_NTF, sizeof(ntfPkt), (char*)&ntfPkt);
}

void Room::NotifyLeaveUserInfo(const std::string userID)
{
	PktRoomLeaveUserInfoNtf ntfPkt;

	ntfPkt.UserID_len = userID.size();
	for (int i = 0; i < userID.size(); i++)
		ntfPkt.UserID[i] = userID[i];
	SendToAllUser((short)PACKET_ID::ROOM_LEAVE_USER_NTF, sizeof(ntfPkt), (char*)&ntfPkt);
}

void Room::NotifyChat(const std::string userID, const short msg_len, const wchar_t* msg)
{
	PktRoomChatNtf ntfPkt;

	ntfPkt.UserID_len = userID.size();
	for (int i = 0; i < userID.size(); i++)
		ntfPkt.UserID[i] = userID[i];
	ntfPkt.Mag_len = msg_len;
	for (int i = 0; i < msg_len; i++)
		ntfPkt.Msg[i] = msg[i];
	SendToAllUser((short)PACKET_ID::ROOM_CHAT_NTF, sizeof(ntfPkt), (char*)&ntfPkt);
}

void Room::NotifyReady(const std::string userID)
{
	PktReadyGameRoomNtf ntfPkt;

	ntfPkt.UserID_len = userID.size();
	for (int i = 0; i < userID.size(); i++)
		ntfPkt.UserID[i] = userID[i];
	SendToAllUser((short)PACKET_ID::PK_READY_GAME_ROOM_NTF, sizeof(ntfPkt), (char*)&ntfPkt);
}

void Room::NotifyCancel(const std::string userID)
{
	PktCancelReadyGameRoomNtf ntfPkt;

	ntfPkt.UserID_len = userID.size();
	for (int i = 0; i < userID.size(); i++)
		ntfPkt.UserID[i] = userID[i];
	SendToAllUser((short)PACKET_ID::PK_CANCEL_READY_GAME_ROOM_NTF, sizeof(ntfPkt), (char*)&ntfPkt);
}

void Room::NotifyGameStart(const std::string turn_userID)
{
	PktStartGameRoomNtf ntfPkt;

	ntfPkt.UserID_len = turn_userID.size();
	for (int i = 0; i < turn_userID.size(); i++)
		ntfPkt.TurnUserID[i] = turn_userID[i];
	SendToAllUser((short)PACKET_ID::PK_START_GAME_ROOM_NTF, sizeof(ntfPkt), (char*)&ntfPkt);
}

void Room::NotifyPutAL(const int x, const int y, const OmokManager::STONE stone)
{
	PktPutALGameRoomNtf ntfPkt;

	ntfPkt.XPos = x;
	ntfPkt.YPos = y;
	ntfPkt.stone = (short)stone;
	SendToAllUser((short)PACKET_ID::PK_PUT_AL_ROOM_NTF, sizeof(ntfPkt), (char*)&ntfPkt);
}

void Room::NotifyEndGame(const std::string win_userID)
{
	PktEndGameRoomNtf ntfPkt;

	ntfPkt.UserID_len = win_userID.size();
	for (int i = 0; i < win_userID.size(); i++)
		ntfPkt.WinUserID[i] = win_userID[i];
	SendToAllUser((short)PACKET_ID::PK_END_GAME_ROOM_NTF, sizeof(ntfPkt), (char*)&ntfPkt);
}