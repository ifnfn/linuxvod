#ifndef PLAYCONTROL_H
#define PLAYCONTROL_H

#include <unistd.h>
#include <string.h>
#include <semaphore.h>

#include "songdata.h"

#define _LARGEFILE64_SOURCE 1
#define ALLOW_OS_CODE
#include "rmexternalapi.h"
#include "rmexternalproperties.h"
#include "realmagichwl_uk.h"

#include "selected.h"
#include "vstp/vstp.h"

typedef unsigned char  BYTE;
typedef unsigned long  DWORD;
typedef unsigned short WORD;

typedef enum tagPLAY{
	stPlaying,         // ����״̬
	stStop,            // ֹͣ״̬
	stPause,           // ��ͣ״̬
} PlayState;

enum tagPLAYSELECT {   // ����ѡ��
	psSelected,        // ����Ĭ�ϸ���
	psDefault,         // ����Ĭ�ϸ���
	psHiSong,          // ����Hi����
	ps119              // ��
};

#define MUSICTRACK 0
#define SOUNDTRACK 1

#define MAXVIDEONUM 2
#define MAXAUDIONUM 7

#define IPLEN 16
typedef struct tagINFO{
	RMuint8 *pStart;      // �������������ڴ�
	sem_t g_sem;          // �������źŵ�

	RUA_handle rua;                      // RUA ���
	RMTcontrolInterface InterFaceCtrl;   // ��ѹ�ӿ�
	RMcontrolInterfaceInputType type;    // ����ӿ����� PUSH FILE ����
	RMTpushControl PushCtrl;             // PUSH������������
	RMTfileControl FileCtrl;             // �ļ�������������
	RMTpropertyControl PropCtrl;         // ���������Կ�����
	RMuint32 flags;

	vstp_t *PlayVstp;                    // ��ý��
	bool StartGetStream;
	RMstreamId videoStreams[MAXVIDEONUM];// ��Ƶ����
	RMstreamId audioStreams[MAXAUDIONUM];// ��Ƶ����
	RMDemuxStreamTypeEnum audioType[MAXAUDIONUM];// ��Ƶ��������
	int MaxVideoStreamNum;               // ��Ƶ����
	int MaxAudioStreamNum;               // ��Ƶ����
	enum tagPLAYSELECT PlaySelect;       // ����ѡ��

	SelectSongNode PlayingSong;          // ���ڲ��Ÿ���

	bool KeepSongList;    // ���ֲ����б�״̬,����Ǳ���״̬,��ɾ����һ��,�����ɾ���б��е�һ�׸���
	char VideoFile[50];   // ���ڲ��ŵ���Ƶ��,��: code=00000001 �����Ǳ����ļ������������LocalFile�ֶ�������
	bool LocalFile;       // true �����ļ�, false Vstp�������

	bool PlayCancel;      //
/*********************************** ״̬���� **************************************/
	PlayState PlayStatus; // ������״̬
	int  CurTrack;        // 0 ԭ�� 1 �鳪
	int  volume;          // ��ǰ����
	int  maxvolume;       // �����������
	int  minvolume;       // ��С��������
	bool Mute;            // ����״̬
	pid_t lampid;         // �ƹ���ƽ���

	bool quit;            // �˳�
	bool lock;            // ����������
	char IP[IPLEN];       // ������IP ��ַ
	char MAC[21];         // ������MAC ��ַ
	char ServerIP[IPLEN];
} INFO;

#define SETBLACKFRAME(PropCtrl) RMFSetPropertyValue(PropCtrl, RM_PROPERTY_HWLIB, "BOARDINFO_SET", "ebiCommand", ebiCommand_VideoHwBlackFrame)


void DeleteFirstSongList(void);             // ɾ���ѵ�����е�һ�׸�
bool SongListFirstPlay(INFO *pInfo);        // �����ѵ�����е�һ�׸�
int GetRealVolume     (INFO *pInfo);        // ����ʵ������
void convertToTime    (RMuint32 totalSeconds, RMuint32 *hours, RMuint32 *minutes, RMuint32 *seconds); // ʱ��ת������
void MuteSwitchPlayer (INFO *pInfo);        // �����л�
void AddVolume        (INFO *pInfo, int v); // ������ֵ��v Ϊ������������Ϊ��ֵʱ��������
void AudioSwitchPlayer(INFO *pInfo);        // ԭ�鳪�л�
void SetAudioChannel(INFO *pInfo);          // ����ԭ�鳪
int PlayAudio(INFO *pInfo, char *type, RMuint32 samplerate, \
		RMuint32 numberofchannels, RMuint32 numberofbitspersample, char *file);

bool InitInfo                (INFO *pInfo); // ��ʼ��������
void ExitPlayer              (INFO *pInfo); // �رղ�����
bool StartPlayer             (INFO *pInfo); // ����
void StopPlayer              (INFO *pInfo); // ֹͣ����
PlayState PauseContinuePlayer(INFO *pInfo); // ��ͣ����
inline void ReplayPlayer     (INFO *pInfo); // �������س�
RMTbuffer *GetPushDataBuf    (INFO *pInfo); // �õ�һ��������
void RunSoundMode            (INFO *pInfo, char *param); // ���нű�

#endif
