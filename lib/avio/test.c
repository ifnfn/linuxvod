#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "avio.h"

int main(int argc, char **argv)
{
	av_register_all();
#if 1
	ByteIOContext io, *h = &io;
	unsigned char buffer[100];
	int len, i;

	if (url_fopen(h, argv[1], O_RDONLY) < 0)
	{
		printf("could not open '%s'\n", argv[1]);
		return -1;
	}
	while(1)
	{
		len = url_fread(h, buffer, sizeof(buffer));
		if (len <= 0)
			break;
		for(i=0;i<len;i++) putchar(buffer[i]);
	}
	url_fclose(h);
#else
	URLContext *x = NULL;
	unsigned char buffer[102400];
	if (url_open(&x, argv[1], URL_RDONLY) >= 0)
	{
		printf("size=%lld\n", url_size(x));
		while (1)
		{
			int l=url_read(x, buffer, 102400);
			if (l<=0) break;
			buffer[l]='\0';
			printf("%s", buffer);
		}
		url_close(x);
	} else
		printf("open error.\n");
		char localA[100]="", localB[100]="";
		memset(localA, 0, 100);
		memset(localB, 0, 100);
		char auth[100], hostname[100];
		int port;
		url_split(NULL, 0, auth, sizeof(auth), hostname, sizeof(hostname), &port,
			localA, sizeof(localA), "http://192.168.0.138/abc/url.txt");

#endif
	return 0;
}

