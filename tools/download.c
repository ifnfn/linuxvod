#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "avio/avio.h"

int main(int argc, char *argv[])
{
	if (argc < 2){
		printf("Usage: %s <remotefile> [savefile]\n", argv[0]);
		return -1;
	}
	char host[30];
	FindServerHost(host, argv[0]);
	if ((strlen(host)<= 0)) {
//		printf("not found server\n");
		return -1;
	}
	char RecvBuf[4093];
	int RecvLen = 0;
	char url[1024];
	sprintf(url, "http://%s/%s", host, argv[1]);
	offset_t size;
	URLContext *uc = NULL;
	av_register_all();
	if (url_open(&uc, url, URL_RDONLY) < 0) return -1;
	int fp = 1;
	if (argc == 3)
		fp = open(argv[2], O_WRONLY | O_CREAT);
	while (1) {
		size = url_read(uc, RecvBuf, 4092);
		if (size <= 0) break;
		RecvBuf[size] ='\0';
		write(fp, RecvBuf, size);
	}
	url_close(uc);

	if (fp != 1)
		close(fp);
	return 0;
}
