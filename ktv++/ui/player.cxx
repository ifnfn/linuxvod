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
//		usleep(1000);
	}
	printf("Exit ControlPlayThread\n");
	return 0;
}

CPlayer::CPlayer(const char *ip): theme(NULL), working(true)
{
	if (ip)
		strcpy(PlayerAddr, ip);
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
	pthread_mutex_init(&CS, NULL); // 初始互斥
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
		char *cmd = strtok(msg, "?");
		char *param = strtok(NULL, "");
//		printf("cmd:%s, param:%s\n", cmd, param);
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
		else if (playcmd == pcPauseContinue) {
//			printf("pcPauseContinue\n");
			ShowMsgBox(param, 2000);
		}
		else if (playcmd == pcMsgBox) {
			char *m = strtok(param, ",");
			int t = atoidef(strtok(NULL, ","), 0);
//			printf("%s, %d\n", m, t);
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
#ifdef WIN32
#define socklen_t int
#endif
	struct sockaddr addr;
	int addrlen = sizeof(struct sockaddr_in);
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

bool CPlayer::SendPlayerCmdNoRecv(const char *cmd)
{
	sendto(sckfd, cmd, strlen(cmd), 0, (struct sockaddr *)&PlayAddr, sizeof(PlayAddr));
	return true;
}

bool CPlayer::RecvSongData(SelectSongNode *rec)
{
	char msg[512];
	struct sockaddr ClientAddr;
	int len = RecvUdpBuf(sckfd, msg, 511, &ClientAddr);
	if (len > 0) {
		msg[len] = '\0';
		if (strncmp(msg, "end", 3) == 0)
			return false;		
		return StrToSelectSongNode(msg, rec);
	}
	return false;
}

#define STRMCAT(str, s1, s2) { \
	if (s2) { \
		char tmpbuf[100]; \
		sprintf(tmpbuf, s1, s2); \
		strcat(str, tmpbuf); \
	} \
}

void CPlayer::NetAddSongToList(MemSongNode *rec)
{
	char msg[1024];
	struct sockaddr ClientAddr;
	sprintf(msg, "%s?id=%ld"     , ADDSONG, rec->ID);
	STRMCAT(msg, "&code=%s"      , rec->SongCode);
	STRMCAT(msg, "&name=%s"      , rec->SongName);
	STRMCAT(msg, "&charset=%d"   , rec->Charset);
	STRMCAT(msg, "&language=%s"  , rec->Language);
	STRMCAT(msg, "&singer=%s"    , rec->SingerName);
	STRMCAT(msg, "&volk=%d"      , rec->VolumeK);
	STRMCAT(msg, "&vols=%d"      , rec->VolumeS);
	STRMCAT(msg, "&num=%d"       , rec->Num);
	STRMCAT(msg, "&klok=%d"      , rec->Klok);
	STRMCAT(msg, "&sound=%d"     , rec->Sound);
	STRMCAT(msg, "&soundmode=%d" , rec->SoundMode);
	STRMCAT(msg, "&type=%s"      , rec->StreamType);
	printf("msg=%s\n", msg);
#ifndef WIN32
	pthread_mutex_lock(&CS);
#endif
	SendPlayerCmdNoRecv(msg);
	int len = RecvUdpBuf(sckfd, msg, 511, &ClientAddr);
	if (len > 0) {
		char *cmd   = strtok(msg, "?");
		char *param = strtok(NULL, "");
		if (strcasecmp(cmd, ADDSONG)==0){
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
}

bool CPlayer::NetDelSongFromList(SelectSongNode *rec)  /* 从已点歌曲列表中删除记录   */
{
	char *p = SelectSongNodeToStr(DELSONG, rec);
	SendPlayerCmdNoRecv(p);
//	DelSongFromList(rec);
	free(p);
	return true;
}

bool CPlayer::NetFirstSong(SelectSongNode *rec, int id) /* 优先歌曲 */
{
	char *p = SelectSongNodeToStr(FIRSTSONG,rec);
	SendPlayerCmdNoRecv(p);
	free(p);
	return true;
}

bool CPlayer::NetSongCodeInList(const char *songcode) /* 指定的歌曲编号是否在列表中 */
{
	SongCodeInList(songcode);
	return true;
}

void CPlayer::ReloadSongList()
{
	SendPlayerCmdNoRecv(LISTSONG);
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

bool CPlayer::SendPlayerCmdAndRecv(const char *cmd) // 发送命令并且等回复消息
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
		char *cmd   = strtok(msg, "?");
		char *param = strtok(NULL, "");
		int time = 3000;
		if (param)
			time=atoidef(param, time);
		ShowMsgBox(cmd, time);
	}
	else 
		ShowMsgBox(NULL, 0);
#ifndef WIN32
	pthread_mutex_unlock(&CS);
#endif
	return true;
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
		return atoidef(msg, 0);
	}
	return -1;
}

void CPlayer::PlayDisc()
{
	MemSongNode disc;
	memset(&disc, 0, sizeof(MemSongNode));
	disc.SongCode = strdup("0");
	disc.SongName = strdup("光驱播放");
	NetAddSongToList(&disc);
	free(disc.SongCode);
	free(disc.SongName);
}

bool CPlayer::CheckVideoCard()
{
	SendPlayerCmdAndRecv("hwstatus");
	return true;
}

int CPlayer::AddVolume()
{
	SendPlayerCmdNoRecv("addvolume");
	return ReadIntFromPlayer();
}
int CPlayer::DecVolume()
{
	SendPlayerCmdNoRecv("delvolume");
	return ReadIntFromPlayer();
}

