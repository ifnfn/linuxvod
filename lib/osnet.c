#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#include "osnet.h"
#include "crypt/md5.h"
#include "crypt/base64.h"

char BroadCastIP[16] = "255.255.255.255";

#define MAXINTERFACES 16

#ifdef WIN32
typedef struct tagASTAT
{
	ADAPTER_STATUS adapt;
	NAME_BUFFER
	NameBuff [30];
} ASTAT, *PASTAT;

bool GetMacAddr(char *CardMacAddr)
{
	ASTAT Adapter;
	// 定义一个存放返回网卡信息的变量
	// 输入参数:lana_num为网卡编号,一般地,从0开始,但在Windows 2000中并不一定是连续分配的
	NCB ncb;
	UCHAR uRetCode;
	memset( &ncb, 0, sizeof(ncb) );
	ncb.ncb_command = NCBRESET;
	ncb.ncb_lana_num = 0; // 指定网卡号
	// 首先对选定的网卡发送一个NCBRESET命令,以便进行初始化
	uRetCode = Netbios( &ncb );
	memset( &ncb, 0, sizeof(ncb) );
	ncb.ncb_command = NCBASTAT;
	ncb.ncb_lana_num = 0; // 指定网卡号
	strcpy((char *)ncb.ncb_callname,"*" );
	ncb.ncb_buffer = (unsigned char *) &Adapter; // 指定返回的信息存放的变量
	ncb.ncb_length = sizeof(Adapter);
	// 接着,可以发送NCBASTAT命令以获取网卡的信息
	uRetCode = Netbios( &ncb );
	if ( uRetCode == 0 )
	{
		// 把网卡MAC地址格式化成常用的16进制形式,如0010-A4E4-5802
		sprintf(CardMacAddr, "%02x:%02x:%02x:%02x:%02x:%02x",
			Adapter.adapt.adapter_address[0],
			Adapter.adapt.adapter_address[1],
			Adapter.adapt.adapter_address[2],
			Adapter.adapt.adapter_address[3],
			Adapter.adapt.adapter_address[4],
			Adapter.adapt.adapter_address[5]);
	}
	return uRetCode > 0;
}

bool GetLocalIP(char *IP)
{
}
#else
bool GetMacAddr(char *CardMacAddr)
{
	if (CardMacAddr == NULL) return 0;
	register int fd, intrface;
	struct ifreq buf[MAXINTERFACES];
	struct ifconf ifc;
	if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) >= 0) {
		ifc.ifc_len = sizeof buf;
		ifc.ifc_buf = (caddr_t) buf;
		if (!ioctl (fd, SIOCGIFCONF, (char *) &ifc)) {
			intrface = ifc.ifc_len / sizeof (struct ifreq);
			while (intrface-- > 0) {
				if (!(ioctl (fd, SIOCGIFHWADDR, (char *) &buf[intrface]))){
					sprintf(CardMacAddr, "%02x:%02x:%02x:%02x:%02x:%02x",
							(unsigned char)buf[intrface].ifr_hwaddr.sa_data[0],
							(unsigned char)buf[intrface].ifr_hwaddr.sa_data[1],
							(unsigned char)buf[intrface].ifr_hwaddr.sa_data[2],
							(unsigned char)buf[intrface].ifr_hwaddr.sa_data[3],
							(unsigned char)buf[intrface].ifr_hwaddr.sa_data[4],
							(unsigned char)buf[intrface].ifr_hwaddr.sa_data[5]);
					close(fd);
					return true;
				}
			}
		}
	}
	close (fd);
	return false;
}

bool GetLocalIP(char *IP)
{
	if (IP == NULL) return 0;
	register int fd, intrface;
	struct ifreq buf[MAXINTERFACES];
	struct ifconf ifc;
	if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) >= 0) {
		ifc.ifc_len = sizeof buf;
		ifc.ifc_buf = (caddr_t) buf;
		if (!ioctl (fd, SIOCGIFCONF, (char *) &ifc)) {
			intrface = ifc.ifc_len / sizeof (struct ifreq);
			while (intrface-- > 0) {
				if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface]))) {
					strcpy(IP, inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));
					close(fd);
					return true;
				}
			}
		}
	}
	close (fd);
	return false;
}
#endif

