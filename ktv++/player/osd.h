#ifndef OSD_H
#define OSD_H

#include "../realmagichwl/include/realmagichwl_uk.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <semaphore.h>

#include "unicode.h"
#include "SDL/SDL_ttf.h"

typedef struct {
	uint8_t R; // ��
	uint8_t G; // ��
	uint8_t B; // ��
	uint8_t A; // ͸����
}RGBA;

typedef struct SCROLOSDTITLE{
	int x, y, w, h;            // ����λ��
	RGBA FrontColor;           // ǰ��ɫ
	RGBA GroundColor;          // ����ɫ
	int FontSize;              // �����С
	int Count;                 // ѭ������
	char Text[512];            // ��ʾ����������
	int Speed;                 // �ƶ��ٶ�
	int32_t Timeout;           // ��ʱ
	int32_t Tick;              // �ڲ�ʹ��, ����ִ�л��ѵ�ʱ��
	int Clear;                 // �����Ļ
	int Running;               // �ڲ�ʹ��, 1, ������ʾ��, 0: �˳���ʾ
	SDL_Surface *Surface;      // ��ʾ��ͼ����
	struct SCROLOSDTITLE *Next;
}ScrollText;

typedef struct tagThreadList { // ���߳���Ĭ��ֵ
	int x, y;                  // X,Y����
	RGBA FrontColor;           // Ĭ�ϵ�ǰ��ɫ
	RGBA GroundColor;          // Ĭ�ϵı���ɫ
	int FontSize;              // Ĭ�ϵ������С
	int Count;                 // Ĭ�ϵ�ѭ������
	int Speed;                 // Ĭ�ϵ��ƶ��ٶ�
	int32_t Timeout;           // Ĭ�ϵĳ�ʱʱ��
	int Clear;                 // Ĭ������
	int Running;               // �߳�ʹ��״̬

	sem_t g_sem;               // �źŵ�,���ڿ�����Ļ
	pthread_mutex_t mutex;     // ����, ������Ļ����
	ScrollText *Scroll, *LastScroll, *WorkScroll; // Scroll    : ͷ��Ļָ��
	                                              // LastScroll: ���������Ļָ��
	                                              // WorkScroll: ��ǰ��ʾ�е���Ļָ��
	pthread_t WorkThread;
} ThreadList;

///////////////////////////////////////////////////////////////////////////////
int  OSD_Create(int cardid);                       // ��cardid ���ѹ��OSD��ʼ��
void OSD_Quit(void);                               // OSD�˳�
void OSD_CleanScreen(void);                        // OSD ����
int  CreateThreadList(const char *text);           // ����text��Ĭ��ֵ������ʾ�߳�
int  CreateScrollTextStr(int id, const char *text);// ��ID����Ļ��������Ļ text
void CleanScrollTextList(int id);                  // ��յ�ID����Ļ��δ��ʾ����Ļ
///////////////////////////////////////////////////////////////////////////////

enum {
	psPlaying,
	psPause,
	psMute,
	psKlok,
	psSound
};

void ShowPlayerStatus();

#endif
