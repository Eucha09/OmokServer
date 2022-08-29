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
	case (int)commonPacketId::PK_READY_GAME_ROOM_REQ:
		GameReadyRoom(packetInfo);
		break;
	case (int)commonPacketId::PK_CANCEL_READY_GAME_ROOM_REQ:
		CancelGameReadyRoom(packetInfo);
	case (int)commonPacketId::PK_PUT_AL_ROOM_REQ:
		PutAL(packetInfo);
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
	Room* room;

	if (pUser) {
		if (pUser->GetRoomIndex() >= 0)
		{
			room = m_pRefRoomMgr->GetRoom(pUser->GetRoomIndex());
			room->ExitUser(pUser);
		}
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
		room->NotifyEnterUserInfo(user->GetID());

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

	// room이 게임중이라면
	if (room->GetRoomState() == Room::ROOM_STATE::GAME)
	{
		string winUserID;
		for (int i = 0; i < room->GetUserCount(); i++)
			if (room->GetUserLIst()[i]->GetID() != user->GetID())
				winUserID = room->GetUserLIst()[i]->GetID();
		room->NotifyEndGame(winUserID);
		room->GetOmokManager()->Clear();
	}

	room->Leave(user);
	user->LeaveRoom();
	m_pRefLogger->Write(LOG_TYPE::L_INFO, "Room %d Leave %s", room->GetIndex(), user->GetID().c_str());

	// 아직 방에 다른 유저가 있다면 PK_USER_LEAVE_ROOM_NTF을 보낸다
	room->NotifyLeaveUserInfo(user->GetID());

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
	room->NotifyChat(user->GetID(), reqPkt->Msg_len, reqPkt->Msg);

	//m_pRefLogger->Write(LOG_TYPE::L_INFO, "Room %d Chat %s : %s", room->GetIndex(), user->GetID().c_str(), reqPkt->Msg);
	
	// 채팅 메시지에 문제가 없다면 요청이 성공함을 알린다
	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(PktRoomChatRes), (char*)&resPkt);

	return ERROR_CODE::NONE;
}

ERROR_CODE PacketProcess::GameReadyRoom(PacketInfo packetInfo)
{
	PktReadyGameRoomRes resPkt;
	PktReadyGameRoomNtf ntfPkt;
	auto reqPkt = (PktReadyGameRoomReq*)packetInfo.pRefData;
	User* user;
	Room* room;

	user = std::get<1>(m_pRefUserMgr->GetUser(packetInfo.SessionIndex));
	if (user->GetRoomIndex() < 0)
	{
		resPkt.SetError(ERROR_CODE::USER_MGR_NOT_IN_THE_ROOM);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(PktRoomChatRes), (char*)&resPkt);
		return ERROR_CODE::USER_MGR_NOT_IN_THE_ROOM;
	}
	
	// user가 준비가 안되있는 상태라면 준비상태로 바꾸고 다른 유저들에게 알리기
	if (user->GetUserState() == User::DOMAIN_STATE::ROOM)
	{
		user->GameReady();

		room = m_pRefRoomMgr->GetRoom(user->GetRoomIndex());

		room->NotifyReady(user->GetID());

		room->ReadyCheck();
	}

	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::PK_READY_GAME_ROOM_RES, sizeof(PktReadyGameRoomRes), (char*)&resPkt);
	
	return ERROR_CODE::NONE;
}

ERROR_CODE PacketProcess::CancelGameReadyRoom(PacketInfo packetInfo)
{
	PktCancelReadyGameRoomRes resPkt;
	PktCancelReadyGameRoomNtf ntfPkt;
	auto reqPkt = (PktCancelReadyGameRoomReq*)packetInfo.pRefData;
	User* user;
	Room* room;

	user = std::get<1>(m_pRefUserMgr->GetUser(packetInfo.SessionIndex));
	if (user->GetRoomIndex() < 0)
	{
		resPkt.SetError(ERROR_CODE::USER_MGR_NOT_IN_THE_ROOM);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(PktRoomChatRes), (char*)&resPkt);
		return ERROR_CODE::USER_MGR_NOT_IN_THE_ROOM;
	}

	// user가 준비상태라면 준비 취소하고 다른 유저들에게 알리기
	if (user->GetUserState() == User::DOMAIN_STATE::READY)
	{
		user->CancelGameReady();

		room = m_pRefRoomMgr->GetRoom(user->GetRoomIndex());

		room->NotifyCancel(user->GetID());
	}

	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::PK_CANCEL_READY_GAME_ROOM_RES, sizeof(PktCancelReadyGameRoomRes), (char*)&resPkt);

	return ERROR_CODE::NONE;
}

ERROR_CODE PacketProcess::PutAL(PacketInfo packetInfo)
{
	PktPutALGameRoomRes resPkt;
	auto reqPkt = (PktPutALGameRoomReq*)packetInfo.pRefData;
	User* user;
	Room* room;

	user = std::get<1>(m_pRefUserMgr->GetUser(packetInfo.SessionIndex));
	if (user->GetRoomIndex() < 0)
	{
		resPkt.SetError(ERROR_CODE::USER_MGR_NOT_IN_THE_ROOM);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(PktRoomChatRes), (char*)&resPkt);
		return ERROR_CODE::USER_MGR_NOT_IN_THE_ROOM;
	}
	if (user->GetUserState() != User::DOMAIN_STATE::GAME)
	{
		resPkt.SetError(ERROR_CODE::USER_MGR_NOT_PLAYING_GAME);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::PK_PUT_AL_ROOM_RES, sizeof(PktPutALGameRoomRes), (char*)&resPkt);
		return ERROR_CODE::USER_MGR_NOT_PLAYING_GAME;
	}

	m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s Put Stone %d %d", user->GetID().c_str(), reqPkt->XPos, reqPkt->YPos);

	room = m_pRefRoomMgr->GetRoom(user->GetRoomIndex());

	room->GetOmokManager()->PutStone(reqPkt->XPos, reqPkt->YPos, user->GetID());

	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::PK_PUT_AL_ROOM_RES, sizeof(PktPutALGameRoomRes), (char*)&resPkt);

	return ERROR_CODE::NONE;
}