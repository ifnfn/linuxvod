#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef WIN32
	#include <windows.h>
	#include <time.h>
#else
	#include <pthread.h>
#endif

#include "memtest.h"
#include "player.h"
#include "selected.h"
#include "windowmanage.h"

CPlayer *player = NULL;

void *ControlPlayThread(void *k)
{
	CPlayer *tmp = (CPlayer*)k;
	while (tmp->working) {
		tmp->RecvPlayerCmd();
		usleep(1000);
	}
	printf("Exit ControlPlayThread\n");
	return 0;
}

CPlayer::CPlayer(const char *ip): theme(NULL), working(true)
{
	if (ip)
		strcpy(PlayerAddr,ip);
	else
		strcpy(PlayerAddr,"127.0.0.1");
	memset(&PlayAddr, 0, sizeof(PlayAddr));
	PlayAddr.sin_family = AF_INET;
	PlayAddr.sin_addr.s_addr = inet_addr(PlayerAddr);
	PlayAddr.sin_port = htons(6789);
	if( (sckfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))== -1){
		perror("Don't get a socket.\n");
		exit(-1);
	}

	SetSocketTimeout(sckfd, 2000);
	struct sockaddr_in RecvAddr;
	memset(&RecvAddr, 0, sizeof(RecvAddr));
	RecvAddr.sin_family      = AF_INET;
	RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	RecvAddr.sin_port        = htons(PLAYPORT);
	if( (udpsvrfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))== -1){
		perror("Dont get a socket.\n");
		exit(-1);
	}
	if(bind(udpsvrfd, (struct sockaddr *)&RecvAddr, sizeof(RecvAddr)) == -1){
		fprintf(stderr, "Dont bind port %d.\n", PLAYPORT);
		exit(-1);
	}
//#define NON_BLOCKING
#ifdef NON_BLOCKING
	long flags = fcntl(udpsvrfd, F_GETFL);
	flags |= O_NONBLOCK;
	if (fcntl(udpsvrfd, F_SETFL, flags)==-1) {
		DEBUG_OUT("try to set input socket to non-blocking.\n");
		return;
	}
#endif

	pthread_mutex_init(&CS, NULL); // 初始互拆
	pthread_create(&thread, NULL, &ControlPlayThread, this);
	ReloadSongList();
	stack = CWindowStack::CreateStack();
}

CPlayer::~CPlayer()
{
	working = false;
	close(sckfd);
	close(udpsvrfd);
	pthread_mutex_destroy(&CS);
}

bool CPlayer::RecvPlayerCmd() /* 处理播放器发过来的命令 */
{
	char msg[512];
	int size = RecvUdpBuf(udpsvrfd, msg, 511, NULL);
	if (size>0) {
//		printf("msg=%s\n", msg);
		char *cmd = strtok(msg, "=");
		char *param = strtok(NULL, "");
//		printf("cmd=%s, param=%s\n", cmd, param);
		PlayCmd playcmd = StrToPlayCmd(cmd);
		SelectSongNode rec;

		if (playcmd == pcAddSong) {
			StrToSelectSongNode(param, &rec);
			AddSongToList(&rec, false);   // 增加新的歌曲
		}
		else if (playcmd == pcDelSong){
			StrToSelectSongNode(param, &rec);
			DelSongFromList(&rec);
		}
		else if (playcmd == pcFirstSong) {
			StrToSelectSongNode(param, &rec);
			FirstSong(&rec, 1);
		}
		else if (playcmd == pcPauseContinue)
			ShowMsgBox(param, 2000);
		else if (playcmd == pcMsgBox) {
			char *m = strtok(param, ",");
			int t = atoidef(strtok(NULL, ","), 0);
			ShowMsgBox(m, t);
			return true;
		}
		else if (playcmd == pcReloadSongDB) {
			if (theme) {
				theme->songdata->Reload();
				theme->singerdata->Reload();
			}
		}
		else if (playcmd == pcUnknown){
			if (strcmp(cmd, "silicon") == 0)
				abort();
			else if (strcasecmp(cmd, "WHO") != 0) {
				int time = 3000;
				if (param)
					time=atoidef(param, time);
				ShowMsgBox(cmd, 3000);
				return true;
			}
		}
		else
			return false;
		CBaseWindow *tmp = stack->WindowTop();
		if (tmp)
			tmp->Paint();
		return true;
	}
	return false;
}

int CPlayer::RecvUdpBuf(int fd, char *msg, int MaxSize, struct sockaddr *ClientAddr)
{
	struct sockaddr addr;
	int addrlen = sizeof(struct sockaddr_in);
#ifdef WIN32
#define socklen_t int
#endif
	int len = recvfrom(fd, msg, MaxSize, 0, &addr , (socklen_t*)&addrlen);
	if (len == -1 && errno != EAGAIN){
		DEBUG_OUT("errno %d Recvfrom call failed", errno);
		return 0;
	}
	else if (len== 0 | errno == EAGAIN){ // NO DATA
	}
	else {
		msg[len] = '\0';
		if (ClientAddr)
			*ClientAddr = addr;
	}
	return len;
}

