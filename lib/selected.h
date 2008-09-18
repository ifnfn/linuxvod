/*==============================================================================
   T h e   K T V L i n u x   P r o j e c t
 -------------------------------------------------------------------------------
   Filename   : selected.h
   Author(s)  : Silicon
   Copyright  : Silicon

   һЩ��չ���ַ��������������Լ������Ҫ�ĺ���
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
	char SongCode[9];     // �������
	char SongName[51];    // ��������
	char Charset;         // �ַ���
	char Language[9];     // ����
	char SingerName[19];  // ��������
	unsigned char VolumeK;// ����ֵ
	unsigned char VolumeS;// ����ֵ
	char Num;             // ����
	char Klok;            // ԭ������
	char Sound;           // ԭ������
	char SoundMode;       // ����ģʽ
	char StreamType[6];   // ����ʽ
	long Password;        // 
}SelectSongNode;

typedef struct tagPLAYSONGLIST {
	long MaxID;           // ����ţ�����
	int count;            // ��������
	int MaxCount;         // ��������¼��

	pthread_mutex_t lock;
	SelectSongNode *items;// ������������
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
void ClearSongList(void);                                        /* ��ձ����ѵ�����б�            */
SelectSongNode* AddSongToList(SelectSongNode *rec, bool autoinc);/* ���ѵ�����б������Ӽ�¼,����ID */
SelectSongNode* AddSongToListFrist(SelectSongNode *rec);         /* ���ѵ�����б����ӵ�һ��        */
bool DelSongIndex(int index);                                    /* ɾ��ָ��λ�õĸ���              */
bool DelSongFromList(SelectSongNode *rec);                       /* ���ѵ�����б���ɾ����¼        */
int  SongIndex(SelectSongNode *rec);                             /* ���ѵ�����б��е�˳���        */
bool FirstSong(SelectSongNode *rec,int id);                      /* ���ȸ���                        */
bool SongCodeInList(const char *songcode);                       /* ָ���ĸ�������Ƿ����б���      */
char *SelectSongNodeToStr(const char *cmd, SelectSongNode *rec); /* �������ṹ��ת�����ַ���        */
bool StrToSelectSongNode(const char *msg, SelectSongNode *rec);  /* ���ַ���ת���ɸ����ṹ          */
void PrintSelectSong(void);

void PrintSelectSongNode(SelectSongNode Node);
#ifdef __cplusplus
}
#endif

#endif
