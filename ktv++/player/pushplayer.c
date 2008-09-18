#define ALLOW_OS_CODE
#include "rmexternalapi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>

#include "vstp/vstp.h"
#include "playcontrol.h"
#include "config.h"
#include "osnet.h"
#include "crypt/hd.h"
#ifdef OSDMENU
#include "osd.h"
#endif

static INFO Info;

static void *PlayVstpQueueThread(void *val)
{
	INFO *tmpInfo = val;
	RMTbuffer *sentbuffer;
	unsigned int readBytes;
	CHKREG
	while (!tmpInfo->quit){
		if (tmpInfo->PlayStatus == stStop){         // 如果已播放完
			if (!SongListFirstPlay(&Info)){         // 如果播放列表中第一首播不了，
				if (tmpInfo->PlaySelect  == psSelected)
					DeleteFirstSongList();          // 删除第一首歌
				usleep(10);
				continue;
			}
		}
		if (tmpInfo->PlayStatus == stStop){
			usleep(10);
			continue;
		}
		if (tmpInfo->type == RM_INPUT_PUSH) {
			while (tmpInfo->PlayStatus != stStop) {
				sentbuffer = GetPushDataBuf(tmpInfo);
				if (sentbuffer == NULL) {
					DEBUG_OUT("sentbuffer = NULL, out memory.\n");
					break;
				}
				readBytes = ReadUrl(tmpInfo->PlayVstp, (char*)sentbuffer->buffer, sentbuffer->bufferSize); // 读数据
				if (readBytes == 0) {// 如果数据已经读完,加入usleep,
					usleep(10);      // 等待播放完成
				}
				sentbuffer->dataSize = readBytes;
				sentbuffer->flags = tmpInfo->flags;
				if (readBytes == 0){
					RMFPushBuffer(tmpInfo->PushCtrl, (RMTbuffer*) NULL);
					sentbuffer->flags |= RMF_DISCARD;
				}
				RMFPushBuffer(tmpInfo->PushCtrl, sentbuffer);
			}
//			PlayerResumeMute(tmpInfo);
		}
//		else if (tmpInfo->type == RM_INPUT_FILE) {
		else {
			while (tmpInfo->PlayStatus != stStop)
				usleep(1000);
		}
		StopPlayer(tmpInfo);     // 停止播放器
		if (!tmpInfo->KeepSongList)
			DeleteFirstSongList();
		if (!tmpInfo->PlayCancel)
			usleep(NextDelayTime *1000);
	}
	return NULL;
}

static void PBToC(unsigned char *msg)
{
	int i=0;
	int len=strlen((char*)msg);
	for(;i<len;i++)
		if (msg[i] == 255){
			msg[i] = 0;
			break;
		}
}

