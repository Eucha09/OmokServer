#include <cstring>

#include "NetLib/ILog.h"
#include "NetLib/TcpNetwork.h"
#include "User.h"
#include "UserManager.h"
#include "Room.h"
#include "RoomManager.h"
#include "PacketProcess.h"

using LOG_TYPE = NServerNetLib::LOG_TYPE;
using ServerConfig = NServerNetLib::ServerConfig;

	
PacketProcess::PacketProcess() {}
PacketProcess::~PacketProcess() {}

void PacketProcess::Init(TcpNet* pNetwork, UserManager* pUserMgr, RoomManager* pLobbyMgr, ServerConfig* pConfig, ILog* pLogger)
{
	m_pRefLogger = pLogger;
	m_pRefNetwork = pNetwork;
	m_pRefUserMgr = pUserMgr;
	m_pRefRoomMgr = pLobbyMgr;
}
	
void PacketProcess::Process(PacketInfo packetInfo)
{
	using netLibPacketId = NServerNetLib::PACKET_ID;
	using commonPacketId = PACKET_ID;

	auto packetId = packetInfo.PacketId;
		
	// 받은 패킷 Id 별로 처리
	switch (packetId)
	{
	case (int)netLibPacketId::NTF_SYS_CONNECT_SESSION:
		NtfSysConnctSession(packetInfo);
		break;
	case (int)netLibPacketId::NTF_SYS_CLOSE_SESSION:
		NtfSysCloseSession(packetInfo);
		break;
	case (int)commonPacketId::LOGIN_IN_REQ:
		Login(packetInfo);
		break;
	case (int)commonPacketId::ROOM_ENTER_REQ:
		EnterRoom(packetInfo);
		break;
	case (int)commonPacketId::ROOM_LEAVE_REQ:
		LeaveRoom(packetInfo);
		break;
	case (int)commonPacketId::ROOM_CHAT_REQ:
		ChatRoom(packetInfo);
		break;
	}
	
}


ERROR_CODE PacketProcess::NtfSysConnctSession(PacketInfo packetInfo)
{
	m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | NtfSysConnctSession. sessionIndex(%d)", __FUNCTION__, packetInfo.SessionIndex);
	return ERROR_CODE::NONE;
}

ERROR_CODE PacketProcess::NtfSysCloseSession(PacketInfo packetInfo)
{
	auto pUser = std::get<1>(m_pRefUserMgr->GetUser(packetInfo.SessionIndex));

	if (pUser) {		
		m_pRefUserMgr->RemoveUser(packetInfo.SessionIndex);		
	}
			
	m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d)", __FUNCTION__, packetInfo.SessionIndex);
	return ERROR_CODE::NONE;
}


ERROR_CODE PacketProcess::Login(PacketInfo packetInfo)
{
	// 패스워드는 무조건 pass 해준다.
	// ID 중복이라면 에러 처리한다.
	PktLogInRes resPkt;
	auto reqPkt = (PktLogInReq*)packetInfo.pRefData;

	auto addRet = m_pRefUserMgr->AddUser(packetInfo.SessionIndex, reqPkt->szID);

	if (addRet != ERROR_CODE::NONE) {
		resPkt.SetError(addRet);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOGIN_IN_RES, sizeof(PktLogInRes), (char*)&resPkt);
		return addRet;
	}
		
	resPkt.ErrorCode = (short)addRet;
	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOGIN_IN_RES, sizeof(PktLogInRes), (char*)&resPkt);

	return ERROR_CODE::NONE;
}

