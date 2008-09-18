#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

enum tagTrack{
	trMusic,  // ����OK ,����
	trSong,   // ԭ��
	trDefault // ����
};

extern int  SongMaxNumber    ; // �������
extern int  VolumeValue      ; // ��ǰ����
extern int  MinVolume        ; // ��С����
extern int  MaxVolume        ; // �������
extern int  NextDelayTime    ; // ��ʱ
extern char Background[50]   ; // ����ͼƬ
extern char DefaultSoundMode ; // Ĭ�ϸ����ĵƹ����ģʽ
extern char HiSoundMode;     ; // Disco�����ĵƹ����ģʽ
extern char Fire119SoundMode ; // �𾯵ĵƹ����ģʽ
extern char TvType[10]       ; // ���ӻ���ʽ
extern int  TVis4x3          ; // ���ӻ������
extern bool Advertising      ; // ���׸���֮ǰ�Ƿ�岥���ͼƬ
extern int  HiSongCount      ; // HI������
extern int  DefaultCount     ; // Ĭ�ϸ�����
extern bool NoServer         ; // �޷�����
extern bool EnabledSound     ;
extern char Fire119[30]      ;
extern int  OSDCount         ;
extern char **OSDList        ;
extern char Cdrom[10]        ; // �����豸

#define VIDEOROOT "/video"     // ������Ƶ��Ŀ¼

#if 0
#define PRINTTRACK(track) \
	if (track == trDefault) \
		printf("trDefault\n"); \
	else if (track == trSong) \
		printf("trSong\n"); \
	else printf("trMusic\n");
#else
#define PRINTTRACK(track)
#endif

extern enum tagTrack SongStartAudioTrack;   // ÿ�׸�����ʼ����
extern enum tagTrack PlayerStartAudioTrack; // ��������ʼ����
extern enum tagTrack DefaultAudioTrack;     // Ĭ�ϸ���������

void ReadPlayIniConfig(char *PlayIni, char *VideoIni);
inline char *GetRandomDefaultSong(void);   // �õ������Ĭ�ϸ���
inline char *GetRandomHiSong(void);        // �õ������HI����
inline char *GetSoundMode(const char *modename);
char *GetLocalFile(char *code, char *ext); // ������ţ����ر��ش��ڵ��ļ���

#endif