void SetBroadCast(int socket)
{
	int n = 1; // 设定常量，用于打开广播模式
#ifdef WIN32
	setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char *)&n, sizeof(n));
#else
	setsockopt(socket, SOL_SOCKET, SO_BROADCAST, &n, sizeof(n));
#endif
}

void SetSocketTimeout(int socket, int msec) // 设置超时
{
#ifdef WIN32
	int tv = msec;
	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
#else
	struct timeval timeout;
	timeout.tv_sec = msec / 1000;
	timeout.tv_usec= (msec % 1000) * 1000;
	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
#endif
}

void SetSocketSendBuf(int socket, int size)
{
	int n = size; // 设定常量，用于打开广播模式
#ifdef WIN32
	int tv = size;
	setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char *)&n, sizeof(n));
#else
	setsockopt(socket, SOL_SOCKET, SO_SNDBUF, &n, sizeof(n));
#endif
}

char *GetBroadCastIP(char *BCastIP)
{
	GetLocalIP(BroadCastIP);
	char *p = strrchr(BroadCastIP, '.');
	if (p)
		strcpy(p+1, "255");
	else
		strcpy(BroadCastIP, "192.168.0.255");
	if (BCastIP)
		strcpy(BCastIP, BroadCastIP);
	return BroadCastIP;
}

int BroadCastStr(int udpfd, char *msg)
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));

	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = inet_addr(BroadCastIP);
	addr.sin_port        = htons(SERVERUDPPORT);
	return sendto(udpfd, msg, strlen(msg), 0, (struct sockaddr*)&addr, sizeof(addr));
}

int UdpSendStr(char *host, int udport, char *msg)
{
	if (!host) return 0;

	int udpfd;
	udpfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpfd < 0){
		perror("Create socket error.\n");
		return -1;
	}
	SetBroadCast(udpfd); // 设定常量，用于打开广播模式

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
	if (host[0] == '*')
		addr.sin_addr.s_addr = inet_addr(GetBroadCastIP(NULL));
	else
		addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_port = htons(udport);
	int n = sendto(udpfd, msg, strlen(msg), 0, (struct sockaddr*)&addr, sizeof(addr));
	close(udpfd);
	return n;
}

void ClientLogin(bool login)
{
	char msg[100], MAC[100];
	GetMacAddr(MAC);
	if (login)
		sprintf(msg, "Status Client:%s=Open", MAC);
	else
		sprintf(msg, "Status Client:%s=Close", MAC);
	UdpSendStr(BroadCastIP, SERVERUDPPORT, msg);
}

static void GetBinMD5(char *command, const char *BinFile)
{
	MD5_CTX context;
	unsigned char Buffer[512];
	int len = 0;
	MD5Init(&context);
	FILE* fp = fopen(BinFile, "rb");
	if (!fp)  return;

	while (!feof(fp)) {
		len = fread(Buffer, 1, 511, fp);
		MD5Update(&context, Buffer, len);
	}
	fclose(fp);
	unsigned char digest[33];
	char md5[33];
	MD5Final(digest, &context);
	MDPrint(digest, md5, 16);
#ifdef SHOWMD5	
	printf("%s: %s\n", BinFile, md5);
#endif
	strcat(command, md5);
}

bool FindServerHost(char *ServerIP, const char* BinFile)
{
	char msg[100];
	struct sockaddr_in addr;
	ServerIP[0] = '\0';
	int udpfd;

#ifdef WIN32
	static bool startup=false;
	if (!startup){
		static WSADATA wsaData;
		if (WSAStartup(0x202, &wsaData)) {
			printf("Winsock 2 initialization failed.\n");
			WSACleanup();
			exit(0);
		}
		startup = true;
	}
#endif
	GetBroadCastIP(BroadCastIP); // 得到广播网址
	udpfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpfd < 0){
		perror("vstp->udpsvrfd Error.\n");
		return false;
	};

	SetBroadCast(udpfd);          // 设置广播
	SetSocketTimeout(udpfd, 3000);// 设置超时
	strcpy(msg, REQUEST_SERVER_MSG);

	GetBinMD5(msg, BinFile);
	BroadCastStr(udpfd, msg);
