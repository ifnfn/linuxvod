#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>

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
#include "avio/avio.h"

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

	struct sockaddr_in RecvAddr;
	memset(&RecvAddr, 0, sizeof(RecvAddr));
	RecvAddr.sin_family      = AF_INET;
	RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	RecvAddr.sin_port        = htons(PLAYPORT);
	if( (udpsvrfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))== -1){
		perror("Dont get a socket.\n");
		exit(-1);
	}
	if(bind(udpsvrfd, (struct sockaddr *)&RecvAddr, sizeof(RecvAddr)) == -1)	{
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
	pthread_create(&thread, NULL, &ControlPlayThread, this);
	ReloadSongList();
	stack = CWindowStack::CreateStack();
}

CPlayer::~CPlayer()
{
	working = false;
	close(udpsvrfd);
}

#if 1
bool CPlayer::RecvPlayerCmd() /* 处理播放器发过来的命令 */
{
	char msg[512];
	int update = 0;
	int size = RecvUdpBuf(udpsvrfd, msg, 511);

	if (size>0) {
		char *cmd = strtok(msg, "?");
		char *param = strtok(NULL, "");
//		printf("cmd:%s, param:%s\n", cmd, param);
		SelectSongNode rec;

		if (strcasecmp(cmd, "addsong") == 0) {
			StrToSelectSongNode(param, &rec);
			AddSongToList(&rec, false);   // 增加新的歌曲
		}
		else if (strcasecmp(cmd, "delsong") == 0) {
			StrToSelectSongNode(param, &rec);
			DelSongFromList(&rec);
			update = 1;
		}
		else if (strcasecmp(cmd, "firstsong") == 0) {
			StrToSelectSongNode(param, &rec);
			FirstSong(&rec, 1);
		}
		else if (strcasecmp(cmd, "PauseContinue") == 0) {
//			printf("pcPauseContinue\n");
			ShowMsgBox(param, 2000);
		}
		else if (strcasecmp(cmd, "MsgBox") == 0) {
			char *m = strtok(param, ",");
			int t = atoidef(strtok(NULL, ","), 0);
//			printf("%s, %d\n", m, t);
			ShowMsgBox(m, t);
			return true;
		}
		else if (strcasecmp(cmd, "ReSongDB") == 0) {
			if (theme) {
				theme->songdata->Reload();
				theme->singerdata->Reload();
			}
		}
		else if (strcasecmp(cmd, "playsong") == 0) {

		}
		else {
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
		CBaseWindow *tmp = stack->WindowTop();
		if (tmp && update) {
			printf("stack->WindowTop Paint\n");
			tmp->Paint();
		}
		return true;
	}
	return false;
}
#endif

int CPlayer::RecvUdpBuf(int fd, char *msg, int MaxSize)
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
	else
		msg[len] = '\0';

	return len;
}

bool CPlayer::SendPlayerCmdNoRecv(char *cmd)
{
	char *data = SendPlayerCmd(cmd);
	
	if (data)
		free(data);

	return true;
}

void CPlayer::NetAddSongToList(MemSongNode *rec)
{
	char *data = SendPlayerCmd("%s?id=%ld&code=%s&name=%s&charset=%d&language=%s&singer=%s&volk=%d"
			"&vols=%d&num=%d&klok=%d&sound=%d&soundmode=%d&type=%s&password=%ld",
			ADDSONG, rec->ID, 
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
			rec->StreamType,
			rec->Password);

	if (data) {
#if 0
		printf("data=%s\n", data);
		char *cmd   = strtok(data, "?");
		char *param = strtok(NULL, "");

		if (strcasecmp(cmd, ADDSONG) == 0) {
			SelectSongNode rec;
			if (StrToSelectSongNode(param, &rec) == true)
				AddSongToList(&rec, false);
		}
		else 
			ShowMsgBox(param, 3000);
#endif
		free(data);
	}
}

bool CPlayer::NetDelSongFromList(SelectSongNode *rec)  /* 从已点歌曲列表中删除记录   */
{
	char *p = SelectSongNodeToStr(NULL, rec);
	if (p) {
		char *data = SendPlayerCmd("%s?%s", DELSONG, p);
		free(p);

		if (data) {
//			DelSongFromList(rec);
			free(data);
		}
	}

	return true;
}

bool CPlayer::NetFirstSong(SelectSongNode *rec, int id) /* 优先歌曲 */
{
	char *p = SelectSongNodeToStr(NULL, rec);
	if (p) {
		char *data = SendPlayerCmd("%s?%s", FIRSTSONG, p);
		free(p);

		if (data) {
//			DelSongFromList(rec);
			free(data);
		}
	}

	return true;
}

bool CPlayer::NetSongCodeInList(const char *songcode) /* 指定的歌曲编号是否在列表中 */
{
	return SongCodeInList(songcode);
}

void CPlayer::ReloadSongList()
{
	char buffer[1024];
	ByteIOContext io;

	av_register_all();
	GetURL(buffer, LISTSONG);
	if (url_fopen(&io, buffer, O_RDONLY) >= 0) {
		ClearSongList();
		while ( !url_feof(&io) ) {
			if (url_fgets(&io, buffer, 1023) != NULL) {
//				printf("buffer=%s\n", buffer);
				SelectSongNode rec;
				if (StrToSelectSongNode(buffer, &rec))
					AddSongToList(&rec, false);
				else
					break;
			}
			else
				break;
		}
		url_fclose(&io);
	}
}

bool CPlayer::SendPlayerCmdAndRecv(char *cmd) // 发送命令并且等回复消息
{
	char *data = SendPlayerCmd(cmd);

	if (data) {
		char *cmd   = strtok(data, "?");
		char *param = strtok(NULL, "");
		int time = 3000;
		if (param)
			time=atoidef(param, time);
		ShowMsgBox(cmd, time);
		free(data);
	}
	else 
		ShowMsgBox(NULL, 0);

	return true;
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
	SendPlayerCmd("hwstatus");
	return true;
}

void CPlayer::GetURL(char *temp, const char *format, ...)
{
	va_list ap;
	int newformat_len;

	sprintf(temp, "http://%s:6789/", PlayerAddr);
	newformat_len = strlen(temp);

	va_start(ap, format);
	vsnprintf(temp + newformat_len, 4095 - newformat_len, format, ap);
	va_end(ap);
}

char *CPlayer::SendPlayerCmd(char *format, ...)
{
	char temp[4096];
	va_list ap;
	offset_t size = 0;
	int newformat_len;

	sprintf(temp, "http://%s:6789/", PlayerAddr);
	newformat_len = strlen(temp);

	va_start(ap, format);
	vsnprintf(temp + newformat_len, 4095 - newformat_len, format, ap);
	va_end(ap);

//	printf("SendPlayerCmd=%s\n", temp);
	
	return (char *)url_readbuf(temp, &size);
}

int CPlayer::AddVolume()
{
	int volume = 0;
	char *data = SendPlayerCmd("addvolume");

	if (data) {
//		printf("AddVolume:%s\n", data);
		volume = atoidef(data, 0);
		free(data);
	}

	return volume;
}
int CPlayer::DecVolume()
{
	int volume = 0;
	char *data = SendPlayerCmd("delvolume");

	if (data) {
//		printf("DecVolume:%s\n", data);
		volume = atoidef(data, 0);
		free(data);
	}

	return volume;
}

