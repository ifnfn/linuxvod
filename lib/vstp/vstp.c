#include <string.h>
#include <assert.h>
#include <sys/timeb.h>
#ifndef WIN32
	#include <sys/ioctl.h>
	#include <sys/errno.h>
	#include <fcntl.h>
#endif

#include "vstp.h"

#define QUEUELEN 200

static char BroadCastIP[16] = "255.255.255.255";

static int64_t FindSongUrl(vstp_t *vstp)
{
	if (!vstp) return 0;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0){
		DEBUG_OUT("hostlist-> udpsvrfd Error.\n");
		return 0;
	}
	SetBroadCast(fd);
	SetSocketSendBuf(fd, 0);
	GetBroadCastIP(BroadCastIP);
	int flags;
	if ((flags = fcntl(fd, F_GETFL, 0)) < 0) {
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
	}

	struct sockaddr_in addr;
	char msg[512];
	sprintf(msg, "HAVE %s", vstp->code);
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = inet_addr(BroadCastIP);
	addr.sin_port        = htons(VSTPPORT);
	sendto(fd, msg, strlen(msg), 0, (struct sockaddr*)&addr, sizeof(addr));


	fd_set  rdevents;
	struct  timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	FD_ZERO(&rdevents);
	FD_SET(fd, &rdevents);
	select(fd + 1, &rdevents, NULL, NULL, &timeout);
	if (FD_ISSET(fd, &rdevents)) {
		struct sockaddr_in sin;
		int Addrlen, len;
		char *recvstr = msg;
		memset(&sin, 0, sizeof(struct sockaddr_in));
		Addrlen = sizeof(struct sockaddr_in);
		len = recvfrom(fd, msg, 511, 0 , (struct sockaddr *)&sin, (socklen_t*)&Addrlen);
		if (len > 0) {
			msg[len] = 0;
			printf("msg=%s\n", msg);
			char* cmd = strtok(recvstr, " ");
			if (!strcasecmp(cmd, "URL")) {
				char* purl = strtok(NULL, "|");
				strcpy(vstp->remoteurl, purl);
				char *p = purl + strlen(purl) - 1;
				while (p > purl) {
					if (*p == '.') break;
					p--;
				}
				if (p > purl) {
					p++;
					vstp->encrypt  = ((strcasecmp(p,"mpg")==0)||(strcasecmp(p,"mp1")==0));
				}
				purl = strtok(NULL, "|");
				int64_t len = atol(purl);
				vstp->remotesize = len;
				return len;
			}
		}
	}
	vstp->remotesize = 0;
	return 0;
}

static char *fetch(char **AInput, const char c)
{
	int len = strlen(*AInput);
	int i;
	char *p = *AInput;
	for(i=0; (i<len) && (p[i] != c); i++);
	if (i < len) {
		p[i] = '\0';
		*AInput = p +i + 1;
	} else
		*AInput = p + i;
	return p;
}

static void DecodeVstpCmd(const char *CmdStr,  VstpCommand *cmd)   // 将字符串转成结构体
{
	int CmdLen = strlen(CmdStr);
	char *cmdtmp = (char *) malloc(CmdLen + 1);
	assert(cmdtmp);
	char *tmp, *k, *p=cmdtmp;

	strcpy(cmdtmp, CmdStr);

	memset(cmd, 0, sizeof(VstpCommand));
	while (strlen(cmdtmp) != 0){
		tmp = fetch(&cmdtmp, '&');
		k   = fetch(&tmp, '=');
		if (strcasecmp(k, "Code") == 0)
			strcpy(cmd->code, tmp);
		else if (strcasecmp(k, "Remote") == 0)
			strcpy(cmd->remote, tmp);
		else if (strcasecmp(k, "size") == 0)
			cmd->Size = atoi(tmp);
	}
	free(p);
}

void printvstp(vstp_t *vstp)
{
	if (vstp == NULL) return;
	DEBUG_OUT("======================================================================\n");
	DEBUG_OUT("Code         = %s\n", vstp->code);           // 歌曲代码
	DEBUG_OUT("======================================================================\n");
}

static void ClearVstp(vstp_t *vstp)
{
	if (vstp->connected == true)
		url_fclose(vstp->pcontext);
	vstp->connected = false;
	vstp->position = 0;

	vstp->remoteurl[0] ='\0';
	vstp->remotesize = 0;
}

vstp_t *CreateVstp(int timeout) // 创建VSTP连接
{
	av_register_all();
	vstp_t *p = (vstp_t *)malloc(sizeof(vstp_t));
	assert(p);
	memset(p, 0, sizeof(vstp_t));
	ClearVstp(p);
	p->timeout.tv_sec  = timeout / 1000;
	p->timeout.tv_usec = (timeout % 1000) * 1000;
	p->pcontext         = &p->io;
	return p;
}

void FreeVstp(vstp_t *vstp)  // 关闭 vstp 连接
{
	if (vstp == NULL) return;
	free(vstp);
}

bool EofUrl(vstp_t *vstp)
{
	if (vstp == NULL) return true;
//	printf("vstp-connected=%d\n", vstp->connected);
	if (vstp->connected == 0) return true;
	return vstp->position >= vstp->remotesize;
}

void CloseUrl(vstp_t *vstp)
{
	if (vstp == NULL) return;
	if (vstp->connected == false) return;
	vstp->position = vstp->code[0] = 0;
	url_fclose(vstp->pcontext);
	vstp->connected = false;
}

int OpenUrl(vstp_t *vstp, const char *url)
{
	if (url== NULL) return 1;
	VstpCommand VstpCmd;
	DecodeVstpCmd(url, &VstpCmd);
	strcpy(vstp->code, VstpCmd.code);
	ClearVstp(vstp);

	vstp->keycode = atoi(VstpCmd.code) % 0xFF;

	FindSongUrl(vstp);
//	vstp->remotesize = FindSongUrl(vstp->code, vstp->remoteurl);
//	printf("RemoteUrl: %s\n", vstp->remoteurl);
	if (vstp->remotesize>0) {
		if (url_fopen(vstp->pcontext, vstp->remoteurl, URL_RDONLY) == 0) {
			vstp->connected = true;
			return 0;
		} else
			return -ENOENT;
	}
	return -ENOENT;
}

int ReadUrl(vstp_t *vstp, char *Buffer, int Count)
{
	if (vstp == NULL) return 0;
	if (!vstp->connected) return 0;
	int readcount = url_fread(vstp->pcontext, (unsigned char*)Buffer, Count);
	if (readcount > 0) {
		if (vstp->encrypt) {
			int i;
			char TmpByte;
			for (i=0; (i < readcount) && (vstp->position + i < ENCRYEDSIZE); i++){
				TmpByte = *(Buffer + i);
				*(Buffer + i) = TmpByte ^ vstp->keycode;
				vstp->keycode = ~TmpByte;
			}
		}
		vstp->position+=readcount;
	}
	return readcount;
}

