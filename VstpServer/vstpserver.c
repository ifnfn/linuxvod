#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <net/if.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <ftw.h>
#include <sqlite.h>

#include "vstpserver.h"

char localip[16];

static sqlite *db = NULL;
static char *ErrMsg = 0;

typedef struct {
	int code;
	char file[100];
	int64_t size;
} SongList;

static SongList *songlist = NULL;
static int count = 0;


static int fn(const char *file, const struct stat *sb, int flag)
{
	char buf[1024], code[20], path[200], extname[10];
	if (flag == FTW_F)
	{
		ExtractFilePath(file, path);
		ExtractFileName(file, code);
		ExtractFileExt (file, extname); 
		if ((strcasecmp(extname, "mpg") == 0) || 
			(strcasecmp(extname, "dat") == 0) ||
			(strcasecmp(extname, "m1s") == 0) ||
			(strcasecmp(extname, "vob") == 0) ||
			(strcasecmp(extname, "avi") == 0) ||
			(strcasecmp(extname, "div") == 0) )
		{
		}
		songlist = realloc(songlist, sizeof(SongList) * (count+1));
		songlist[count].code = atoi(code);
		songlist[count].size = sb->st_size;
		strncpy(songlist[count].file, file, 99);
		count++;
	}
	return 0;
}

static int cmp( const void *a , const void *b ) 
{ 
	SongList *A = (SongList*)a, *B = (SongList*)b;
	return A->code > B-> code ? 1 : (A->code == B->code ? 0 : -1);
}

void InitSongList(const char *path)
{
	if (songlist) free(songlist);
	songlist = NULL;
	ftw(path, fn, 500);
	qsort(songlist, count, sizeof(SongList), cmp);
}

int64_t geturlbycode(char *code, char *url)
{
	SongList key;
	key.code = atoi(code);

	SongList *p = bsearch(&key, songlist, count, sizeof(SongList), cmp);
	if (p) {
		strcpy(url, p->file);
		return p->size;
	}
	return -1;
}

int CreateUDPServer(int MaxNum, int port)
{
	int accept_fd, udpfd;
	InitSongList("/ktvdata/video");
	char xxx[1000];
	if (geturlbycode("67430", xxx) > -1)
		printf("xxx=%s\n", xxx);

	struct sockaddr_in client_addr;
	int addr_len = sizeof(struct sockaddr_in);
	if((udpfd = socket(AF_INET, SOCK_DGRAM, 0))<0){
		fprintf(stderr, "Socket Error:%s\n\a", strerror(errno));
		exit(1);
	}
	bzero(&client_addr, addr_len);
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(port);
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int n = 1;
	setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(int));
	n = 1;
	setsockopt(udpfd, SOL_SOCKET, SO_BROADCAST, &n, sizeof(int));

	if(bind(udpfd, (struct sockaddr *)&client_addr, sizeof(struct sockaddr_in)) < 0){
		fprintf(stderr,"Bind Error:%s\n\a", strerror(errno));
		return 1;
	}
	listen(udpfd, MaxNum);

	char buf[1024], *cmd, *param;
	int len;
	printf("Start Wait...\n");
	while(1) 
	{
		addr_len = sizeof(struct sockaddr);
		len = recvfrom(udpfd, buf, 1024, 0, (struct sockaddr*)&client_addr, &addr_len);

		if (len > 1023) len = 1023;
		buf[len] = '\0';
		printf("UDP Recv %s from %s:%d\n", buf, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		cmd = strtok(buf, " ");
		if (!strcasecmp(cmd, "WHO"))
		{
			sprintf(buf, "Status Server:%s=%s", localip, "Open");
			len = strlen(buf);
		}
		else if (!strcasecmp(cmd, "HAVE")) 
		{
			char *code =strtok(NULL, " ");
			if (code) 
			{
				char url[1024]="";
				int64_t size = geturlbycode(code, url);
				if (size > 0)
					sprintf(buf, "URL http://%s:%s/%s|%lld", url, size);
			}
		}
		printf("send: %s to %s:%d\n", buf, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		if (buf[0] != '\0')
			int a = sendto(udpfd, buf, strlen(buf), 0, (struct sockaddr*)&client_addr, sizeof(struct sockaddr));
	}
}

