/*==============================================================================
   T h e   K T V L i n u x   P r o j e c t
--------------------------------------------------------------------------------
   Filename   : player.h
   Author(s)  : Silicon
   Copyright  : Silicon

      ²¥·ÅÆ÷¿ØÖÆ½Ó¿Ú£¬Íê³ÉºÍ²¥Æ÷µÄÍ¨ĞÅ
==============================================================================*/

#ifndef PLAYER_H
#define PLAYER_H

#include "osnet.h"
#include "data.h"
#include "selected.h"
#include "windowmanage.h"

//==============================================================================
class CPlayer // ²¥·ÅÆ÷¿ØÖÆÀà
{
public:
	CPlayer(const char *ip=NULL);
	~CPlayer();
public:
	CKtvTheme *theme;
	void SendPlayerCmdNoRecv(const char *cmd);          // ·¢ËÍÍøÂç²¥·ÅÆ÷ÃüÁî, ÎŞ»Ø¸´ÏûÏ¢
	void SendPlayerCmdAndRecv(const char *cmd);         // ·¢ËÍÃüÁî²¢ÇÒµÈ»Ø¸´ÏûÏ
public:
	SelectSongNode* NetAddSongToList(MemSongNode *rec); // ÏòÒÑµã¸èÇúÁĞ±íÖĞÔö¼Ó¼ÇÂ¼
	bool NetDelSongFromList(SelectSongNode *rec);       // ´ÓÒÑµã¸èÇúÁĞ±íÖĞÉ¾³ı¼ÇÂ¼
	int  NetSongIndex(SelectSongNode *rec);             // ÔÚÒÑµã¸èÇúÁĞ±íÖĞµÄË³ĞòºÅ
	bool NetFirstSong(SelectSongNode *rec,int id);      // ÓÅÏÈ¸èÇú

	bool NetSongCodeInList(char *songcode);             // Ö¸¶¨µÄ¸èÇú±àºÅÊÇ·ñÔÚÁĞ±íÖĞ
	void ReloadSongList();                              // ÖØĞÂ´ÓÍøÂç²¥·ÅÆ÷¶ÁÈ¡¸è±í
	bool RecvPlayerCmd();                               // ´¦Àí²¥·ÅÆ÷·¢¹ıÀ´µÄÃüÁî
	bool ReadStrFromPlayer(char *s, int x);             // ¶Á²¥·ÅÆ÷·¢¹ıÀ´µÄ×Ö·û´®
	int  ReadIntFromPlayer();                           // ¶Á²¥·ÅÆ÷·¢¹ıÀ´µÄ×Ö·û´®
	void ClearSocketBuf();                              // Çå¿ÕÌ×½Ó×Ö»º³åÇø
private:
	int RecvUdpBuf(int fd, char *msg, int MaxSize, struct sockaddr *ClientAddr);
	bool RecvSongData(SelectSongNode *rec);             // ½ÓÊÕ²¥·ÅÆ÷·şÎñ·¢»ØµÄ¸èÇú

	int sckfd;                 // UDP ¿Í»§¶Ë·¢ËÍÌ×½Ó×Ö
	int udpsvrfd;              // UDP ·şÎñÌ×½Ó×Ö
	char PlayerAddr[17];       // ²¥·ÅÆ÷µÄIPµØÖ·
	int Volume;                // ²¥·Å´óÒôÁ¿
	int MinVolume, MaxVolume;  // ×î´óÒôÁ¿¡¢×îĞ¡ÒôÁ¿
	bool EnabledSound;         // ÊÇ·ñÔÊĞíµ÷½ÚÒôÁ¿´óĞ¡

	struct sockaddr_in PlayAddr;
	int CurTrack;
	pthread_mutex_t CS;        // UDP ¿Í»§¶Ë·¢ËÍÌ×½Ó×Ö²Ù×÷»¥²ğ
	bool working;
	pthread_t thread;
	friend void *ControlPlayThread(void *k);
	CWindowStack *stack;
};

extern CPlayer *player;
//==============================================================================
#endif
