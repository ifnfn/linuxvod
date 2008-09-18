#ifndef	MTVTYPE_H
#define MTVTYPE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef enum tagMtvKey {
	DIKC_NULL=0,
	DIKC_ANY=-1,
	DIKC_KLOK=1,
	DIKC_NEXT,
	DIKC_REPLAY,
	DIKC_PAUSE,
	DIKC_SELECT,
	DIKC_VIP,
	DIKC_HIFI,
	DIKC_RECORD,
	DIKC_PLAYREC,
	DIKC_FIRST,
	DIKC_DELETE,
	DIKC_VOLUME_UP,
	DIKC_VOLUME_DOWN,
	DIKC_MUTE,
	DIKC_LEFT,
	DIKC_RIGHT,
	DIKC_ENTER,
	DIKC_ESCAPE,
	DIKC_PAGE_UP,
	DIKC_PAGE_DOWN,
	DIKC_DOWN,
	DIKC_UP,
	DIKC_TVVGA,
	DIKC_SERVICE,
	DIKC_MENU,
	DIKC_NEWS,
	DIKC_NET,
	DIKC_WINE,
	DIKC_PLUNGINS,
	DIKC_SWITCHLANG,
	DIKC_PINYIN,
	DIKC_SINGER,
	DIKC_LANG,
	DIKC_CODE,
	DIKC_CLASS,
	DIKC_SORT,
	DIKC_WORDNUM,
	DIKC_WBH,
	DIKC_QUIT,
	DIKC_DJ1,
	DIKC_DJ2,
	DIKC_DJ3,
	DIKC_DJ4,
	DIKC_1,
	DIKC_2,
	DIKC_3,
	DIKC_4,
	DIKC_5,
	DIKC_6,
	DIKC_7,
	DIKC_8,
	DIKC_9,
	DIKC_0,
	DIKC_A,
	DIKC_B,
	DIKC_C,
	DIKC_D,
	DIKC_E,
	DIKC_F,
	DIKC_G,
	DIKC_H,
	DIKC_I,
	DIKC_J,
	DIKC_K,
	DIKC_L,
	DIKC_M,
	DIKC_N,
	DIKC_O,
	DIKC_P,
	DIKC_Q,
	DIKC_R,
	DIKC_S,
	DIKC_T,
	DIKC_U,
	DIKC_V,
	DIKC_W,
	DIKC_X,
	DIKC_Y,
	DIKC_Z,
}MtvKey_t;

#define MAX_KEY_STRUCT DIKC_Z + 3
typedef struct tagKeyStruct{
	char name[20];
	MtvKey_t mtv_value;
	int32_t sys_value;
	char title[18];
}key_struct;

extern key_struct KeyMaps[MAX_KEY_STRUCT];

//==============================================================================

void PrintKeyMaps();

int FindMtvKey(uint16_t sys_value);                  //  将DirectFB 键盘按键转换成 MtvKey
uint16_t MtvKeyToSysKey(int mtvkey);                 //  将MtvKey 键盘按键转换成 DirectFB
key_struct *FindMtvKeyByKeyName(const char *KeyName);//  根据键名到找到按键的定义
int FindMtvValueByKeyName(const char *KeyName);      //  根据键名，找到Mtv键值
key_struct *FindMtvKeyByKeyValue(uint16_t sys_value);
//==============================================================================
#endif
