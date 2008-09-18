/*==============================================================================
   T h e   K T V L i n u x   P r o j e c t
--------------------------------------------------------------------------------
   Filename   : player.h
   Author(s)  : Silicon
   Copyright  : Silicon

      播放器控制接口，完成和播器的通信
==============================================================================*/

#ifndef PLAYER_H
#define PLAYER_H

#include "osnet.h"
#include "data.h"
#include "selected.h"
#include "windowmanage.h"

//==============================================================================
class CPlayer // 播放器控制类
{
public:
	CPlayer(const char *ip=NULL);
	~CPlayer();
public:
	CKtvTheme *theme;
	bool SendPlayerCmdNoRecv(char *cmd);            // 发送网络播放器命令, 无回复消息
	bool SendPlayerCmdAndRecv(char *cmd);           // 发送命令并且等回复消息
public:
	void NetAddSongToList(MemSongNode *rec);        // 向已点歌曲列表中增加记录
	bool NetDelSongFromList(SelectSongNode *rec);   // 从已点歌曲列表中删除记录
	bool NetFirstSong(SelectSongNode *rec, int id); // 优先歌曲

	bool NetSongCodeInList(const char *songcode);   // 指定的歌曲编号是否在列表中
	void ReloadSongList();                          // 重新从网络播放器读取歌表
	bool RecvPlayerCmd();                           // 处理播放器发过来的命令
	void PlayDisc();                                // 播放光驱
	bool CheckVideoCard();                          // 检查解压卡
	int  AddVolume();                               // 增加音量
	int  DecVolume();                               // 减小音量
private:
	char *SendPlayerCmd(char *format, ...);
	void GetURL(char *data, const char *format, ...);
	int RecvUdpBuf(int fd, char *msg, int MaxSize);
	void ClearSocketBuf();                          // 清空套接字缓冲区

	int udpsvrfd;              // UDP 服务套接字
	char PlayerAddr[17];       // 播放器的IP地址
	int Volume;                // 播放大音量
	int MinVolume, MaxVolume;  // 最大音量、最小音量
	bool EnabledSound;         // 是否允许调节音量大小

	int CurTrack;
	bool working;
	pthread_t thread;
	friend void *ControlPlayThread(void *k);
	CWindowStack *stack;
};

extern CPlayer *player;
//==============================================================================
#endif
