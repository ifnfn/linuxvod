#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <ftw.h>

#include "config.h"
#include "ini.h"
#include "strext.h"
#include "avio/avio.h"

int SongMaxNumber     = 100;
int VolumeValue       = 40;
int MinVolume         = 0;
int MaxVolume         = 100;
int NextDelayTime     = 0;
char Background[50]   = DATAPATH"/blank.jpg";
char Fire119[30]      = "99999999.dat";
char DefaultSoundMode = 0; // Ĭ�ϸ����ĵƹ����ģʽ
char HiSoundMode      = 1; // Disco�����ĵƹ����ģʽ
char Fire119SoundMode = 2; // �𾯵ĵƹ����ģʽ
bool Advertising      = false;
bool EnabledSound     = true;
char TvType[10]       = "PAL";
int TVis4x3           = 1;
int OSDCount          = 0;
char **OSDList        = NULL;
char Cdrom[10]        = "";

enum tagTrack SongStartAudioTrack   = trDefault;
enum tagTrack PlayerStartAudioTrack = trMusic;
enum tagTrack DefaultAudioTrack     = trSong;

int    HiSongCount       = 0;     // HI������
static char **HiSongList = NULL;  // HI�����б�

int  DefaultCount             = 0;
static char **DefaultSongList = NULL;

static int    DirCount = 0;
static char **DirList  = NULL;

static int  SoundModeCount = 0;
static char **ModeNames    = NULL;
static char **ModeCmds     = NULL;

char *GetLocalFile(char *code, char *ext) // �����ļ�
{
	static char filename[100];
	int i, j;
	char *extname[] = {"div", "divx", "dat", "m1s", "avi", "mpg", "vob"};
	for (i = 0; i < DirCount; ++i) {
		for (j = 0; j < 5; ++j) {
			sprintf(filename, "%s/%s.%s", DirList[i], code, extname[j]);
			printf("filename=%s\n", filename);
			if (FileExists(filename)) {
				if (ext)
					strcpy(ext, extname[j]);
				return filename;
			}
		}
	}
	filename[0] = '\0';
	return NULL;
}

