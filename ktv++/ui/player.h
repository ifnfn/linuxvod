/*==============================================================================
   T h e   K T V L i n u x   P r o j e c t
--------------------------------------------------------------------------------
   Filename   : player.h
   Author(s)  : Silicon
   Copyright  : Silicon

      ���������ƽӿڣ���ɺͲ�����ͨ��
==============================================================================*/

#ifndef PLAYER_H
#define PLAYER_H

#include "osnet.h"
#include "data.h"
#include "selected.h"
#include "windowmanage.h"

//==============================================================================
class CPlayer // ������������
{
public:
	CPlayer(const char *ip=NULL);
	~CPlayer();
public:
	CKtvTheme *theme;
	bool SendPlayerCmdNoRecv(const char *cmd);      // �������粥��������, �޻ظ���Ϣ
	bool SendPlayerCmdAndRecv(const char *cmd);     // ��������ҵȻظ���Ϣ
public:
	void NetAddSongToList(MemSongNode *rec);        // ���ѵ�����б������Ӽ�¼
	bool NetDelSongFromList(SelectSongNode *rec);   // ���ѵ�����б���ɾ����¼
	bool NetFirstSong(SelectSongNode *rec, int id); // ���ȸ���

	bool NetSongCodeInList(const char *songcode);   // ָ���ĸ�������Ƿ����б���
	void ReloadSongList();                          // ���´����粥������ȡ���
	bool RecvPlayerCmd();                           // ��������������������
	bool ReadStrFromPlayer(char *s, int x);         // �����������������ַ���
	void PlayDisc();                                // ���Ź���
	bool CheckVideoCard();                          // ����ѹ��
	int  AddVolume();                               // ��������
	int  DecVolume();                               // ��С����
private:
	int RecvUdpBuf(int fd, char *msg, int MaxSize, struct sockaddr *ClientAddr);
	bool RecvSongData(SelectSongNode *rec);         // ���ղ��������񷢻صĸ���
	int  ReadIntFromPlayer();                       // ��������������������
	void ClearSocketBuf();                          // ����׽��ֻ�����

	int sckfd;                 // UDP �ͻ��˷����׽���
	int udpsvrfd;              // UDP �����׽���
	char PlayerAddr[17];       // ��������IP��ַ
	int Volume;                // ���Ŵ�����
	int MinVolume, MaxVolume;  // �����������С����
	bool EnabledSound;         // �Ƿ��������������С

	struct sockaddr_in PlayAddr;
	int CurTrack;
	pthread_mutex_t CS;        // UDP �ͻ��˷����׽��ֲ�������
	bool working;
	pthread_t thread;
	friend void *ControlPlayThread(void *k);
	CWindowStack *stack;
};

extern CPlayer *player;
//==============================================================================
#endif
