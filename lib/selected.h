/*==============================================================================
   T h e   K T V L i n u x   P r o j e c t
 -------------------------------------------------------------------------------
   Filename   : selected.h
   Author(s)  : Silicon
   Copyright  : Silicon

   一些扩展的字符串操作函数，以及相关需要的函数
==============================================================================*/

/*============================================================================*/
#ifndef SELECTED_H
#define SELECTED_H

#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#ifdef WIN32
	#include <windows.h>
#else
	#include <semaphore.h>
#endif
#include <pthread.h>

/*============================================================================*/

typedef struct tagSelectSongNode{
	long ID;
	char SongCode[9];     // 歌曲编号
	char SongName[51];    // 歌曲名称
	char Charset;         // 字符集
	char Language[9];     // 语言
	char SingerName[19];  // 歌星姓名
	unsigned char VolumeK;// 音量值
	unsigned char VolumeS;// 音量值
	char Num;             // 字数
	char Klok;            // 原伴音轨
	char Sound;           // 原唱音轨
	char SoundMode;       // 声音模式
	char StreamType[6];   // 流格式
	long Password;        // 
}SelectSongNode;

typedef struct tagPLAYSONGLIST {
	long MaxID;           // 最大编号，自增
	int count;            // 歌曲数量
	int MaxCount;         // 曾经最大记录数

	pthread_mutex_t lock;
	SelectSongNode *items;// 歌曲资料数组
} PlaySongList;

extern PlaySongList SelectedList;

extern const char *ADDSONG;
extern const char *DELSONG;
extern const char *FIRSTSONG;
extern const char *LISTSONG;
extern const char *PLAYSONG;

#ifdef __cplusplus
extern "C" {
#endif

inline void NoSongUnlock(void);
inline void NoSongLock(void);
void InitSongList(void);
void ClearSongList(void);                                        /* 清空本地已点歌曲列表            */
SelectSongNode* AddSongToList(SelectSongNode *rec, bool autoinc);/* 向已点歌曲列表中增加记录,返回ID */
SelectSongNode* AddSongToListFrist(SelectSongNode *rec);         /* 向已点歌曲列表增加第一首        */
bool DelSongIndex(int index);                                    /* 删除指定位置的歌曲              */
bool DelSongFromList(SelectSongNode *rec);                       /* 从已点歌曲列表中删除记录        */
int  SongIndex(SelectSongNode *rec);                             /* 在已点歌曲列表中的顺序号        */
bool FirstSong(SelectSongNode *rec,int id);                      /* 优先歌曲                        */
bool SongCodeInList(const char *songcode);                       /* 指定的歌曲编号是否在列表中      */
char *SelectSongNodeToStr(const char *cmd, SelectSongNode *rec); /* 将歌曲结构，转换成字符串        */
bool StrToSelectSongNode(const char *msg, SelectSongNode *rec);  /* 将字符串转换成歌曲结构          */
void PrintSelectSong(void);

void PrintSelectSongNode(SelectSongNode Node);
#ifdef __cplusplus
}
#endif

#endif
