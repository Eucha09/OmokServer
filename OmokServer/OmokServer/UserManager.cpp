#include <algorithm>

#include "User.h"
#include "UserManager.h"


UserManager::UserManager()
{
}

UserManager::~UserManager()
{
}

void UserManager::Init(const int maxUserCount)
{
	for (int i = 0; i < maxUserCount; ++i)
	{
		User user;
		user.Init((short)i);

		m_UserObjPool.push_back(user);
		m_UserObjPoolIndex.push_back(i);
	}
}

User* UserManager::AllocUserObjPoolIndex()
{
	if (m_UserObjPoolIndex.empty()) {
		return nullptr;
	}

	int index = m_UserObjPoolIndex.front();
	m_UserObjPoolIndex.pop_front();
	return &m_UserObjPool[index];
}

void UserManager::ReleaseUserObjPoolIndex(const int index)
{
	m_UserObjPoolIndex.push_back(index);
	m_UserObjPool[index].Clear();
}

// User가 로그인 하였을 때 호출
ERROR_CODE UserManager::AddUser(const int sessionIndex, const char* pszID)
{
	// User 중복 체크
	if (FindUser(pszID) != nullptr) {
		return ERROR_CODE::USER_MGR_ID_DUPLICATION;
	}

	// 안사용하고 있는 User 객체 가져오기
	auto pUser = AllocUserObjPoolIndex();
	if (pUser == nullptr) { // Pooling해놓은 User 객체를 다 사용한 경우
		return ERROR_CODE::USER_MGR_MAX_USER_COUNT;
	}

	pUser->Set(sessionIndex, pszID);
		
	m_UserSessionDic.insert({ sessionIndex, pUser });
	m_UserIDDic.insert({ pszID, pUser });

	return ERROR_CODE::NONE;
}

// User가 접속이 끊어졌을 때 호출
ERROR_CODE UserManager::RemoveUser(const int sessionIndex)
{
	auto pUser = FindUser(sessionIndex);

	if (pUser == nullptr) {
		return ERROR_CODE::USER_MGR_REMOVE_INVALID_SESSION;
	}

	auto index = pUser->GetIndex();
	auto pszID = pUser->GetID();

	m_UserSessionDic.erase(sessionIndex);
	m_UserIDDic.erase(pszID.c_str());
	ReleaseUserObjPoolIndex(index);

	return ERROR_CODE::NONE;
}

std::tuple<ERROR_CODE, User*> UserManager::GetUser(const int sessionIndex)
{
	auto pUser = FindUser(sessionIndex);

	if (pUser == nullptr) {
		return { ERROR_CODE::USER_MGR_INVALID_SESSION_INDEX, nullptr };
	}

	if (pUser->IsConfirm() == false) {
		return{ ERROR_CODE::USER_MGR_NOT_CONFIRM_USER, nullptr };
	}

	return{ ERROR_CODE::NONE, pUser };
}

User* UserManager::FindUser(const int sessionIndex)
{
	auto findIter = m_UserSessionDic.find(sessionIndex);
		
	if (findIter == m_UserSessionDic.end()) {
		return nullptr;
	}
		
	return (User*)findIter->second;
}

User* UserManager::FindUser(const char* pszID)
{
	auto findIter = m_UserIDDic.find(pszID);

	if (findIter == m_UserIDDic.end()) {
		return nullptr;
	}

	return (User*)findIter->second;
}
