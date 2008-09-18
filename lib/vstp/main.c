#include <stdio.h>
#include <stdlib.h>

#include "vstp.h"

int main(int argc, char**argv)
{
	vstp_t *PlayVstp;
	char cmd[100];
	int readBytes;
	unsigned char buffer[193];
	PlayVstp = CreateVstp(1000);
	int i;
	for (i=1;i<argc;i++){
		sprintf(cmd, "code=%s", argv[i]);
		printf("cmd=%s\n", cmd);
		if (OpenUrl(PlayVstp, cmd) ) {
			printf("OpenChannel Error\n");
			continue;
		}
		sprintf(cmd, "/%s.mpg", argv[i]);
		FILE *fp = fopen(cmd, "wb");
		long size = 0;
		do{     // 如果已连接
			readBytes = ReadUrl(PlayVstp, buffer, sizeof(buffer)-1); // 读数据
//			printf("readbytes=%d\n", readBytes);
			fwrite(buffer, readBytes, 1, fp);
			size+=readBytes;
		} while (!EofUrl(PlayVstp));
		CloseUrl(PlayVstp);
		fclose(fp);
	}
	return 0;
}