ERROR_CODE PacketProcess::EnterRoom(PacketInfo packetInfo)
{
	PktRoomEnterRes resPkt;
	PktRoomEnterUserInfoNtf ntfPkt;
	auto reqPkt = (PktRoomEnterReq*)packetInfo.pRefData;
	Room* room = nullptr;
	User* user = nullptr;
	int roomNumber = 0;

	// 에러체크: 이미 방에 들어가 있는지 확인한다
	user = std::get<1>(m_pRefUserMgr->GetUser(packetInfo.SessionIndex));
	if (user->GetRoomIndex() != -1)
	{
		resPkt.SetError(ERROR_CODE::ROOM_MGR_ALREADY_IN_THE_ROOM);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(PktRoomEnterRes), (char*)&resPkt);
		return ERROR_CODE::ROOM_MGR_ALREADY_IN_THE_ROOM;
	}

	// 에러체크: 방 번호가 서버가 생성한 방번호 보다 큰지 확인한다
	if (reqPkt->RoomIndex >= m_pRefRoomMgr->MaxRoomCount())
	{
		resPkt.SetError(ERROR_CODE::ROOM_MGR_INVALID_ROOM_NUMBER);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(PktRoomEnterRes), (char*)&resPkt);
		return ERROR_CODE::ROOM_MGR_INVALID_ROOM_NUMBER;
	}
	
	// 방번호가 -1이면 모든 방을 조사해서 들어갈수 있는 방을 고른다.
	if (reqPkt->RoomIndex == -1)
	{
		for (int i = 0; i < m_pRefRoomMgr->MaxRoomCount(); i++)
		{
			room = m_pRefRoomMgr->GetRoom(i);
			if (room->GetUserCount() > 0)
			{
				roomNumber = i;
				break;
			}
		}
	}
	else
		roomNumber = reqPkt->RoomIndex;

	room = m_pRefRoomMgr->GetRoom(roomNumber);

	//방에 들어간 인원이 이미 다 찬 상태이면 실패이다.
	if (room->GetUserCount() == room->MaxUserCount())
	{
		resPkt.SetError(ERROR_CODE::ROOM_MGR_ROOM_IS_FULL);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(PktRoomEnterRes), (char*)&resPkt);
		return ERROR_CODE::ROOM_MGR_ROOM_IS_FULL;
	}

	// 방에 다른 유저가 있다면 새 유저가 들어온다는 것을 알린다.
	if (room->GetUserCount() > 0)
	{
		std::string userId = user->GetID();
		ntfPkt.UserID_len = userId.size();
		for (int i = 0; i < userId.size(); i++)
			ntfPkt.UserID[i] = userId[i];
			
		std::vector<User*>& userList = room->GetUserLIst();
		for(int i = 0; i < userList.size(); i++)
			m_pRefNetwork->SendData(userList[i]->GetSessioIndex(), (short)PACKET_ID::ROOM_ENTER_NEW_USER_NTF, sizeof(PktRoomEnterUserInfoNtf), (char*)&ntfPkt);
	}

	//원하는 방번호를 가진 방에 입장시킨다.
	room->Enter(user);
	m_pRefLogger->Write(LOG_TYPE::L_INFO, "Room %d Enter %s", roomNumber, user->GetID().c_str());
	//유저 상태를 방 입장 상태로 바꾸고, 방번호를 저장한다
	user->EnterRoom(room->GetIndex());

	// 현재 방에 있는 유저들의 목록을 담아 보낸다.
	std::vector<User*>& userList = room->GetUserLIst();
	resPkt.UserCount = userList.size();
	int writePos = 0;
	for (int i = 0; i < userList.size(); i++)
	{
		std::string userID = userList[i]->GetID();
		resPkt.UserIDList[writePos++] = userID.size();
		for (int j = 0; j < userID.size(); j++)
			resPkt.UserIDList[writePos++] = userID[j];
	}
	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(PktRoomEnterRes), (char*)&resPkt);

	return ERROR_CODE::NONE;
}