#ifdef WIN32
	int len, Addrlen;
	Addrlen = sizeof(struct sockaddr_in);
	memset(&addr, 0, sizeof(struct sockaddr_in));
	len = recvfrom(udpfd, msg, 99, 0 , (struct sockaddr *)&addr, &Addrlen);
	strcpy(ServerIP, inet_ntoa(addr.sin_addr));
	closesocket(udpfd);
#else
	fd_set fdread;
	int ret, len=0, Addrlen;

	FD_ZERO(&fdread);
	FD_SET(udpfd, &fdread);

	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec= 0;
	if ((ret = select(udpfd + 1, &fdread, NULL, NULL, &tv)) == -1){
		perror("select socket error.\n");
		goto exit;
	}
	if (FD_ISSET(udpfd, &fdread)) {
		Addrlen = sizeof(struct sockaddr_in);
		memset(&addr, 0, sizeof(struct sockaddr_in));
		len=recvfrom(udpfd, msg, 99, 0, (struct sockaddr *)&addr, (socklen_t*)&Addrlen);
		if (len>=0) msg[len] = 0;
#ifdef SHOWMD5		
		printf("msg=%s\n", msg);
#endif

#ifdef CHECKMD5
		char tmpbuf[100];
		memset(tmpbuf, 0, 100);
		from64tobits(tmpbuf, SERVICEMD5);
//		printf("msg=%s, tmpbuf=%s\n", msg, tmpbuf);
		if ( !strcmp(msg, tmpbuf) ) {
			strcpy(ServerIP, inet_ntoa(addr.sin_addr));
//			printf("Found Server: %s\n", ServerIP);
		}
		else {			
			ServerIP[0] = '\0';
			len = 0;
		}
#else
		strcpy(ServerIP, inet_ntoa(addr.sin_addr));
#endif
	}
exit:
	close(udpfd);
#endif
	return len > 0;
}

int SetBlocking(int socket) // 设置非阻塞模式
{
#ifndef WIN32
	long flags = fcntl(socket, F_GETFL);
	flags |= O_NONBLOCK;
	return fcntl(socket, F_SETFL, flags);
#endif
}

int CreateStreamUdp(int port)
{   
	struct sockaddr_in addr;
	int udpfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpfd < 0) {
		perror("Create socket error.\n");
		return 0;
	}
	SetBroadCast(udpfd);
	char *BroadCastIP = GetBroadCastIP(NULL);
	char sendmsg[50] = "WHO";
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = inet_addr(BroadCastIP);
	addr.sin_port        = htons(port);
	sendto(udpfd, sendmsg, strlen(sendmsg), 0, (struct sockaddr*)&addr, sizeof(addr));
	return udpfd;
}

int GetStreamServer(int udpfd, char *hosts)
{
	int count = 0;
	fd_set fdread;
	int ret, Addrlen;
	struct timeval tv;
	struct sockaddr_in addr;

	FD_ZERO(&fdread);
	FD_SET(udpfd, &fdread);

	tv.tv_sec = 1;
	tv.tv_usec= 0;
	if ((ret = select(udpfd + 1, &fdread, NULL, NULL, &tv)) == -1){
		perror("select socket error.\n");
		return 0;
	}
	if (FD_ISSET(udpfd, &fdread)) {
		Addrlen = sizeof(struct sockaddr_in);
		memset(&addr, 0, sizeof(struct sockaddr_in));
		char msg[1024] = "";
		int len=recvfrom(udpfd,msg,1024,0,(struct sockaddr *)&addr,(socklen_t*)&Addrlen);
		if (len>= 0) msg[len]='\0';
		strcpy(hosts, inet_ntoa(addr.sin_addr));
		count = 1;
	}
	return count;
}
