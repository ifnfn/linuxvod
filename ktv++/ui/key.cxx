#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memtest.h"
#include "key.h"

#define REGENTRY(REG_NAME,value) {#REG_NAME, REG_NAME, value, #REG_NAME},

key_struct KeyMaps[MAX_KEY_STRUCT] ={
//	name[25] 	mtv_value   	sys_value, title;
	REGENTRY(DIKC_ANY,          -1)
	REGENTRY(DIKC_NULL,          0)
	REGENTRY(DIKC_1,           '1')
	REGENTRY(DIKC_2,           '2')
	REGENTRY(DIKC_3,           '3')
	REGENTRY(DIKC_4,           '4')
	REGENTRY(DIKC_5,           '5')
	REGENTRY(DIKC_6,           '6')
	REGENTRY(DIKC_7,           '7')
	REGENTRY(DIKC_8,           '8')
	REGENTRY(DIKC_9,           '9')
	REGENTRY(DIKC_0,           '0')
	REGENTRY(DIKC_A,           'A')
	REGENTRY(DIKC_B,           'B')
	REGENTRY(DIKC_C,           'C')
	REGENTRY(DIKC_D,           'D')
	REGENTRY(DIKC_E,           'E')
	REGENTRY(DIKC_F,           'F')
	REGENTRY(DIKC_G,           'G')
	REGENTRY(DIKC_H,           'H')
	REGENTRY(DIKC_I,           'I')
	REGENTRY(DIKC_J,           'J')
	REGENTRY(DIKC_K,           'K')
	REGENTRY(DIKC_L,           'L')
	REGENTRY(DIKC_M,           'M')
	REGENTRY(DIKC_N,           'N')
	REGENTRY(DIKC_O,           'O')
	REGENTRY(DIKC_P,           'P')
	REGENTRY(DIKC_Q,           'Q')
	REGENTRY(DIKC_R,           'R')
	REGENTRY(DIKC_S,           'S')
	REGENTRY(DIKC_T,           'T')
	REGENTRY(DIKC_U,           'U')
	REGENTRY(DIKC_V,           'V')
	REGENTRY(DIKC_W,           'W')
	REGENTRY(DIKC_X,           'X')
	REGENTRY(DIKC_Y,           'Y')
	REGENTRY(DIKC_Z,           'Z')

	REGENTRY(DIKC_KLOK,         107)
	REGENTRY(DIKC_NEXT,         115)
	REGENTRY(DIKC_REPLAY,       114)
	REGENTRY(DIKC_PAUSE,        97)
	REGENTRY(DIKC_SELECT,       118)
	REGENTRY(DIKC_VIP,          105)
	REGENTRY(DIKC_HIFI,         104)
	REGENTRY(DIKC_RECORD,       119)
	REGENTRY(DIKC_PLAYREC,      112)
	REGENTRY(DIKC_FIRST,        61444)
	REGENTRY(DIKC_DELETE,       127)
	REGENTRY(DIKC_VOLUME_UP,    61)
	REGENTRY(DIKC_VOLUME_DOWN,  45)
	REGENTRY(DIKC_MUTE,         111)
	REGENTRY(DIKC_LEFT,         61440)
	REGENTRY(DIKC_RIGHT,        61441)
	REGENTRY(DIKC_ENTER,        13)
	REGENTRY(DIKC_ESCAPE,       27)
	REGENTRY(DIKC_PAGE_UP,      280)
	REGENTRY(DIKC_PAGE_DOWN,    281)
	REGENTRY(DIKC_DOWN,         61443)
	REGENTRY(DIKC_UP,           61442)
	REGENTRY(DIKC_TVVGA,        84)
	REGENTRY(DIKC_SERVICE,      102)
	REGENTRY(DIKC_MENU,         109)
	REGENTRY(DIKC_NEWS,         89)

	REGENTRY(DIKC_DJ1,          90)
	REGENTRY(DIKC_DJ2,          91)
	REGENTRY(DIKC_DJ3,          92)
	REGENTRY(DIKC_DJ4,          93)


	REGENTRY(DIKC_NET,          78)
	REGENTRY(DIKC_WINE,         74)
//	REGENTRY(DIKC_SOUNDDA,      107)
	REGENTRY(DIKC_PLUNGINS,     76)
	REGENTRY(DIKC_SWITCHLANG,   77)
	REGENTRY(DIKC_PINYIN,       120)
	REGENTRY(DIKC_SINGER,       122)

	REGENTRY(DIKC_LANG,         98)
	REGENTRY(DIKC_CODE,         67)
	REGENTRY(DIKC_CLASS,        100)
	REGENTRY(DIKC_SORT,         101)
	REGENTRY(DIKC_WORDNUM,      103)
	REGENTRY(DIKC_WBH,          121)
	REGENTRY(DIKC_QUIT,         61708)
};

