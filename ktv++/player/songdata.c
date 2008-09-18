#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/timeb.h>
#include <pthread.h>

#include "songdata.h"
#include "config.h"
#include "osnet.h"
#include "selected.h"

struct sockaddr_in clientaddr_list[10];  // 连接到播放的工作站列表
static int ClientNum = 0;                // 连接到播放的工作站列表数量
static int udp_msg_sockfd = -1;
static char *Prompt[] = {"暂停播放?0", "继续播放", "原唱模式", "卡拉OK模式", "静音?0", ""};

typedef int (*command_process)(INFO *Info, const char *param, int sockfd);

typedef struct command_t {
	const char *command;
	command_process process;
}command_t ;

extern command_t command_table[];
static int SendHttpHead(int sockfd, const char *text);


#define CREATE_UDP do { \
		if (udp_msg_sockfd < 0) \
			udp_msg_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); \
	} while(0)


int AppendClientToList(struct sockaddr clientaddr)
{
	int i;
	struct sockaddr_in *tmp = (struct sockaddr_in *)(&clientaddr);
	for (i=0; i<ClientNum; i++){
		if (!memcmp(&clientaddr_list[i].sin_addr, &tmp->sin_addr, sizeof(tmp->sin_addr)))
			return 0;
	}
	memcpy(clientaddr_list + ClientNum, &clientaddr, sizeof(struct sockaddr_in));
	clientaddr_list[ClientNum].sin_port = htons(PLAYPORT);
	ClientNum++;
//	printf("Add (%d) %s.\n", ClientNum, inet_ntoa(tmp->sin_addr));
	return 1;
}

int AppendIPToList(const char *ip)
{
	struct sockaddr_in addr;
	bzero(&addr, sizeof(struct sockaddr_in));
	addr.sin_addr.s_addr = inet_addr(ip);
	struct sockaddr *x = (struct sockaddr*)&addr;
	return AppendClientToList(*x);
}

inline void CloseUdpSocket(void)
{
	if (udp_msg_sockfd > 0)
		close(udp_msg_sockfd);
}

void BroadcastSongRec(const char *cmd, SelectSongNode *rec)
{
	char *recstr = SelectSongNodeToStr(cmd, rec);
	SendToBroadCast(recstr);
	free(recstr);
}

inline void PlayerSendPrompt(int id)
{
	if (id >= mptPause && id <= mptClean)
		SendToBroadCast(Prompt[id]);
}

void SendToBroadCast(char *msg) // 发送广播数据
{
	int i;

	CREATE_UDP;
	for(i=0;i<ClientNum;i++)
		sendto(udp_msg_sockfd, msg, strlen(msg), 0, \
			(struct sockaddr *)(clientaddr_list + i), sizeof(struct sockaddr_in));
}

static int SendHttpHead(int sockfd, const char *text)
{
	char   tempstring[1024];

	if (send(sockfd, "HTTP/1.1 200 OK\n", 16, 0)==-1)
		return -1;
	if (send(sockfd, "Server: LinuxPlayer\n", 20, 0)==-1)	
		return -1;
	
	if (text) {
		sprintf(tempstring, "Content-Length: %d\n", strlen(text));
		if (send(sockfd, tempstring, strlen(tempstring), 0)==-1)	
			return -1;
	}
	strcpy(tempstring, "Content-Type: text/html\n");
	if (send(sockfd, tempstring, strlen(tempstring), 0)==-1)
		return -1;

	if (send(sockfd, "\n", 1, 0)==-1)
		return -1;

	if (text != NULL) {
		if (send(sockfd, text, strlen(text), 0) == -1)
			return -1;
	}

	return 0;
}

static int command_AddSong(INFO *pInfo, const char *param, int sockfd)
{
	if (pInfo->lock == true) 
		return 0;

	SelectSongNode rec;
	StrToSelectSongNode(param, &rec);
	if (SelectedList.count < SongMaxNumber) {
		SelectSongNode *newrec = AddSongToList(&rec, true); // 增加新的歌曲
		if (newrec) 
			BroadcastSongRec(ADDSONG, newrec);
		if (SelectedList.count == 1 && DefaultCount > 0 &&
			((pInfo->PlaySelect == psDefault) || (pInfo->PlaySelect == psSelected)))
		{// 如果有默认歌，则停掉默认歌
			pInfo->KeepSongList = true;
			StopPlayer(pInfo);
		}
	}
	else{
		char msg[100];
		sprintf(msg, "msg?最多允许点歌%d首", SongMaxNumber);
		SendHttpHead(sockfd, msg);
	}       

	return 0;
}

static int command_DelSong(INFO *pInfo, const char *param, int sockfd)
{
	if (pInfo->lock == true) 
		return 0;

	SelectSongNode rec;
	StrToSelectSongNode(param, &rec);
	DelSongFromList(&rec);
	BroadcastSongRec(DELSONG, &rec);

	return 0;
}

