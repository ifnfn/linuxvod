#ifndef PLAYCONTROL_H
#define PLAYCONTROL_H

#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>

#define _LARGEFILE64_SOURCE 1
#define ALLOW_OS_CODE
#include "rmexternalapi.h"
#include "rmexternalproperties.h"
#include "realmagichwl_uk.h"

#include "selected.h"
#include "vstp/vstp.h"
#include "crypt/hd.h"

typedef unsigned char  BYTE;
typedef unsigned long  DWORD;
typedef unsigned short WORD;

typedef enum tagPLAY{
	stPlaying,         // 播放状态
	stStoping,         // 停止状态
	stStoped, 
	stPause,           // 暂停状态
} PlayerState;

enum tagPLAYSELECT {   // 播放选择
	psSelected,        // 播放默认歌曲
	psDefault,         // 播放默认歌曲
	psHiSong,          // 播放Hi歌曲
	ps119              // 火警
};

enum tagMediaType {
	mtFILE,
	mtPUSH,
	mtDISC
};

//#define DVDPLAYER

#ifdef NETPLAYER
#	define CHKREG
#else
#	define CHKREG {if (CheckKtvRegCode() == false) exit(0);}
//#	define CHKREG 
#endif

#define MUSICTRACK 0
#define SOUNDTRACK 1
#define MUTE       2

#define MAXVIDEONUM 2
#define MAXAUDIONUM 7

#define IPLEN 16
#define RM_MAX_STRING 1024

typedef struct tagINFO{
	RMTDisc*                     Disc;
	RUA_handle                   rua;              // RUA 句柄
	RMTcontrolInterface          InterFaceCtrl;    // 解压接口
	RMcontrolInterfaceInputType  type;             // 输入接口类型 PUSH FILE 两种
#ifdef NETPLAYER 
	RMTpushControl               PushCtrl;         // PUSH播放器控制器
#endif
	RMTfileControl               FileCtrl;         // 文件播放器控制器
#ifdef DVDPLAYER
	RMTdvdControl                DvdCtrl;          // DVD 播放器控制器
	RMTvcdControl                VcdCtrl;          // VCD 播放器控制器
	RMTcddaControl               CddaCtrl;         // CDDA播放器控制器
#endif
	RMTpropertyControl           PropCtrl;         // 播放器属性控制器

	RMuint8*                     pStart;           // 播放器缓冲区内存
#ifdef NETPLAYER
	sem_t                        push_sem;         // 缓冲区信号灯
	pthread_t                    PushThread;       // PUSH 数据线程
	vstp_t*                      PlayVstp;         // 流媒体
	RMuint32                     push_flags;
#endif
	bool                         StartGetStream;
	RMstreamId                   videoStreams[MAXVIDEONUM];  // 视频流轨
	RMstreamId                   audioStreams[MAXAUDIONUM];  // 音频流轨
	int                          MaxVideoStreamNum;          // 视频轨数
	int                          MaxAudioStreamNum;          // 音频轨数
	enum tagPLAYSELECT           PlaySelect;                 // 播放选择

	SelectSongNode               PlayingSong;                // 正在播放歌曲
	enum tagMediaType            MediaType;                  // 本地文件

	bool KeepSongList;              // 保持播放列表状态,如果是保存状态,不删除第一首,否则会删除列表中第一首歌曲
	char VideoURL[RM_MAX_STRING];   // 正在播放的视频串,如: code=00000001 或者是本地文件名，由下面的LocalFile字段来决定

/*********************************** 状态控制 **************************************/
	pthread_mutex_t              play_lock;         // 播放同步锁 
	pthread_cond_t               play_singal;       // 同步信号
	PlayerState                  PlayStatus;        // 播放器状态
	int                          CurTrack;          // 0 原唱 1 伴唱
	int                          volume;            // 当前音量
	int                          maxvolume;         // 最大允许音量
	int                          minvolume;         // 最小允许音量
	bool                         Mute;              // 静音状态
	pid_t                        lampid;            // 灯光控制进程

	bool                         lock;              // 播放器锁定
	char                         IP[IPLEN];         // 本机的IP 地址
	char                         MAC[21];           // 本机的MAC 地址
	char                         ServerIP[IPLEN];   // 服务器IP 地址
	struct sockaddr              addr_sin;
	int                          HaveHW;

	bool                         quit;              // 退出
} INFO;

#define SETBLACKFRAME(PropCtrl) RMFSetPropertyValue(PropCtrl, RM_PROPERTY_HWLIB, "BOARDINFO_SET", "ebiCommand", ebiCommand_VideoHwBlackFrame)

void ConvertToTime(RMuint32 totalSeconds, RMuint32 *hours, RMuint32 *minutes, RMuint32 *seconds); // 时间转换函数
bool GetDiscDetect(const char* device, char *url);

void DeleteFirstSongList(void);             // 删除已点歌曲中第一首歌
bool SongListFirstPlay (INFO *pInfo);        // 播放已点歌曲中第一首歌
int  GetRealVolume     (INFO *pInfo);        // 计算实际音量
bool MuteSwitchPlayer  (INFO *pInfo);        // 静音切换
void AddVolume         (INFO *pInfo, int v); // 音量增值，v 为正增加音量，为负值时减少音量
void AudioSwitchPlayer (INFO *pInfo);        // 原伴唱切换
void SetAudioChannel   (INFO *pInfo);        // 设置原伴唱
bool InitInfo          (INFO *pInfo);        // 初始化播放器
void ExitPlayer        (INFO *pInfo);        // 关闭播放器
bool StartPlayer       (INFO *pInfo);        // 播放
void StopPlayer        (INFO *pInfo);        // 停止播唱
void ReplayPlayer      (INFO *pInfo);        // 播放器重唱
void PlayerMute        (INFO *pInfo);
void PlayerResumeMute  (INFO *pInfo);
void PausePlayer       (INFO *pInfo);
void ContinuePlayer    (INFO *pInfo);
void VodSyncAudioStream(INFO *pInfo);

RMTbuffer *GetPushDataBuf      (INFO *pInfo);                     // 得到一个缓冲区
void FreePushDataBuf           (INFO *pInfo, RMTbuffer *buffer);  // 释放缓冲区
PlayerState PauseContinuePlayer(INFO *pInfo);                     // 暂停继续
void RunSoundMode              (INFO *pInfo, const char *param);  // 运行脚本

int PlayAudio(INFO *pInfo, char *type,          \
		RMuint32 samplerate,            \
		RMuint32 numberofchannels,      \
		RMuint32 numberofbitspersample, const char *file);

#endif
