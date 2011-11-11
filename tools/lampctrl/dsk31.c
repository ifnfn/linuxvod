#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "dsk31.h"
#include "serial.h"

const unsigned char ASKDATA[]     = {0x9E, 0x51};             // 前级应答数据
const unsigned char ASKDATA2[]    = {0x51};                   // 前级应答数据
const unsigned char NOASK[]       = {0x5F};                   // 前级无应答数据
const unsigned char CONNECTDATA[] = {0x2 , 0x30, 0x51, 0x81}; // 前级联接命令
const unsigned char CONNECT_OK[]  = {0x86};                   // 前级应答数据
const unsigned char HEAD[]        = {0x53, 0x99, 0xC0, 0x00}; // 头信息
const unsigned char TAIL[]        = {0x53, 0x99, 0xC0, 0xA5}; // 尾信息

struct tagCMD {
	char CmdName[20];
	char len;
	unsigned char CmdData[3];
};

#define CmdCount 12
struct tagCMD DSK31Cmd[] = {
	{"Connect",       2, {0x30, 0x51, 0x0}},
	{"Diconnect"    , 2, {0x30, 0x00, 0x0}},
	{"MicFeedBack"  , 2, {0x53, 0x19, 0x0}},  // 麦克风抑制
	{"MicVolume"    , 2, {0x53, 0xD9, 0x0}},  // 麦克风音量
	{"CenterGain"   , 2, {0x53, 0x35, 0x0}},  // 麦克风中置音量
	{"CenterTreble" , 2, {0x53, 0x37, 0x0}},  // 麦克风中置音量
	{"CenterBass"   , 2, {0x53, 0x39, 0x0}},  // 麦克风中置音量

	{"MusicVolume"  , 2, {0x55, 0xD9, 0x0}},  // 音乐音量
	{"SubWoofer"    , 2, {0x55, 0xE7, 0x0}},  // 超低音量
	{"SubWooferFreq", 2, {0x55, 0xF3, 0x0}},  // 超低音分频点
	{"MusicTreble"  , 3, {0x55, 0x15, 0x1}},  // 音乐高音音量
	{"MusicBass"    , 3, {0x55, 0x15, 0x2}}   // 音乐低音音量
};

static int dskport=-1; // DSK31 　的串口号

/* 功能： 将前级控制命令换成前级所需要的字符串格式
 * 参数：
 * 		CmdName : 命令名称
 * 		Count   : 参数数量
 * 		... 	: 支持可变参数
 * 返回：是不成功
 */
bool SendDskCmd(const char *CmdName, int count, ... )
{
	printf("Send Command: %s\n", CmdName);
	unsigned char newcmd[512];
	va_list argp;
	unsigned char x;

/////////////////////////// 发送请求命令，等待响应 ////////////////////////////////
	newcmd[0] = 0xF3;
	WriteComm(dskport, newcmd, 1);
	if (!WaitDataRight(dskport, ASKDATA, 2)) { // 如果等待的数据是ASKDATA
		return false;
	}

/////////////////////////////// 查找到命令参数 ///////////////////////////////////
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
	if (i>=CmdCount) return false; //没有找到

////////////////////////// 生成命令串，并发送到前级 ///////////////////////////////
	va_start( argp, count); /*argp指向传入的第一个可选参数，count是最后一个确定的参数*/
	int j;
	for (j=0;j<count;j++) {
		newcmd[0]++;
		newcmd[newcmd[0]] = va_arg( argp, int); /* 取出当前的参数，类型为 int */
		x += newcmd[newcmd[0]];
	}
	va_end( argp ); /* 将argp置为NULL */
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

///////////////////////////// 等待前级处理的结果 //////////////////////////////////
	if (WaitDataRight(dskport, ASKDATA2, 1)) {
		WriteComm(dskport, CONNECT_OK, 1);
		WaitDataRight(dskport, ASKDATA2, 1);
		return true;
	}
}

bool dskConnect(int port)  // 与前级建立连接
{
	if (!(dskport=OpenComm(port, 38400, 8, "1", 'N'))){
		fprintf (stderr, "Make sure /dev/ttyS%d not in use or you have enough privilege.\n\n",port);
		exit (-1);
	}
	SendDskCmd("Connect", 0);
}

void dskDisconnect(void)  // 断开与前级的连接
{
	if (dskport != -1) {
		SendDskCmd("Diconnect", 0);
		CloseComm(dskport);
		dskport = -1;
	}
}
