#pragma once
#pragma once

#include <vector>
#include <string>
#include <unordered_map>
using namespace std;

class User;
class Room;

class OmokManager
{
public:
	enum class STONE {
		NONE = 0,
		BLACK = 1,
		WHITE = 2,
	};

public:
	OmokManager() = default;
	virtual ~OmokManager() = default;

	void Init(Room* Room);

	void Release();

	void Clear();

	void PlayerSet(string firstUserID, string secondUserID);

	void checkOmok(int x, int y);

	void OmokComplete(int x, int y);

	void PutStone(int x, int y, string userID);

protected:
	Room* m_pRoom;

	STONE m_goBoard[19][19];
	bool m_flag = false;  // false = ∞À¿∫ µπ, true = »Úµπ
	
	string m_black;
	string m_white;
};


