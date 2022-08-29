#include <algorithm>
#include <string.h>

#include "NetLib/ILog.h"
#include "NetLib/TcpNetwork.h"
#include "Packet.h"
#include "ErrorCode.h"
#include "User.h"
#include "Room.h"
#include "OmokManager.h"

void OmokManager::Init(Room* room)
{
    m_pRoom = room;
}

void OmokManager::Release()
{

}

void OmokManager::PlayerSet(string firstUserID, string secondUserID)
{
    m_black = firstUserID;
    m_white = secondUserID;
}

void OmokManager::Clear()
{
    memset(m_goBoard, 0, sizeof(m_goBoard));
    m_flag = false;
    m_black.clear();
    m_white.clear();

    m_pRoom->EndGame();
}

// 돌을 놓는 메서드
void OmokManager::PutStone(int x, int y, string userID)
{
    if (m_goBoard[x][y] != STONE::NONE)
        return;

    if (m_flag == false && m_black == userID)
    {
        m_goBoard[x][y] = STONE::BLACK;
        m_flag = true;
        m_pRoom->NotifyPutAL(x, y, STONE::BLACK);
        checkOmok(x, y);
    }
    else if (m_flag == true && m_white == userID)
    {
        m_goBoard[x][y] = STONE::WHITE;
        m_flag = false;
        m_pRoom->NotifyPutAL(x, y, STONE::WHITE);
        checkOmok(x, y);
    }
}

// 오목인지 체크하는 메서드
void OmokManager::checkOmok(int x, int y)
{
    int cnt = 1;

    // 오른쪽 방향
    for (int i = x + 1; i <= 18; i++)
    {
        if (m_goBoard[i][y] == m_goBoard[x][y])
            cnt++;
        else
            break;
    }

    // 왼쪽방향
    for (int i = x - 1; i >= 0; i--)
    {
        if (m_goBoard[i][y] == m_goBoard[x][y])
            cnt++;
        else
            break;
    }

    if (cnt >= 5)
    {
        OmokComplete(x, y);
        return;
    }

    cnt = 1;

    // 아래 방향
    for (int i = y + 1; i <= 18; i++)
    {
        if (m_goBoard[x][i] == m_goBoard[x][y])
            cnt++;
        else
            break;
    }

    // 위 방향
    for (int i = y - 1; i >= 0; i--)
    {
        if (m_goBoard[x][i] == m_goBoard[x][y])
            cnt++;
        else
            break;
    }

    if (cnt >= 5)
    {
        OmokComplete(x, y);
        return;
    }

    cnt = 1;

    // 대각선 오른쪽 위방향
    for (int i = x + 1, j = y - 1; i <= 18 && j >= 0; i++, j--)
    {
        if (m_goBoard[i][j] == m_goBoard[x][y])
            cnt++;
        else
            break;
    }

    // 대각선 왼쪽 아래 방향
    for (int i = x - 1, j = y + 1; i >= 0 && j <= 18; i--, j++)
    {
        if (m_goBoard[i][j] == m_goBoard[x][y])
            cnt++;
        else
            break;
    }

    if (cnt >= 5)
    {
        OmokComplete(x, y);
        return;
    }

    cnt = 1;

    // 대각선 왼쪽 위방향
    for (int i = x - 1, j = y - 1; i >= 0 && j >= 0; i--, j--)
    {
        if (m_goBoard[i][j] == m_goBoard[x][y])
            cnt++;
        else
            break;
    }

    // 대각선 오른쪽 아래 방향
    for (int i = x + 1, j = y + 1; i <= 18 && j <= 18; i++, j++)
    {
        if (m_goBoard[i][j] == m_goBoard[x][y])
            cnt++;
        else
            break;
    }

    if (cnt >= 5)
    {
        OmokComplete(x, y);
        return;
    }
}

// 오목이 되었을 떄 처리하는 루틴 
void OmokManager::OmokComplete(int x, int y)
{
    if (m_goBoard[x][y] == STONE::BLACK)
        m_pRoom->NotifyEndGame(m_black);
    else if (m_goBoard[x][y] == STONE::WHITE)
        m_pRoom->NotifyEndGame(m_white);

    Clear();
    /*
    DialogResult res = MessageBox.Show(goBoard[x, y].ToString().ToUpper()
        + " Wins!\n새로운 게임을 시작할까요?", "게임 종료", MessageBoxButtons.YesNo);
    if (res == DialogResult.Yes)
        NewGame();
    else if (res == DialogResult.No)
        this.Close();
        */
}

// 새로운 게임을 시작(초기화)
/*
private void NewGame()
{
    flag = false;

    for (int x = 0; x < 19; x++)
        for (int y = 0; y < 19; y++)
            goBoard[x, y] = STONE.none;

    panel1.Refresh();
    DrawBoard();
    DrawStone();
}
*/