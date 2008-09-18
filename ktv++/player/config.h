#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

enum tagTrack{
	trMusic,  // 卡啦OK ,音乐
	trSong,   // 原唱
	trDefault // 不变
};

extern int  SongMaxNumber    ; // 最大音量
extern int  VolumeValue      ; // 当前音量
extern int  MinVolume        ; // 最小音量
extern int  MaxVolume        ; // 最大音量
extern int  NextDelayTime    ; // 延时
extern char Background[50]   ; // 背景图片
extern char DefaultSoundMode ; // 默认歌曲的灯光控制模式
extern char HiSoundMode;     ; // Disco歌曲的灯光控制模式
extern char Fire119SoundMode ; // 火警的灯光控制模式
extern char TvType[10]       ; // 电视机制式
extern int  TVis4x3          ; // 电视机长宽比
extern bool Advertising      ; // 两首歌曲之前是否插播广告图片
extern int  HiSongCount      ; // HI歌数量
extern int  DefaultCount     ; // 默认歌数量
extern bool NoServer         ; // 无服务器
extern bool EnabledSound     ;
extern char Fire119[30]      ;
extern int  OSDCount         ;
extern char **OSDList        ;
extern char Cdrom[10]        ; // 光驱设备

#define VIDEOROOT "/video"     // 本地视频根目录

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

extern enum tagTrack SongStartAudioTrack;   // 每首歌曲开始音轨
extern enum tagTrack PlayerStartAudioTrack; // 播放器开始音轨
extern enum tagTrack DefaultAudioTrack;     // 默认歌曲的音轨

void ReadPlayIniConfig(char *PlayIni, char *VideoIni);
inline char *GetRandomDefaultSong(void);   // 得到随机的默认歌曲
inline char *GetRandomHiSong(void);        // 得到随机的HI歌曲
inline char *GetSoundMode(const char *modename);
char *GetLocalFile(char *code, char *ext); // 给定编号，返回本地存在的文件名

#endif