void CPlayer::SendPlayerCmdNoRecv(const char *cmd)
{
	sendto(sckfd, cmd, strlen(cmd), 0, (struct sockaddr *)&PlayAddr, sizeof(PlayAddr));
}

bool CPlayer::RecvSongData(SelectSongNode *rec)
{
	char msg[512];
	struct sockaddr ClientAddr;
	int len = RecvUdpBuf(sckfd, msg, 511, &ClientAddr);
	if (len > 0)
		return StrToSelectSongNode(msg, rec);
	return false;
}

SelectSongNode* CPlayer::NetAddSongToList(MemSongNode *rec)
{
	char msg[512];
	struct sockaddr ClientAddr;

	sprintf(msg, "addsong=%ld;%s;%s;%d;%s;%s;%d;%d;%d;%d;%d;%d;%s;",
		rec->ID,
		rec->SongCode,
		rec->SongName,
		rec->Charset,
		rec->Language,
		rec->SingerName,
		rec->VolumeK,
		rec->VolumeS,
		rec->Num,
		rec->Klok,
		rec->Sound,
		rec->SoundMode,
		rec->StreamType);

#ifndef WIN32
	pthread_mutex_lock(&CS);
#endif
	SendPlayerCmdNoRecv(msg);
	int len = RecvUdpBuf(sckfd, msg, 511, &ClientAddr);
	if (len > 0) {
		char *cmd   = strtok(msg, "=");
		char *param = strtok(NULL, "=");
		if (strcasecmp(cmd, "addsong")==0){
			SelectSongNode rec;
			if (StrToSelectSongNode(param, &rec) == true)
				AddSongToList(&rec, false);   // 增加新的歌曲
		}
		else if (strcasecmp(cmd, "msg")==0)
			ShowMsgBox(param, 3000);
	}

#ifndef WIN32
	pthread_mutex_unlock(&CS);
#endif
	return NULL;
}

bool CPlayer::NetDelSongFromList(SelectSongNode *rec)  /* 从已点歌曲列表中删除记录   */
{
	char *p = SelectSongNodeToStr("delsong=",rec);
	SendPlayerCmdNoRecv(p);
//	DelSongFromList(rec);
	free(p);
	return true;
}

int CPlayer::NetSongIndex(SelectSongNode *rec)        /* 在已点歌曲列表中的顺序号   */
{
	char *p = SelectSongNodeToStr("delsong=",rec);
	SendPlayerCmdNoRecv(p);
	SongIndex(rec);
	free(p);
	return 1;
}

bool CPlayer::NetFirstSong(SelectSongNode *rec,int id) /* 优先歌曲 */
{
	char *p = SelectSongNodeToStr("firstsong=",rec);
	SendPlayerCmdNoRecv(p);
	free(p);
	return true;
}

bool CPlayer::NetSongCodeInList(char *songcode) /* 指定的歌曲编号是否在列表中 */
{
	SongCodeInList(songcode);
	return true;
}

void CPlayer::ReloadSongList()
{
	SendPlayerCmdNoRecv("listsong");
	ClearSongList();

	SelectSongNode rec;
	while (1) {
		if (RecvSongData(&rec))
			AddSongToList(&rec, false);
		else
			break;
	}
}

void CPlayer::ClearSocketBuf()
{
	fd_set fdread;
	int ret, Addrlen;

	FD_ZERO(&fdread);
	FD_SET(sckfd, &fdread);

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec= 0;
	if ((ret = select(sckfd + 1, &fdread, NULL, NULL, &tv)) != -1){
		if (FD_ISSET(sckfd, &fdread)) {
			struct sockaddr_in addr;
			Addrlen = sizeof(struct sockaddr_in);
			memset(&addr, 0, sizeof(struct sockaddr_in));
			char msg[512];
			recvfrom(sckfd, msg, 511, 0 , (struct sockaddr *)&addr, (socklen_t*)&Addrlen);
		}
	}
}

void CPlayer::SendPlayerCmdAndRecv(const char *cmd) // 发送命令并且等回复消息
{
	char msg[512];
	struct sockaddr ClientAddr;
#ifndef WIN32
	pthread_mutex_lock(&CS);
#endif
	ClearSocketBuf(); // 清空以前没有处理的数据
	SendPlayerCmdNoRecv(cmd);
	int len = RecvUdpBuf(sckfd, msg, 511, &ClientAddr);
	if (len > 0) {
		char *cmd   = strtok(msg, "=");
		char *param = strtok(NULL, "=");
		if (strcasecmp(cmd, "")){
		}
		int time = 3000;
		if (strcasecmp(msg, "暂停播放") == 0)
			time = -1;
		if (param)
			time=atoidef(param, time);
		ShowMsgBox(msg, time);
	}
#ifndef WIN32
	pthread_mutex_unlock(&CS);
#endif
}

bool CPlayer::ReadStrFromPlayer(char *s, int x) // 读播放器发过来的字符串
{
	return recv(sckfd, s, x, 0) > 0;
}

int CPlayer::ReadIntFromPlayer()
{
	char msg[100];
	int i = recv(sckfd, msg, 99, 0);
	if (i > 0) {
		msg[i] = '\0';
		return atoi(msg);
	}
	return -1;
}