static int command_FirstSong(INFO *pInfo, const char *param, int sockfd)
{
	if (pInfo->lock == true) 
		return 0;

	SelectSongNode rec;
	StrToSelectSongNode(param, &rec);
	FirstSong(&rec, 1);
	BroadcastSongRec(FIRSTSONG, &rec);

	return 0;
}

static int command_ListSong(INFO *pInfo, const char *param, int sockfd)
{
	int i;
	char *recstr;

	SendHttpHead(sockfd, NULL);
	for(i=0; i<SelectedList.count; i++) {
		recstr = SelectSongNodeToStr(NULL, SelectedList.items+i);
		send(sockfd, recstr, strlen(recstr), 0);
		free(recstr);
		send(sockfd, "\n", 1, 0);
	}
	send(sockfd, "\xFF", 1, 0);

	return 1;
}

static int command_PauseContinue(INFO *pInfo, const char *param, int sockfd)
{
	PlayerState tmpStatus = PauseContinuePlayer(pInfo);
	if (tmpStatus == stPlaying)
		PlayerSendPrompt(mptContinue);
	else if (tmpStatus == stPause)
		PlayerSendPrompt(mptPause);

	return 0;
}

static int command_AddVolume(INFO *pInfo, const char *param, int sockfd)
{
	char data[20];

	AddVolume(pInfo, +5);
	sprintf(data, "%d", pInfo->volume);
	SendHttpHead(sockfd, data);

	return 1;
}

static int command_DelVolume(INFO *pInfo, const char *param, int sockfd)
{
	char data[20];

	AddVolume(pInfo, -5);
	sprintf(data, "%d", pInfo->volume);
	SendHttpHead(sockfd, data);

	return 1;
}

static int command_AudioSwitch(INFO *pInfo, const char *param, int sockfd)
{
	AudioSwitchPlayer(pInfo);
	if (pInfo->CurTrack)
		PlayerSendPrompt(mptMusic);
	else
		PlayerSendPrompt(mptSong);

	return 0;
}

static int command_AudioSet(INFO *pInfo, const char *param, int sockfd)
{
	if (strcasecmp(param, "sound") == 0){
		pInfo->CurTrack = SOUNDTRACK;
		SetAudioChannel(pInfo);
		PlayerSendPrompt(mptSong);
	}
	else if (strcasecmp(param, "music") == 0) {
		pInfo->CurTrack = MUSICTRACK;
		SetAudioChannel(pInfo);
		PlayerSendPrompt(mptMusic);
	}

	return 0;
}

static int command_SetMute(INFO *pInfo, const char *param, int sockfd)
{
	if (MuteSwitchPlayer(pInfo)) // 静音切换
		SendHttpHead(sockfd, "静音");
	else
		SendHttpHead(sockfd, "");

	return 0;
}

static int command_PlayNext(INFO *pInfo, const char *param, int sockfd)
{
//	PlayerMute(pInfo);
//	ContinuePlayer(pInfo);
//	PausePlayer(pInfo);
	pInfo->PlayCancel = true;
	if (pInfo->PlaySelect == psHiSong)  // 如果是HI模式，切换到SlectedDefault
		pInfo->PlaySelect = psSelected;
	pInfo->PlayStatus   = stStop;
//	StopPlayer(pInfo);

	return 0;
}

static int command_Replay(INFO *pInfo, const char *param, int sockfd)
{
	ReplayPlayer(pInfo);

	return 0;
}

static int command_Lock(INFO *pInfo, const char *param, int sockfd)
{
	pInfo->lock = true;

	return 0;
}

static int command_Unlock(INFO *pInfo, const char *param, int sockfd)
{
	pInfo->lock = false;
	StartPlayer(pInfo);

	return 0;
}

static int command_OsdText(INFO *pInfo, const char *param, int sockfd)
{
#ifdef OSDMENU
	char tmp[512] = "text=";
	strcat(tmp, param);
	CreateScrollTextStr(0, tmp);
#endif

	return 0;
}

static int command_SetVolume(INFO *pInfo, const char *param, int sockfd)
{
	if (param) 
		pInfo->volume = atoi(param);

	return 0;
}

static int command_SetVolumeK(INFO *pInfo, const char *param, int sockfd)
{
	if (param) {
		pInfo->CurTrack = MUSICTRACK;
		SetAudioChannel(pInfo);
		PlayerSendPrompt(mptMusic);

		pInfo->PlayingSong.VolumeK = atoi(param);
		AddVolume(pInfo, 0);
	}

	return 0;
}

static int command_SetVolumeS(INFO *pInfo, const char *param, int sockfd)
{
	if (param) {
		pInfo->CurTrack = SOUNDTRACK;
		SetAudioChannel(pInfo);
		PlayerSendPrompt(mptSong);

		pInfo->PlayingSong.VolumeS = atoi(param);
		AddVolume(pInfo, 0);
	}

	return 0;
}

