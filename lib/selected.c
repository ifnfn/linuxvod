#include <stdio.h>
#ifdef WIN32
     #include <windows.h>
#else
     #include <semaphore.h>
     #include <sys/sem.h>
     #include <pthread.h>
#endif
#include "selected.h"
#include "strext.h"

PlaySongList SelectedList = {0,0,0,NULL};

struct tagCMD {
	const char *cmdstr;
	PlayCmd cmd;
};

const char *ADDSONG   = "addsong";
const char *DELSONG   = "delsong";
const char *FIRSTSONG = "firstsong";
const char *LISTSONG  = "listsong";
const char *PLAYSONG  = "playsong";

#define CMDSIZE pcUnknown
static struct tagCMD CMD[CMDSIZE] = {
	{"addsong"      , pcAddSong       },
	{"delsong"      , pcDelSong       },
	{"firstsong"    , pcFirstSong     },
	{"listsong"     , pcListSong      },
	{"playsong"     , pcPlaySong      },
	{"setvolume"    , pcSetVolume     },
	{"audioswitch"  , pcAudioSwitch   },
	{"audio"	, pcAudioSet	  },
	{"setmute"      , pcSetMute       },
	{"playcode"     , pcPlayCode      },
	{"playnext"     , pcPlayNext      },
	{"PauseContinue", pcPauseContinue },
	{"addvolume"    , pcAddVolume     },
	{"delvolume"    , pcDelVolume     },
	{"Lock"         , pcLock          },
	{"unlock"       , pcUnlock        },
	{"replay"       , pcReplay        },
	{"osdtext"      , pcOsdText       },
	{"mac"          , pcMACIP         },
	{"MaxVolume"    , pcMaxVolume     },
	{"119"          , pc119           },
	{"runscript"    , pcRunScript     },
	{"HiSong"       , pcHiSong        },
	{"MsgBox"	, pcMsgBox	  },
	{"hwstatus"     , pchwStatus      },
	{"ReSongDB"	, pcReloadSongDB  },
};

static pthread_cond_t count_nonzero;
static pthread_mutex_t mutex;

#define THREADLOCK 1
#ifdef  THREADLOCK
static pthread_mutex_t count_lock;
	#define LOCK() pthread_mutex_lock(&mutex)
	#define UNLOCK() pthread_mutex_unlock(&mutex)
#else
	#define LOCK()
	#define UNLOCK()
#endif

void NoSongLock(void)
{
	pthread_cond_wait( &count_nonzero, &count_lock);
}

void NoSongUnlock(void)
{
	pthread_cond_signal(&count_nonzero);
}

PlayCmd StrToPlayCmd(char *cmd)
{
	int i;
	for (i=0;i<CMDSIZE;i++){
		if (!strncasecmp(CMD[i].cmdstr, cmd, strlen(CMD[i].cmdstr)))
			return CMD[i].cmd;
	}
	return pcUnknown;
}

void InitSongList(void)
{
	SelectedList.count = 0;
	pthread_mutex_init(&mutex, NULL);
#ifdef  THREADLOCK
	pthread_mutex_init(&count_lock, NULL);
#endif
	pthread_cond_init(&count_nonzero, NULL);
}

void ClearSongList(void)
{
	LOCK();
	SelectedList.count = 0;
	UNLOCK();
}

bool StrToSelectSongNode(const char *msg, SelectSongNode *rec)
{
	if (msg == NULL) return false;
	char *tmp = strdup(msg);
	if (!rec) return false;
	memset(rec, 0, sizeof(SelectSongNode));
	char *x = strtok(tmp, "&");
	while (x) {
		char *sub = strstr(x, "=");
		if (!sub)  {
			x = strtok(NULL, "&");
			continue;
		}
		*sub = '\0';
		sub++;
		if (     !strcmp(x, "id"))        rec->ID = atoidef(sub, 0);
		else if (!strcmp(x, "code"))      strcpy(rec->SongCode, sub);
		else if (!strcmp(x, "name"))      strcpy(rec->SongName, sub);
		else if (!strcmp(x, "charset"))   rec->Charset   = atoidef(sub, 0);
		else if (!strcmp(x, "language"))  strcpy(rec->Language, sub);
		else if (!strcmp(x, "singer"))    strcpy(rec->SingerName, sub);
		else if (!strcmp(x, "volk"))      rec->VolumeK   = atoidef(sub, '0');
		else if (!strcmp(x, "vols"))      rec->VolumeS   = atoidef(sub, '0');
		else if (!strcmp(x, "num"))       rec->Num       = atoidef(sub, '0');
		else if (!strcmp(x, "klok"))      rec->Klok      = atoidef(sub, '0');
		else if (!strcmp(x, "sound"))     rec->Sound     = atoidef(sub, '0');
		else if (!strcmp(x, "soundmode")) rec->SoundMode = atoidef(sub, '0');
		else if (!strcmp(x, "type"))      strcpy(rec->StreamType, sub);
		x = strtok(NULL, "&");
	}
	free(tmp);
	return true;
}

#define STRMCAT(str, s1, s2) { \
	if (s2) { \
	char tmpbuf[100]; \
	sprintf(tmpbuf, s1, s2); \
	strcat(str, tmpbuf); \
	} \
}

