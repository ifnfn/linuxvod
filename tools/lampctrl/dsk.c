#include <stdio.h>
#include <stdlib.h>
#include "serial.h"
#include "dsk31.h"

/**********************************************************************
 * ����˵����ʹ�ô��ڶ����Եģ����͵��������ַ���
 * ����û�з����ַ����������ţ����Խ��յ��󣬺�������˽������š�
 * �Ҳ���ʹ�õ��ǵ�Ƭ���������ݵ��ڶ������ڣ�����ͨ����
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
