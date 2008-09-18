#include <stdio.h>
#include <stdlib.h>

#include "vstp.h"

int main(int argc, char**argv)
{
	vstp_t *PlayVstp;
	char cmd[100];
	int readBytes;
	unsigned char buffer[8193];
	PlayVstp = CreateVstp(1000);
	int i;
	int count = atoi(argv[1]);
	for (i=0;i<count;i++){
		sprintf(cmd, "code=%s", argv[2]);
		if ( OpenUrl(PlayVstp, cmd) < 0 ) {
			printf("OpenUrl Error.\n");
			continue;
		}
		sprintf(cmd, "/%d-%s.mpg", i, argv[2]);
		FILE *fp = fopen(cmd, "wb");
		long size = 0;
		do{     // 如果已连接
			readBytes = ReadUrl(PlayVstp, buffer, 8192); // 读数据
			if (readBytes <= 0) {
				break;
			}
			fwrite(buffer, readBytes, 1, fp);
//			buffer[readBytes] = '\0';
//			printf("%s", buffer);
			size+=readBytes;
		} while (1);
		CloseUrl(PlayVstp);
		fclose(fp);
//		printf("(%d) Recv=%d\n", i, PlayVstp->recvsize);
	}
	FreeVstp(PlayVstp);
	return 0;
}
