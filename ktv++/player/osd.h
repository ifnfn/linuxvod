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
	uint8_t R; // 红
	uint8_t G; // 绿
	uint8_t B; // 蓝
	uint8_t A; // 透明度
}RGBA;

typedef struct SCROLOSDTITLE{
	int x, y, w, h;            // 坐标位置
	RGBA FrontColor;           // 前景色
	RGBA GroundColor;          // 背景色
	int FontSize;              // 字体大小
	int Count;                 // 循环次数
	char Text[512];            // 显示的文字内容
	int Speed;                 // 移动速度
	int32_t Timeout;           // 超时
	int32_t Tick;              // 内部使用, 程序执行花费的时间
	int Clear;                 // 清空屏幕
	int Running;               // 内部使用, 1, 正在显示中, 0: 退出显示
	SDL_Surface *Surface;      // 显示的图形区
	struct SCROLOSDTITLE *Next;
}ScrollText;

typedef struct tagThreadList { // 该线程中默认值
	int x, y;                  // X,Y坐标
	RGBA FrontColor;           // 默认的前景色
	RGBA GroundColor;          // 默认的背景色
	int FontSize;              // 默认的字体大小
	int Count;                 // 默认的循环次数
	int Speed;                 // 默认的移动速度
	int32_t Timeout;           // 默认的超时时间
	int Clear;                 // 默认清屏
	int Running;               // 线程使用状态

	sem_t g_sem;               // 信号灯,用于控制字幕
	pthread_mutex_t mutex;     // 互斥, 操作字幕链表
	ScrollText *Scroll, *LastScroll, *WorkScroll; // Scroll    : 头字幕指针
	                                              // LastScroll: 最后插入的字幕指针
	                                              // WorkScroll: 当前显示中的字幕指针
	pthread_t WorkThread;
} ThreadList;

///////////////////////////////////////////////////////////////////////////////
int  OSD_Create(int cardid);                       // 第cardid 块解压卡OSD初始化
void OSD_Quit(void);                               // OSD退出
void OSD_CleanScreen(void);                        // OSD 清屏
int  CreateThreadList(const char *text);           // 根据text的默认值建立显示线程
int  CreateScrollTextStr(int id, const char *text);// 第ID个字幕区增加字幕 text
void CleanScrollTextList(int id);                  // 清空第ID个字幕区未显示的字幕
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
