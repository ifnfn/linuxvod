/*==============================================================================
 * ��Ƶ��ý����Э�� 1.0��
 *
 * vstp_t *CreateChannel(char *CmdStr, int timeout); // ����VSTP����
 * ˵��    : ����VSTP����
 * CmdStr : ��ʽ��   Code=00000001
 * Timeout: ������ʱ
 * ����    : VSTP���
 *
 * int ReadChannel(vstp_t *vstp, char *Buffer, int Count); // ������
 * ˵��    : ��vstp�ж�ȡ����
 * vstp   : vstp ���
 * Buffer : ��ȡ������������
 * Count  : ��ȡ��������������ָ��
 * ����    : ����ʵ�ʶ�ȡ��������
 *
 *============================================================================*/
#ifndef VSTP_H
#define VSTP_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#include "../strext.h"
#include "../osnet.h"
#include "../avio/avio.h"

#define MAX_MSG_SIZE  1024
#define ENCRYEDSIZE   10240 * 2048
#define RemoteOpenCmd "OPEN %s:%s,%d,%d\r\n"

typedef struct tagVstpCommand{
	char code[50];      // ��������
	char remote[255];   // �����б�
	long Size;          // ������Ƶ�ļ���С
} VstpCommand;

typedef struct tagVstp {
	char           code[50];      // ��������
	struct timeval timeout;       // ��ʱ
	unsigned char  encrypt;       // �Ƿ����
	uint8_t        keycode;       // ����Ǽ��ܣ������ K ֵ
	bool           connected;
	char           remoteurl[255];// Զ�̵�ַ
	int64_t        remotesize;    // Զ�̴�С
	int64_t        position;      // ���յ�ʲô�ط�
	ByteIOContext  io;
	ByteIOContext *pcontext;
} vstp_t;

#ifdef __cplusplus
extern "C" {
#endif
	
vstp_t *CreateVstp(int timeout);  // ����VSTP����
void FreeVstp(vstp_t *vstp);      // �ر�vstp����

int  OpenUrl (vstp_t *vstp, const char *url);
int  ReadUrl (vstp_t *vstp, char *Buffer, int Count);
void CloseUrl(vstp_t *vstp);
bool EofUrl  (vstp_t *vstp);

#ifdef __cplusplus
}
#endif
	
#endif // VSTP_H