static int SortKey = 0; // 1 sys_value 2: name

static void KeyMapsSort(int key)
{
	int k, compare;
	if (key == SortKey) return;
	SortKey = key;
	key_struct tmpBuf;
	for(int i=0; i< MAX_KEY_STRUCT - 1; i++){
		k = i;
		for(int j=i; j<MAX_KEY_STRUCT; j++){
			if (SortKey == 1)
				compare = KeyMaps[k].sys_value > KeyMaps[j].sys_value;
			else
				compare = strcasecmp(KeyMaps[k].name, KeyMaps[j].name) > 0;
			if(compare)
				k = j;
		}
		if( i != k){
			memcpy(&tmpBuf, &KeyMaps[i], sizeof(key_struct));
			memcpy(&KeyMaps[i], &KeyMaps[k], sizeof(key_struct));
			memcpy(&KeyMaps[k], &tmpBuf, sizeof(key_struct));
		}
	}
}

int FindMtvKey(uint16_t sys_value)
{
	if (SortKey != 1)
		KeyMapsSort(1);
	int Low = 0, High = MAX_KEY_STRUCT - 1, Mid;
	while(Low <= High){
		Mid = (Low + High) / 2;
		if(KeyMaps[Mid].sys_value < sys_value)
			Low = Mid + 1;
		else if(KeyMaps[Mid].sys_value > sys_value)
			High = Mid - 1;
		else
			return KeyMaps[Mid].mtv_value;
	}
	return sys_value;
}

void PrintKeyMaps()
{
	printf("name\t\tmtv_value\t\tsys_value\n");
	for (int i=0; i<MAX_KEY_STRUCT; i++)
		printf("%s\t\t%d\t\t%d\n", KeyMaps[i].name, KeyMaps[i].mtv_value, KeyMaps[i].sys_value);
}

key_struct *FindMtvKeyByKeyValue(uint16_t keyvalue)
{
	if (SortKey != 1)
		KeyMapsSort(1);
	int Low = 0, High = MAX_KEY_STRUCT - 1, Mid;
	while(Low <= High){
		Mid = (Low + High) / 2;
		if(KeyMaps[Mid].sys_value < keyvalue)
			Low = Mid + 1;
		else if(KeyMaps[Mid].sys_value > keyvalue)
			High = Mid - 1;
		else
			return KeyMaps+ Mid;
	}
	return NULL;
}

key_struct *FindMtvKeyByKeyName(const char *KeyName)
{
	if (SortKey != 2)
		KeyMapsSort(2);
	int Low = 0, High = MAX_KEY_STRUCT - 1, Mid, compare;
	while(Low <= High){
		Mid = (Low + High) / 2;
		compare = strcasecmp(KeyMaps[Mid].name, KeyName);
		if(compare < 0)
			Low = Mid + 1;
		else if (compare > 0)
			High = Mid - 1;
		else
			return KeyMaps + Mid;
	}
	return NULL;
}

int FindMtvValueByKeyName(const char *KeyName)
{
	key_struct *k = FindMtvKeyByKeyName(KeyName);
	if (k)
		return k->mtv_value;
	else
		return 0;
}

uint16_t MtvKeyToSysKey(int mtvkey)
{
	int i;
	for (i= 0; i< MAX_KEY_STRUCT; i++)
		if (KeyMaps[i].mtv_value == mtvkey)
			return KeyMaps[i].sys_value;
	return 0;
}
