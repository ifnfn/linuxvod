#include <stdio.h>
#include <stdlib.h>
#include "serial.h"
#include "dsk31.h"

/**********************************************************************
 * 代码说明：使用串口二测试的，发送的数据是字符，
 * 但是没有发送字符串结束符号，所以接收到后，后面加上了结束符号。
 * 我测试使用的是单片机发送数据到第二个串口，测试通过。
**********************************************************************/
int main(int argc, char **argv)
{

	dskConnect(0);
	int volume=0;
	while (1) {
		printf("input: ");
		fflush(stdout);
		scanf("%d", &volume);
		if (volume == -1)
			break;
//		SendDskCmd("MusicVolume", 1, volume);
		SendDskCmd("MicVolume", 1, volume);

	}
	sleep(1);
	dskDisconnect();
	return 0;
}
