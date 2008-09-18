/*==============================================================================
   T h e   K T V L i n u x   P r o j e c t
 -------------------------------------------------------------------------------
   Filename   : selected.h
   Author(s)  : Silicon
   Copyright  : Silicon

   һЩ��չ���ַ��������������Լ������Ҫ�ĺ���
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

bool GetMacAddr  (char *CardMacAddr);        // �õ�������
bool GetLocalIP  (char *IP);                 // �õ�IP��ַ
void SetBroadCast(int socket);               // ���ù㲥
int  SetBlocking (int socket);               // ���÷�����ģʽ

void SetSocketTimeout(int socket, int msec); // ���ó�ʱ
void SetSocketSendBuf(int socket, int size); // ���÷��ͻ����С
char *GetBroadCastIP (char *BCastIP);        // �õ��㲥��ַ
int BroadCastStr     (int udpfd, char *msg); // �㲥����
int UdpSendStr       (char *host, int udport, char *msg);
void ClientLogin     (bool login);
bool FindServerHost  (char *ServerIP, const char* BinFile);
int CreateStreamUdp  (int port);
int GetStreamServer  (int udpfd, char *hosts);

#ifdef __cplusplus
}
#endif

