#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "dsk31.h"
#include "serial.h"

const unsigned char ASKDATA[]     = {0x9E, 0x51};             // ǰ��Ӧ������
const unsigned char ASKDATA2[]    = {0x51};                   // ǰ��Ӧ������
const unsigned char NOASK[]       = {0x5F};                   // ǰ����Ӧ������
const unsigned char CONNECTDATA[] = {0x2 , 0x30, 0x51, 0x81}; // ǰ����������
const unsigned char CONNECT_OK[]  = {0x86};                   // ǰ��Ӧ������
const unsigned char HEAD[]        = {0x53, 0x99, 0xC0, 0x00}; // ͷ��Ϣ
const unsigned char TAIL[]        = {0x53, 0x99, 0xC0, 0xA5}; // β��Ϣ

struct tagCMD {
	char CmdName[20];
	char len;
	unsigned char CmdData[3];
};

#define CmdCount 12
struct tagCMD DSK31Cmd[] = {
	{"Connect",       2, {0x30, 0x51, 0x0}},
	{"Diconnect"    , 2, {0x30, 0x00, 0x0}},
	{"MicFeedBack"  , 2, {0x53, 0x19, 0x0}},  // ��˷�����
	{"MicVolume"    , 2, {0x53, 0xD9, 0x0}},  // ��˷�����
	{"CenterGain"   , 2, {0x53, 0x35, 0x0}},  // ��˷���������
	{"CenterTreble" , 2, {0x53, 0x37, 0x0}},  // ��˷���������
	{"CenterBass"   , 2, {0x53, 0x39, 0x0}},  // ��˷���������

	{"MusicVolume"  , 2, {0x55, 0xD9, 0x0}},  // ��������
	{"SubWoofer"    , 2, {0x55, 0xE7, 0x0}},  // ��������
	{"SubWooferFreq", 2, {0x55, 0xF3, 0x0}},  // ��������Ƶ��
	{"MusicTreble"  , 3, {0x55, 0x15, 0x1}},  // ���ָ�������
	{"MusicBass"    , 3, {0x55, 0x15, 0x2}}   // ���ֵ�������
};

static int dskport=-1; // DSK31 ���Ĵ��ں�

/* ���ܣ� ��ǰ�����������ǰ������Ҫ���ַ�����ʽ
 * ������
 * 		CmdName : ��������
 * 		Count   : ��������
 * 		... 	: ֧�ֿɱ����
 * ���أ��ǲ��ɹ�
 */
bool SendDskCmd(const char *CmdName, int count, ... )
{
	printf("Send Command: %s\n", CmdName);
	unsigned char newcmd[512];
	va_list argp;
	unsigned char x;

/////////////////////////// ������������ȴ���Ӧ ////////////////////////////////
	newcmd[0] = 0xF3;
	WriteComm(dskport, newcmd, 1);
	if (!WaitDataRight(dskport, ASKDATA, 2)) { // ����ȴ���������ASKDATA
		return false;
	}

/////////////////////////////// ���ҵ�������� ///////////////////////////////////
	memset(newcmd, 0, 512);
	int i;
	for (i=0;i<CmdCount;i++) {
		if (strcasecmp(DSK31Cmd[i].CmdName, CmdName) == 0) {
			memcpy(newcmd + 1, DSK31Cmd[i].CmdData, DSK31Cmd[i].len);
			newcmd[0] = DSK31Cmd[i].len;
			int j;
			x=0;
			for (j=1; j<=DSK31Cmd[i].len; j++)
				x  = x + newcmd[j];
			break;
		}
	}
	if (i>=CmdCount) return false; //û���ҵ�

////////////////////////// ��������������͵�ǰ�� ///////////////////////////////
	va_start( argp, count); /*argpָ����ĵ�һ����ѡ������count�����һ��ȷ���Ĳ���*/
	int j;
	for (j=0;j<count;j++) {
		newcmd[0]++;
		newcmd[newcmd[0]] = va_arg( argp, int); /* ȡ����ǰ�Ĳ���������Ϊ int */
		x += newcmd[newcmd[0]];
	}
	va_end( argp ); /* ��argp��ΪNULL */
	newcmd[newcmd[0] + 1] = x;

	int len = newcmd[0] + 2;
	printf("len(%d): ", len);
	for (i=0;i<len;i++)
		printf("%02x ", newcmd[i]);
	printf("\n");

	char buf[200];
	memcpy(buf, HEAD, 4);

	memcpy(buf + 4, newcmd, len);

	memcpy(buf + 4 + len, TAIL, 4);
	WriteComm(dskport, buf, len + 8);

///////////////////////////// �ȴ�ǰ������Ľ�� //////////////////////////////////
	if (WaitDataRight(dskport, ASKDATA2, 1)) {
		WriteComm(dskport, CONNECT_OK, 1);
		WaitDataRight(dskport, ASKDATA2, 1);
		return true;
	}
}

bool dskConnect(int port)  // ��ǰ����������
{
	if (!(dskport=OpenComm(port, 38400, 8, "1", 'N'))){
		fprintf (stderr, "Make sure /dev/ttyS%d not in use or you have enough privilege.\n\n",port);
		exit (-1);
	}
	SendDskCmd("Connect", 0);
}

void dskDisconnect(void)  // �Ͽ���ǰ��������
{
	if (dskport != -1) {
		SendDskCmd("Diconnect", 0);
		CloseComm(dskport);
		dskport = -1;
	}
}