static int command_PlayCode(INFO *pInfo, const char *param, int sockfd)
{
	if (param){
		char tmp[50];
		strcpy(tmp, param);
		char *code = strtok(tmp, ",");
		char *ext  = strtok(NULL, ",");
		if (ext == NULL) ext ="M1S";

		strcpy(pInfo->PlayingSong.SongCode, code);
		strcpy(pInfo->PlayingSong.StreamType, ext);
#ifndef NETPLAYER
		char *tmpfile = GetLocalFile(code, NULL);
		if (tmpfile)
			strcpy(pInfo->VideoFile, tmpfile);
		else
			pInfo->VideoFile[0]='\0';
#else
		sprintf(pInfo->VideoFile, "code=%s", code);
#endif
		pInfo->MediaType = mtFILE;
		StartPlayer(pInfo);
		pInfo->KeepSongList = true;
	}

	return 0;
}

static int command_MaxVolume(INFO *pInfo, const char *param, int sockfd)
{
	if (param) 
		pInfo->maxvolume = atoi(param);
	AddVolume(pInfo, 0);  // 最大音量生效

	return 0;
}

static int command_RunScript(INFO *pInfo, const char *param, int sockfd)
{
	DEBUG_OUT("RunScript: %s\n", param);
	RunSoundMode(pInfo, param);

	return 0;
}

static int command_Unknown(INFO *pInfo, const char *param, int sockfd)
{
	SendToBroadCast((char *)param);

	return 0;
}

static int command_hwStatus(INFO *pInfo, const char *param, int sockfd)
{
	if (!pInfo->HaveHW)
		SendHttpHead(sockfd, "没有找解压卡");
	else
		SendHttpHead(sockfd, "");

	return 0;
}


static int command_119(INFO *pInfo, const char *param, int sockfd)
{
	if (pInfo->PlaySelect == ps119) {
//		printf("119 psSelected\n");
		pInfo->PlaySelect = psSelected;
	}
	else {
		pInfo->PlaySelect = ps119;
//		printf("119 ps119\n");
	}
	pInfo->KeepSongList = true;
	pInfo->PlayCancel   = true;
	pInfo->PlayStatus   = stStop;
	NoSongUnlock();

	return 0;
}

static int command_HiSong(INFO *pInfo, const char *param, int sockfd)
{
	if (HiSongCount == 0) 
		return 0;                 // 如果没有设置HI歌，不进入HI模式
	if (pInfo->PlaySelect == psHiSong)  // 如果是HI模式，切换到SlectedDefault
		pInfo->PlaySelect = psSelected;
	else if ( (pInfo->PlaySelect == psSelected) || (pInfo->PlaySelect == psDefault) )
		pInfo->PlaySelect = psHiSong;
	pInfo->KeepSongList = true;
	pInfo->PlayCancel   = true;
	pInfo->PlayStatus   = stStop;
	NoSongUnlock();

	return 0;
}

static int command_MACIP(INFO *pInfo, const char *param, int sockfd)
{
	if (strcasecmp(pInfo->MAC, param) == 0) 
		SendHttpHead(sockfd, pInfo->IP);

	return 0;
}

static int command_PlaySong(INFO *pInfo, const char *param, int sockfd)
{
	return 0;
}

static int command_MsgBox(INFO *pInfo, const char *param, int sockfd)
{
	return 0;
}

static int command_ReloadSongDB(INFO *pInfo, const char *param, int sockfd)
{
	return 0;
}

command_t command_table[] = {
	{"addsong"      , command_AddSong       },
	{"delsong"      , command_DelSong       },
	{"firstsong"    , command_FirstSong     },
	{"listsong"     , command_ListSong      },
	{"playsong"     , command_PlaySong      },
	{"setvolume"    , command_SetVolume     },
	{"audioswitch"  , command_AudioSwitch   },
	{"audio"        , command_AudioSet      },
	{"setmute"      , command_SetMute       },
	{"playcode"     , command_PlayCode      },
	{"playnext"     , command_PlayNext      },
	{"PauseContinue", command_PauseContinue },
	{"addvolume"    , command_AddVolume     },
	{"delvolume"    , command_DelVolume     },
	{"Lock"         , command_Lock          },
	{"unlock"       , command_Unlock        },
	{"replay"       , command_Replay        },
	{"osdtext"      , command_OsdText       },
	{"mac"          , command_MACIP         },
	{"MaxVolume"    , command_MaxVolume     },
	{"119"          , command_119           },
	{"runscript"    , command_RunScript     },
	{"HiSong"       , command_HiSong        },
	{"MsgBox"       , command_MsgBox        },
	{"hwstatus"     , command_hwStatus      }, 
	{"ReSongDB"     , command_ReloadSongDB  }, 
	{"SetVolumeK"   , command_SetVolumeK    },
	{"SetVolumeS"   , command_SetVolumeS    },
	{NULL           , command_Unknown       },
};

int process(INFO *pInfo, const char *cmd, const char *param, int sockfd)
{
	int id = 0;
	while (command_table[id].command) {
		if (strcmp(command_table[id].command, cmd) == 0) {
			if (command_table[id].process(pInfo, param, sockfd) == 0) {
				SendHttpHead(sockfd, "OK\n");
			}
			return 0;
		}
		id++;
	}

	return -1;
}