char *SelectSongNodeToStr(const char *cmd, SelectSongNode *rec)  /* 将歌曲结构，转换成字符串 */
{
	char msg[1024];
	if (cmd)
		sprintf(msg, "%s?id=%ld", cmd, rec->ID);
	else
		sprintf(msg, "id=%ld", rec->ID);
	STRMCAT(msg, "&code=%s", rec->SongCode);
	STRMCAT(msg, "&name=%s", rec->SongName);
	STRMCAT(msg, "&charset=%d", rec->Charset);
	STRMCAT(msg, "&language=%s", rec->Language);
	STRMCAT(msg, "&singer=%s", rec->SingerName);
	STRMCAT(msg, "&volk=%d", rec->VolumeK);
	STRMCAT(msg, "&vols=%d", rec->VolumeS);
	STRMCAT(msg, "&num=%d", rec->Num);
	STRMCAT(msg, "&klok=%d", rec->Klok);
	STRMCAT(msg, "&sound=%d", rec->Sound);
	STRMCAT(msg, "&soundmode=%d", rec->SoundMode);
	STRMCAT(msg, "&type=%s", rec->StreamType);
	char *p = (char*)malloc(strlen(msg) + 1);
	strcpy(p, msg);
	return p;
}

SelectSongNode* AddSongToList(SelectSongNode *rec, bool autoinc)   /* 向已点歌曲列表中增加记录 */
{
//	DEBUG_OUT("SongCode=%s\n", rec->SongCode);
//	DEBUG_OUT("SongName=%s\n", rec->SongName);
//	DEBUG_OUT("Klok=%d\n", rec->Klok);
//	DEBUG_OUT("Sound=%d\n", rec->Sound);
	LOCK();
	if (autoinc)
		rec->ID = SelectedList.MaxID++;

	if (SelectedList.count >= SelectedList.MaxCount) { // 保存内存只增加，不减少，这样可以减少内存操作
		SelectedList.items = (SelectSongNode *)realloc(SelectedList.items, sizeof(SelectSongNode) * (SelectedList.count+1));
		SelectedList.MaxCount++;
	}
	memcpy(SelectedList.items + SelectedList.count, rec, sizeof(SelectSongNode));
	SelectedList.count++;
	UNLOCK();
	NoSongUnlock();
	return SelectedList.items + SelectedList.count - 1;
}

SelectSongNode* AddSongToListFrist(SelectSongNode *rec)
{
	SelectSongNode* i = AddSongToList(rec, true);
	FirstSong(rec, 0);
	return i;
}

bool DelSongFromList(SelectSongNode *rec) /* 从已点歌曲列表中删除记录 */
{
	int i = SongIndex(rec);
	if (i>= 0){
		LOCK();
		SelectedList.count--;
		memcpy(SelectedList.items+i, SelectedList.items+i+1, (SelectedList.count-i)*sizeof(SelectSongNode));
		UNLOCK();
	}
	return i >= 0;
}

bool DelSongIndex(int index)
{
	if (index>= 0){
		LOCK();
		SelectedList.count--;
		memcpy(SelectedList.items + index, SelectedList.items + index + 1,\
			(SelectedList.count - index) * sizeof(SelectSongNode));
		UNLOCK();
	}
	return index >= 0;
}

int SongIndex(SelectSongNode *rec)
{
	int i;
	LOCK();
	for (i=0;i<SelectedList.count;i++) {
		if (SelectedList.items[i].ID == rec->ID) {
			UNLOCK();
			return i;
		}
	}
	UNLOCK();
	return -1;
}

bool FirstSong(SelectSongNode *rec,int id)/* 优先歌曲 */
{
	int i = SongIndex(rec);
	if ( i >= id ) { // 不是第id首歌曲
		LOCK();
		SelectSongNode tmp = SelectedList.items[i];
		int num= i - id;
		if (num) {
			memmove(SelectedList.items+id+1,SelectedList.items+id,sizeof(SelectSongNode)*num);
			SelectedList.items[id] = tmp;
			UNLOCK();
			return true;
		}
		UNLOCK();
		return false;
	}
	return false;
}

bool SongCodeInList(const char *songcode)
{
	int i;
	LOCK();
	for (i=0; i<SelectedList.count; i++) {
		if ( !strcasecmp(SelectedList.items[i].SongCode, songcode) ) {
			UNLOCK();
			return true;
		}
	}
	UNLOCK();
	return false;
}

#ifdef DEBUG
void PrintSelectSongNode(SelectSongNode Node)
{
	DEBUG_OUT("ID=%ld\n", Node.ID);
	DEBUG_OUT("SongCode=%s\n", Node.SongCode);       // 歌曲编号
	DEBUG_OUT("SongName=%s\n", Node.SongName);       // 歌曲名称
	DEBUG_OUT("Charset=%d\n", Node.Charset);         // 字符集
	DEBUG_OUT("Language=%s\n", Node.Language);       // 语言
	DEBUG_OUT("SingerName=%s\n", Node.SingerName);   // 歌星姓名
	DEBUG_OUT("VolumeK=%d\n", Node.VolumeK);         // 歌星姓名
	DEBUG_OUT("VolumeS=%d\n", Node.VolumeS);         // 歌星姓名
	DEBUG_OUT("Num=%d\n", Node.Num);                 // 歌星姓名
	DEBUG_OUT("Klok=%c\n", Node.Klok);               // 歌星姓名
	DEBUG_OUT("Sound=%c\n", Node.Sound);             // 歌星姓名
	DEBUG_OUT("SoundMode=%d\n", Node.SoundMode);     // 歌星姓名
	DEBUG_OUT("StreamType=%s\n\n", Node.StreamType); // 歌星姓名
}

void PrintSelectSong(void)
{
	int i;
	LOCK();
	for (i=0;i<SelectedList.count;i++)
		DEBUG_OUT("%s = %s\n", SelectedList.items[i].SongCode,SelectedList.items[i].SongName);
	UNLOCK();
}
#endif
