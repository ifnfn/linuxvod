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
#include "songdata.h"
#include "crypt/hd.h"
#ifdef OSDMENU
#include "osd.h"
#endif

static void *PlayVstpQueueThread(void *val)
{
	struct sockaddr ClientAddr;
	char msg[512];
	INFO *tmpInfo = (INFO *)val;
	fd_set fdread;
	int ret, addrlen;
	struct timeval tv;

	CHKREG
	int udpsock = CreateUdpBind(6788);
	SetBlocking(udpsock);
	while (!tmpInfo->quit){
		if (tmpInfo->PlayStatus == stStoped) 
			SongListFirstPlay(tmpInfo);               // 如果播放列表中第一首播不了，

		VodSyncAudioStream(tmpInfo);
		FD_ZERO(&fdread);
		FD_SET(udpsock, &fdread);

		tv.tv_sec = 0;
		tv.tv_usec= 1000 * 100;
		if ((ret = select(udpsock + 1, &fdread, NULL, NULL, &tv)) == -1){
			perror("select socket error.\n");
			continue;
		}
		if (FD_ISSET(udpsock, &fdread)) {
			addrlen = sizeof(struct sockaddr_in);
			int len = recvfrom(udpsock, msg, 511, 0, &ClientAddr ,(socklen_t*)&addrlen);
			if (len > 0) {
				msg[len] = '\0';
				if (strcasecmp(msg, "WHO") == 0)
					ClientLogin(1);
				else
					process(tmpInfo, msg, NULL, udpsock);
			}
		}
	}

	return NULL;
}

#define BACKLOG 150
static int CreateTCPBind(int port)
{
	int sockfd, n;                         /* listen on sock_fd, new connection on new_fd */
	struct sockaddr_in my_addr;            /* my address information */

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("socket: Could create a socket, exiting\n");
		return 0;
	}

 	/* 如果服务器终止后,服务器可以第二次快速启动而不用等待一段时间  */
        n = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&n, sizeof(n));

	my_addr.sin_family      = AF_INET;     /* host byte order             */
	my_addr.sin_port        = htons(port); /* short, network byte order   */
	my_addr.sin_addr.s_addr = INADDR_ANY;  /* auto-fill with my IP        */
	bzero(&(my_addr.sin_zero), 8);         /* zero the rest of the struct */

	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
		printf("bind: Could not bind to port, exiting\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		printf("listen: Unable to do a listen()\n");
		exit(1);
	}
	DEBUG_OUT("Create TCP Server at port %d\n", port);

	return sockfd;
}

static int ConnectWorkThread(INFO *pInfo, int sockfd)
{
#define SIZE 1024
	char *tempdata = NULL, *ptr;
	char *tempstring = NULL;
	unsigned int loop=0;
	int numbytes=0;
	int err = -1;
	struct sockaddr_in sa;
	int addrlen = sizeof(struct sockaddr_in);
	char * stringpos = NULL;
	char status[10];

	tempdata = (char *)malloc(SIZE);
	tempstring = (char *)malloc(SIZE);

	memset(tempdata, 0, SIZE);
	memset(tempstring, 0, SIZE);
#undef SIZE

	while(!strstr(tempdata, "\r\n\r\n") && !strstr(tempdata, "\n\n")) {
		if ((numbytes=recv(sockfd, tempdata+numbytes, 4096-numbytes, 0))==-1) 
			goto end;
	}
	for(loop=0; loop<4096 && tempdata[loop]!='\n' && tempdata[loop]!='\r'; loop++)
		tempstring[loop] = tempdata[loop];

	tempstring[loop] = '\0';
	ptr = strtok(tempstring, " ");

	if (ptr == NULL) 
		goto end;

	if (strcmp(ptr, "GET") && strcmp(ptr, "POST"))
		goto end;

	strcpy(status, ptr);
	ptr = strtok(NULL, " ");
	if (ptr == NULL) {
		goto end;
	}

	getpeername(sockfd, (struct sockaddr *)&sa, (socklen_t*)&addrlen);
	DEBUG_OUT("Connection from %s, request = \"%s %s\"\n", inet_ntoa(sa.sin_addr), status, ptr);

	// ***** PATCH *****
	// Replaces %20s of the message string by blanks
	while (strstr(ptr,"%20")!=NULL) {
		stringpos = strstr(ptr, "%20");
		*stringpos = (char)32;
		strcpy(stringpos + 1, stringpos + 3);
	}
	// ***** END *****

	if (strcmp(status, "GET") == 0) {
		char *cmd = ptr +1, *param;
		param = strstr(ptr, "?");
		if (param != NULL) { 
			param[0] = '\0'; 
			param++;
		}

		process(pInfo, cmd, param, sockfd);
	}
	else if (strcmp(status, "POST") == 0){

	}
	err = 0;
end:
	if (tempdata) 	free(tempdata);
	if (tempstring) free(tempstring);

	close(sockfd);
	return err;
}


int main(int argc, char **argv)
{
	int sockfd;
#ifndef NETPLAYER
	remove(argv[0]);
#endif	
	CHKREG
	INFO Info;
	InitInfo(&Info);
	if (Info.HaveHW == 0) {                                   // 如果没有找到硬件
		DEBUG_OUT("CreatePlayer Error.\n");
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
	ReadPlayIniConfig(playurl, videourl);
#ifdef OSDMENU
	int i;
	for (i=0; i<OSDCount;i++)
		CreateThreadList(OSDList[i]);
#endif

	sockfd = CreateTCPBind(PLAYERUDPPORT);
	if (sockfd <= 0)
		return -1;

//	RunSoundMode(&Info, "0");
	pthread_t PlayPthread = 0;
	pthread_create(&PlayPthread, NULL, &PlayVstpQueueThread, (void *)(&Info));

	CHKREG

	while(1) {
		int new_fd;
		struct sockaddr their_addr;
		int sin_size = sizeof(struct sockaddr_in);

		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, (socklen_t*)&sin_size)) == -1)
			continue;
		AppendClientToList(their_addr);
		ConnectWorkThread(&Info, new_fd);
		close(new_fd);
	}

	ClientLogin(false);
	CloseUdpSocket();
	return 0;
}