int main(int argc, char **argv)
{
#ifndef NETPLAYER
	remove(argv[0]);
#endif	
	CHKREG
	bool havehw = InitInfo(&Info);
	if (!havehw) {                                   // 如果没有找到硬件
		DEBUG_OUT("CreatePlayer Error.\n");
//		exit(-1);
	}
	char playurl[512], videourl[512];
#ifdef NETPLAYER
	FindServerHost(Info.ServerIP, argv[0]);
	AppendIPToList(Info.ServerIP);
	ClientLogin(true);
	sprintf(playurl,  "http://%s/play.ini", Info.ServerIP);
	sprintf(videourl, "http://%s/video.ini", Info.ServerIP);
#else
	strcpy(playurl, DATAPATH"/play.ini");
	strcpy(videourl, DATAPATH"/video.ini");
#endif
	InitSongList();
//	printf("video=%s\nplay=%s\n", videourl, playurl);
	ReadPlayIniConfig(playurl, videourl);
#ifdef OSDMENU
	int i;
	for (i=0; i<OSDCount;i++)
		CreateThreadList(OSDList[i]);
#endif
	if (CreateUdpBind(PLAYERUDPPORT) <= 0) return -1;

//	RunSoundMode(&Info, "0");
	pthread_t PlayPthread = 0;
	pthread_create(&PlayPthread, NULL, &PlayVstpQueueThread, (void *)(&Info));

	char msg[512];
	char tmp[512];
	char *cmd=NULL, *param=NULL;
	PlayCmd playcmd;
	struct sockaddr addr_sin;

	CHKREG
	while(!Info.quit){
		if (RecvUdpBuf(msg, 511, &addr_sin) > 0){
			strcpy(tmp, msg);
			cmd   = strtok(msg , "?");
			param = strtok(NULL, "");
			playcmd = StrToPlayCmd(cmd);
			printf("cmd=%s,param=%s, playcmd=%d\n", cmd, param, playcmd);
			switch (playcmd){
				case pcAddSong:
				case pcDelSong:
				case pcFirstSong:
					if (Info.lock == true) continue;
				case pcListSong:
					ProcessRecv(playcmd, param, addr_sin);
					if ((playcmd == pcAddSong) && (SelectedList.count == 1) && (DefaultCount > 0) &&
					   ((Info.PlaySelect == psDefault) || (Info.PlaySelect == psSelected)))
					{// 如果有默认歌，则停掉默认歌
						Info.KeepSongList = true;
						StopPlayer(&Info);
					}
					break;
				case pcPauseContinue:
				{
					PlayerState tmpStatus = PauseContinuePlayer(&Info);
					if (tmpStatus == stPlaying)
						PlayerSendPrompt(mptContinue, &addr_sin);
					else if (tmpStatus == stPause)
						PlayerSendPrompt(mptPause, &addr_sin);
					break;
				}
				case pcAddVolume:
					AddVolume(&Info, +5);
					PlayerSendInt(Info.volume, &addr_sin);
					break;
				case pcDelVolume:
					AddVolume(&Info, -5);
					PlayerSendInt(Info.volume, &addr_sin);
					break;
				case pcAudioSwitch:
					AudioSwitchPlayer(&Info);
					if (Info.CurTrack)
						PlayerSendPrompt(mptMusic, &addr_sin);
					else
						PlayerSendPrompt(mptSong, &addr_sin);
					break;
				case pcAudioSet:
					if (strcasecmp(param, "sound") == 0){
						Info.CurTrack = SOUNDTRACK;
						SetAudioChannel(&Info);
						PlayerSendPrompt(mptSong, &addr_sin);
					}
					else if (strcasecmp(param, "music") == 0) {
						Info.CurTrack = MUSICTRACK;
						SetAudioChannel(&Info);
						PlayerSendPrompt(mptMusic, &addr_sin);
					}
					break;
				case pcSetMute:
//					printf("pcSetMute\n");
					if (MuteSwitchPlayer(&Info)) // 静音切换
						PlayerSendStr("静音?0", &addr_sin);
					else
						PlayerSendStr("", &addr_sin);
					break;
				case pcPlayNext:
					PlayerMute(&Info);
					ContinuePlayer(&Info);
					Info.PlayCancel = true;
					if (Info.PlaySelect == psHiSong)  // 如果是HI模式，切换到SlectedDefault
						Info.PlaySelect = psSelected;
					Info.PlayStatus   = stStop;
//					StopPlayer(&Info);
					break;
				case pcReplay:
					ReplayPlayer(&Info);
					break;
				case pcLock:
					Info.lock = true;
					break;
				case pcUnlock:
					Info.lock = false;
					StartPlayer(&Info);
					break;
				case pcOsdText:
				{
#ifdef OSDMENU
					char tmp[512] = "text=";
					strcat(tmp, param);
					CreateScrollTextStr(0, tmp);
#endif
					break;
				}
				case pcSetVolume:
					if (param) Info.volume = atoi(param);
					break;
				case pcSetVolumeK:
					if (param) {
						Info.CurTrack = MUSICTRACK;
						SetAudioChannel(&Info);
						PlayerSendPrompt(mptMusic, &addr_sin);

						Info.PlayingSong.VolumeK = atoi(param);
						AddVolume(&Info, 0);
					}
					break;
				case pcSetVolumeS:
					if (param) {
						Info.CurTrack = SOUNDTRACK;
						SetAudioChannel(&Info);
						PlayerSendPrompt(mptSong, &addr_sin);

						Info.PlayingSong.VolumeS = atoi(param);
						AddVolume(&Info, 0);
					}
					break;
				case pcPlayCode:
					if (param){
						char tmp[50];
						strcpy(tmp, param);
						char *code = strtok(tmp, ",");
						char *ext  = strtok(NULL, ",");
						if (ext == NULL) ext ="M1S";

						strcpy(Info.PlayingSong.SongCode, code);
						strcpy(Info.PlayingSong.StreamType, ext);
#ifndef NETPLAYER
						char *tmpfile = GetLocalFile(code, NULL);
						if (tmpfile)
							strcpy(Info.VideoFile, tmpfile);
						else
							Info.VideoFile[0]='\0';
#else
						sprintf(Info.VideoFile, "code=%s", code);
#endif
						Info.MediaType = mtFILE;
						StartPlayer(&Info);
						Info.KeepSongList = true;
					}
					break;
				case pcMACIP:
					if (strcasecmp(Info.MAC, param) == 0) PlayerSendStr(Info.IP, &addr_sin);
					break;
				case pcMaxVolume:         // 设置最大音量
					if (param) Info.maxvolume = atoi(param);
					AddVolume(&Info, 0);  // 最大音量生效
					break;
				case pc119:               // 火警
					if (Info.PlaySelect == ps119) {
//						printf("119 psSelected\n");
						Info.PlaySelect = psSelected;
					}
					else {
						Info.PlaySelect = ps119;
//						printf("119 ps119\n");
					}
					Info.KeepSongList = true;
					Info.PlayCancel   = true;
					Info.PlayStatus   = stStop;
					NoSongUnlock();
					break;
				case pcHiSong:
					if (HiSongCount == 0) break;       // 如果没有设置HI歌，不进入HI模式
					if (Info.PlaySelect == psHiSong){  // 如果是HI模式，切换到SlectedDefault
						Info.PlaySelect = psSelected;
					}
					else if ( (Info.PlaySelect == psSelected) || (Info.PlaySelect == psDefault) ) {
						Info.PlaySelect = psHiSong;
					}
					Info.KeepSongList = true;
					Info.PlayCancel   = true;
					Info.PlayStatus   = stStop;
					NoSongUnlock();
					break;
				case pcPlaySong:
					break;
				case pcRunScript:
					DEBUG_OUT("RunScript: %s\n", param);
					RunSoundMode(&Info, param);
					break;
				case pcUnknown:
					PBToC((unsigned char*)tmp);
					SendToBroadCast(tmp);
					break;
				case pchwStatus:
					if (!havehw)
						PlayerSendStr("没有找解压卡", &addr_sin);
					else
						PlayerSendStr("", &addr_sin);
					break;
				case pcMsgBox:
				case pcReloadSongDB:
					break;
			}
		}
	}
	ClientLogin(false);
	CloseUdpSocket();
	return 0;
}