//TODO: 방 나가기
ERROR_CODE PacketProcess::LeaveRoom(PacketInfo packetInfo)
{
	PktRoomLeaveRes resPkt;
	PktRoomLeaveUserInfoNtf ntfPkt;
	User* user = nullptr;
	Room* room = nullptr;

	// 유저가 방에 들어온 상태가 맞는지 확인한다
	user = std::get<1>(m_pRefUserMgr->GetUser(packetInfo.SessionIndex));
	if (user->GetRoomIndex() < 0)
	{
		resPkt.SetError(ERROR_CODE::USER_MGR_NOT_IN_THE_ROOM);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(PktRoomLeaveRes), (char*)&resPkt);
		return ERROR_CODE::USER_MGR_NOT_IN_THE_ROOM;
	}

	// 방에서 유저를 빼고, 유저 상태도 변경한다.
	room = m_pRefRoomMgr->GetRoom(user->GetRoomIndex());
	room->Leave(user);
	user->LeaveRoom();
	m_pRefLogger->Write(LOG_TYPE::L_INFO, "Room %d Leave %s", room->GetIndex(), user->GetID().c_str());

	// 아직 방에 다른 유저가 있다면 PK_USER_LEAVE_ROOM_NTF을 보낸다
	std::string userId = user->GetID();
	ntfPkt.UserID_len = userId.size();
	for (int i = 0; i < userId.size(); i++)
		ntfPkt.UserID[i] = userId[i];
	std::vector<User*>& userList = room->GetUserLIst();
	for (int i = 0; i < userList.size(); i++)
		m_pRefNetwork->SendData(userList[i]->GetSessioIndex(), (short)PACKET_ID::ROOM_LEAVE_USER_NTF, sizeof(PktRoomLeaveUserInfoNtf), (char*)&ntfPkt);
	
	// 문제가 없다면 요청이 성공함을 알린다
	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(PktRoomLeaveRes), (char*)&resPkt);
	
	return ERROR_CODE::NONE;
}

//TODO: 방 채팅
ERROR_CODE PacketProcess::ChatRoom(PacketInfo packetInfo)
{
	PktRoomChatRes resPkt;
	PktRoomChatNtf ntfPkt;
	auto reqPkt = (PktRoomChatReq*)packetInfo.pRefData;
	User* user;
	Room* room;

	// 유저가 방에 들어온 상태가 맞는지 확인한다
	user = std::get<1>(m_pRefUserMgr->GetUser(packetInfo.SessionIndex));
	if (user->GetRoomIndex() < 0)
	{
		resPkt.SetError(ERROR_CODE::USER_MGR_NOT_IN_THE_ROOM);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(PktRoomChatRes), (char*)&resPkt);
		return ERROR_CODE::USER_MGR_NOT_IN_THE_ROOM;
	}

	// 메세지 길이가 유효한지 확인한다.
	if (reqPkt->Msg_len >= MAX_ROOM_CHAT_MSG_SIZE)
	{
		resPkt.SetError(ERROR_CODE::CHAT_MSG_IS_TOO_LONG);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(PktRoomChatRes), (char*)&resPkt);
		return ERROR_CODE::CHAT_MSG_IS_TOO_LONG;
	}

	room = m_pRefRoomMgr->GetRoom(user->GetRoomIndex());
	
	// 방의 모든 유저에게 PK_CHAT_ROOM_NTF을 보낸다
	std::string userID = user->GetID();
	ntfPkt.UserID_len = userID.size();
	for (int i = 0; i < userID.size(); i++)
		ntfPkt.UserID[i] = userID[i];
	ntfPkt.Mag_len = reqPkt->Msg_len;
	for (int i = 0; i < reqPkt->Msg_len; i++)
		ntfPkt.Msg[i] = reqPkt->Msg[i];
	//m_pRefLogger->Write(LOG_TYPE::L_INFO, "Room %d Chat %s : %s", room->GetIndex(), user->GetID().c_str(), reqPkt->Msg);
	std::vector<User*>& userList = room->GetUserLIst();
	for (int i = 0; i < userList.size(); i++)
		m_pRefNetwork->SendData(userList[i]->GetSessioIndex(), (short)PACKET_ID::ROOM_CHAT_NTF, sizeof(PktRoomChatNtf), (char*)&ntfPkt);
	
	// 채팅 메시지에 문제가 없다면 요청이 성공함을 알린다
	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(PktRoomChatRes), (char*)&resPkt);

	return ERROR_CODE::NONE;
}