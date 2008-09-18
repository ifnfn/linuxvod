/*==============================================================================
   T h e   K T V L i n u x   P r o j e c t
 -------------------------------------------------------------------------------
   Filename   : selected.h
   Author(s)  : Silicon
   Copyright  : Silicon

   一些扩展的字符串操作函数，以及相关需要的函数
==============================================================================*/

#include <stdbool.h>
/*============================================================================*/

#ifdef WIN32
	#pragma comment(lib, "libcmt.lib")
	#pragma comment(lib, "wsock32.lib")
	#include <winsock.h>
#else
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <net/if_arp.h>
	#include <net/if.h>
	#include <pthread.h>
	#include <sys/sem.h>
	#include <semaphore.h>
	#include <sys/ioctl.h>
#endif

//#define SHOWMD5
//#define CHECKMD5

#define SERVERUDPPORT 8386
#define VSTPPORT      8385
#define PLAYERUDPPORT 6789
#define PLAYERPORT    31016

#define REQUEST_SERVER_MSG "GetMainServer Client:X="
#define SERVICEMD5    "ODMwQTEzMzk3MjQwODhBMjY5REZCN0Y1NTczMzUyQjg="

#ifdef __cplusplus
extern "C" {
#endif

bool GetMacAddr  (char *CardMacAddr);        // 得到网卡号
bool GetLocalIP  (char *IP);                 // 得到IP地址
void SetBroadCast(int socket);               // 设置广播
int  SetBlocking (int socket);               // 设置非阴塞模式

void SetSocketTimeout(int socket, int msec); // 设置超时
void SetSocketSendBuf(int socket, int size); // 设置发送缓冲大小
char *GetBroadCastIP (char *BCastIP);        // 得到广播地址
int BroadCastStr     (int udpfd, char *msg); // 广播数据
int UdpSendStr       (char *host, int udport, char *msg);
void ClientLogin     (bool login);
bool FindServerHost  (char *ServerIP, const char* BinFile);
int CreateStreamUdp  (int port);
int GetStreamServer  (int udpfd, char *hosts);

#ifdef __cplusplus
}
#endif

