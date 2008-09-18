#include <stdio.h>
#include <string.h>

#ifdef WIN32
     #include <windows.h>
#else
     #include <semaphore.h>
     #include <sys/sem.h>
     #include <pthread.h>
#endif
#include "selected.h"
#include "strext.h"

PlaySongList SelectedList;

const char *ADDSONG   = "addsong";
const char *DELSONG   = "delsong";
const char *FIRSTSONG = "firstsong";
const char *LISTSONG  = "listsong";
const char *PLAYSONG  = "playsong";

static pthread_cond_t count_nonzero;

#define THREADLOCK 1
#ifdef  THREADLOCK
static pthread_mutex_t count_lock;
	#define LOCK() pthread_mutex_lock(&SelectedList.lock)
	#define UNLOCK() pthread_mutex_unlock(&SelectedList.lock)
#else
	#define LOCK()
	#define UNLOCK()
#endif

void InitSongList(void)
{
	memset(&SelectedList, 0, sizeof(PlaySongList));
	pthread_mutex_init(&SelectedList.lock, NULL);
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
	int count =0;
	if (msg == NULL) return false;
	if (!rec) return false;

	char *tmp = strdup(msg);
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
//		else if (!strcmp(x, "password"))  rec->Password  = atoidef(sub, '0');

		x = strtok(NULL, "&");
		count ++;
	}
	free(tmp);

	return count;
}

#define STRMCAT(str, s1, s2) {           \
	do {                             \
		char tmpbuf[100];        \
		sprintf(tmpbuf, s1, s2); \
		strcat(str, tmpbuf);     \
	} while(0);                      \
}
char *SelectSongNodeToStr(const char *cmd, SelectSongNode *rec)  /* 将歌曲结构，转换成字符串 */
{
	char msg[1024];
	memset(msg, 0, 1024);
	if (cmd)
		STRMCAT(msg, "%s?"  , cmd            );
	STRMCAT(msg, "id=%ld"       , rec->ID        );
	STRMCAT(msg, "&code=%s"     , rec->SongCode  );
	STRMCAT(msg, "&name=%s"     , rec->SongName  );
	STRMCAT(msg, "&charset=%d"  , rec->Charset   );
	STRMCAT(msg, "&language=%s" , rec->Language  );
	STRMCAT(msg, "&singer=%s"   , rec->SingerName);
	STRMCAT(msg, "&volk=%d"     , rec->VolumeK   );
	STRMCAT(msg, "&vols=%d"     , rec->VolumeS   );
	STRMCAT(msg, "&num=%d"      , rec->Num       );
	STRMCAT(msg, "&klok=%d"     , rec->Klok      );
	STRMCAT(msg, "&sound=%d"    , rec->Sound     );
	STRMCAT(msg, "&soundmode=%d", rec->SoundMode );
	STRMCAT(msg, "&type=%s"     , rec->StreamType);
//	STRMCAT(msg, "&password=%ld", rec->Password  );

	return strdup(msg);
}
#undef STRMCAT

static char *StreamType[] = {
	"DIVX",                // divx file playback (audio = MP3)
	"M1S",                 // MPEG1 system playback
	"VOB",                 // MPEG2 VOB playback
	"MP3",                 // mp3 audio file playback
	"DIVX_PCM",            // divx file playback (audio = PCM)
	"DIVX_RPCM",           // divx file playback (audio = reversed PCM)
	"M2P",                 // MPEG2 program playback
	"M2T",                 // MPEG2 transport playback
	"M4T",                 // MPEG4 transport playback
	"MP4",                 // mp4 file playback
	"MP4_MP3AUDIO",        // mp4 file playback, audio = mp3
	"MPEG4_VIDEO",         // video MPEG4 bitstream playback
	"MPEG2_VIDEO",         // video MPEG2 bitstream playback
	"M4P",                 // MPEG4 program playback
	"PCM",                 // PCM play
	NULL
};

SelectSongNode* AddSongToList(SelectSongNode *rec, bool autoinc)   /* 向已点歌曲列表中增加记录 */
{
//	DEBUG_OUT("SongCode=%s\n", rec->SongCode);
//	DEBUG_OUT("SongName=%s\n", rec->SongName);
//	DEBUG_OUT("Klok=%d\n", rec->Klok);
//	DEBUG_OUT("Sound=%d\n", rec->Sound);
//
	int i  = 0;

	if (rec == NULL) return NULL;
	while (strcmp(rec->StreamType, StreamType[i]) && StreamType[i+1] != NULL)
		i++;

	if (StreamType[i] == NULL) return NULL;
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

	return SelectedList.items + SelectedList.count - 1;
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
	DEBUG_OUT("VolumeK=%d\n", Node.VolumeK);         //
	DEBUG_OUT("VolumeS=%d\n", Node.VolumeS);         //
	DEBUG_OUT("Num=%d\n", Node.Num);                 //
	DEBUG_OUT("Klok=%c\n", Node.Klok);               //
	DEBUG_OUT("Sound=%c\n", Node.Sound);             //
	DEBUG_OUT("SoundMode=%d\n", Node.SoundMode);     //
	DEBUG_OUT("StreamType=%s\n\n", Node.StreamType); //
//	DEBUG_OUT("Password=%ld\n\n", Node.Password);    //
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
