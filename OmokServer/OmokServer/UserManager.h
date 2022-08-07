#pragma once
#include <unordered_map>
#include <deque>
#include <string>
#include <vector>

#include "ErrorCode.h"
	
class User;

class UserManager // 로그인된 유저 관리
{
public:
	UserManager();
	virtual ~UserManager();

	void Init(const int maxUserCount);

	ERROR_CODE AddUser(const int sessionIndex, const char* pszID); // 유저 추가
	ERROR_CODE RemoveUser(const int sessionIndex); // 유저 삭제

	std::tuple<ERROR_CODE,User*> GetUser(const int sessionIndex);

				
private:
	User* AllocUserObjPoolIndex();
	void ReleaseUserObjPoolIndex(const int index);

	User* FindUser(const int sessionIndex);
	User* FindUser(const char* pszID);
				
private:
	// Object Pooling
	std::vector<User> m_UserObjPool; // 미리 User 객체를 생성해놓고 보관
	std::deque<int> m_UserObjPoolIndex; // 안쓰고 있는 객체 Index 보관

	// User 객체를 찾을 때
	std::unordered_map<int, User*> m_UserSessionDic; // SessionIndex로 찾고 싶을 때
	std::unordered_map<const char*, User*> m_UserIDDic; // UserID로 찾고 싶을 때

};
