/*==============================================================================
 * 视频流媒传输协议 1.0版
 *
 * vstp_t *CreateChannel(char *CmdStr, int timeout); // 创建VSTP连接
 * 说明    : 创建VSTP连接
 * CmdStr : 格式串   Code=00000001
 * Timeout: 创建超时
 * 返回    : VSTP句柄
 *
 * int ReadChannel(vstp_t *vstp, char *Buffer, int Count); // 读数据
 * 说明    : 从vstp中读取数据
 * vstp   : vstp 句柄
 * Buffer : 读取数量放在这里
 * Count  : 读取多少数据由这里指定
 * 返回    : 返回实际读取的数据量
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
	char code[50];      // 歌曲代码
	char remote[255];   // 主机列表
	long Size;          // 歌曲视频文件大小
} VstpCommand;

typedef struct tagVstp {
	char           code[50];      // 歌曲代码
	struct timeval timeout;       // 超时
	unsigned char  encrypt;       // 是否加密
	uint8_t        keycode;       // 如果是加密，则计算 K 值
	bool           connected;
	char           remoteurl[255];// 远程地址
	int64_t        remotesize;    // 远程大小
	int64_t        position;      // 接收到什么地方
	ByteIOContext  io;
	ByteIOContext *pcontext;
} vstp_t;

#ifdef __cplusplus
extern "C" {
#endif
	
vstp_t *CreateVstp(int timeout);  // 创建VSTP连接
void FreeVstp(vstp_t *vstp);      // 关闭vstp连接

int  OpenUrl (vstp_t *vstp, const char *url);
int  ReadUrl (vstp_t *vstp, char *Buffer, int Count);
void CloseUrl(vstp_t *vstp);
bool EofUrl  (vstp_t *vstp);

#ifdef __cplusplus
}
#endif
	
#endif // VSTP_H
