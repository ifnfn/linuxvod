#ifndef _DSK31_H_
#define _DSK31_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define CONNECT_LEN 4

bool dskConnect(int port);                             // ��ǰ����������
void dskDisconnect(void);                              // �Ͽ���ǰ��������
bool SendDskCmd(const char *CmdName, int count, ... ); // ִ������

#endif