void ReadPlayIniConfig(char *PlayIni, char *VideoIni)
{
	av_register_all();
	offset_t size;
	char *SECTION = "setup";
	unsigned char *data = url_readbuf(PlayIni, &size);
	if (!data) return;
	struct ENTRY *ini = OpenIniFileFromMemory((unsigned char *)data, size);
	free(data);
	if (!ini) return;
	SongMaxNumber    = ReadInt(ini, SECTION, "SongMaxNumber"   , SongMaxNumber   );
	VolumeValue      = ReadInt(ini, SECTION, "VolumeValue"     , VolumeValue     );
	MinVolume        = ReadInt(ini, SECTION, "MinVolume"       , MinVolume       );
	MaxVolume        = ReadInt(ini, SECTION, "MaxVolume"       , MaxVolume       );
	NextDelayTime    = ReadInt(ini, SECTION, "NextDelayTime"   , NextDelayTime   );
	HiSoundMode      = ReadInt(ini, SECTION, "HiSoundMode"     , HiSoundMode     );
	DefaultSoundMode = ReadInt(ini, SECTION, "DefaultSoundMode", DefaultSoundMode);
	Fire119SoundMode = ReadInt(ini, SECTION, "Fire119SoundMode", Fire119SoundMode);
	TVis4x3          = ReadInt(ini, SECTION, "TVis4x3"         , TVis4x3         );
	Advertising      = ReadInt(ini, SECTION, "Advertising"     , Advertising     );
	EnabledSound     = ReadInt(ini, SECTION, "EnabledSound"    , EnabledSound    );

	strcpy(TvType, ReadString(ini, SECTION, "TV_NTSC_PAL", TvType));// ���ӻ���ʽ
	strcpy(Background, ReadString(ini, SECTION, "Background", Background));
	strcpy(Fire119, ReadString(ini, SECTION, "Fire119", Fire119));

	char *tmp = ReadString(ini, SECTION, "SongStartAudioTrack", "����");
	if (strcasecmp(tmp, "����") == 0)
		SongStartAudioTrack = trDefault;
	else if (strcasecmp(tmp, "�鳪") == 0)
		SongStartAudioTrack = trMusic;
	else
		SongStartAudioTrack = trSong;
	PRINTTRACK(SongStartAudioTrack);

	tmp = ReadString(ini, SECTION, "PlayerStartAudioTrack", "�鳪");
	if (strcasecmp(tmp, "����") == 0)
		PlayerStartAudioTrack = trDefault;
	else if (strcasecmp(tmp, "ԭ��") == 0)
		PlayerStartAudioTrack = trSong;
	else
		PlayerStartAudioTrack = trMusic;
	PRINTTRACK(PlayerStartAudioTrack);
	tmp = ReadString(ini, SECTION, "DefaultAudioTrack", "�鳪");
	if (strcasecmp(tmp, "����") == 0)
		DefaultAudioTrack = trDefault;
	else if (strcasecmp(tmp, "ԭ��") == 0)
		DefaultAudioTrack = trSong;
	else
		DefaultAudioTrack = trMusic;
	PRINTTRACK(DefaultAudioTrack);
	HiSongCount    = ReadKeyValueList(ini, "HiSongCode" , NULL, &HiSongList);
	DefaultCount   = ReadKeyValueList(ini, "DefaultSong", NULL, &DefaultSongList);
	SoundModeCount = ReadKeyValueList(ini, "SoundMode"  , &ModeNames, &ModeCmds);
	OSDCount       = ReadKeyValueList(ini, "OSD"        , NULL, &OSDList);
	CloseIniFile(ini);
	data = url_readbuf(VideoIni, &size);
	if (data) {
		ini = OpenIniFileFromMemory(data, size);
		free(data);
		DirCount = ReadKeyValueList(ini, "videopath",   NULL, &DirList);
		strcpy(Cdrom, ReadString(ini, "cdrom", "cdrom", Cdrom));
		CloseIniFile(ini);
	}
	srand(time(0));
#if 0
	printf("TvType           = %s\n", TvType);
	printf("TVis4x3          = %d\n", TVis4x3);
	printf("SongMaxNumber    = %d\n", SongMaxNumber);
	printf("VolumeValue      = %d\n", VolumeValue);
	printf("MinVolume        = %d\n", MinVolume);
	printf("MaxVolume        = %d\n", MaxVolume);
	printf("NextDelayTime    = %d\n", NextDelayTime);
	printf("HiSongCount      = %d\n", HiSongCount);
	printf("DefaultCount     = %d\n", DefaultCount);
	printf("DirCount         = %d\n", DirCount);

	printf("DefaultSoundMode = %d\n", DefaultSoundMode);
	printf("HiSoundMode      = %d\n", HiSoundMode);
	printf("Fire119SoundMode = %d\n", Fire119SoundMode);
	printf("EnabledSound     = %d\n", EnabledSound);

	int i;
	printf("\nHiCode List\n");
	for (i=0;i<HiSongCount;i++)
		printf("[%2d]=%s\n", i,HiSongList[i]);
	printf("\n\nDefault List\n");
	for (i=0;i<DefaultCount;i++)
		printf("[%2d]=%s\n", i,DefaultSongList[i]);
	printf("\n\nDirCount = %d\n", DirCount);
	for (i=0;i<DirCount;i++)
		printf("[%2d]=%s\n", i,DirList[i]);
	printf("\n\nSoundModeCount\n");
	for (i=0;i<SoundModeCount;i++)
		printf("[%2d]=%s=%s\n", i,ModeNames[i], ModeCmds[i]);
#endif
}

inline char *GetRandomDefaultSong(void) // �õ������Ĭ�ϸ���
{
	if (DefaultCount)
		return DefaultSongList[rand () % DefaultCount];
	else
		return NULL;
}

inline char *GetRandomHiSong(void) // �õ������HI����
{
	if (HiSongCount)
		return HiSongList[rand () % HiSongCount];
	else
		return NULL;
}

inline char *GetSoundMode(const char *modename)
{
	int i;
	for (i=0; i<SoundModeCount;i++){
		if (strcasecmp(modename, ModeNames[i]) == 0)
			return ModeCmds[i];
	}
	return NULL;
}

