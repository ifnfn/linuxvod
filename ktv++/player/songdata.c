#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/timeb.h>
#include <pthread.h>

#include "songdata.h"
#include "config.h"
#include "osnet.h"
#include "selected.h"

struct sockaddr_in clientaddr_list[10];  // ���ӵ����ŵĹ���վ�б�
static int ClientNum = 0;                // ���ӵ����ŵĹ���վ�б�����

static int udpbindsockfd = 0;

static char *Prompt[4] = {"��ͣ����?0", "��������", "ԭ��ģʽ", "����OKģʽ"};
//static char *Prompt[4] = {"��ͣ����", "��������", "����OKģʽ", "ԭ��ģʽ"};

int CreateUdpBind(int port )
{
	struct sockaddr_in addr_sin;
	int AddrLen = sizeof(struct sockaddr_in);
	bzero(&addr_sin, sizeof(addr_sin));
	addr_sin.sin_family = AF_INET;
	addr_sin.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_sin.sin_port = htons(port);
	if ((udpbindsockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		perror("SOCKET call failed");
		return 0;
	}
	if(bind(udpbindsockfd, (struct sockaddr *)&addr_sin, AddrLen) < 0){
		perror("BIND Call failed");
		return 0;
	}
	return udpbindsockfd;
}

int AppendClientToList(struct sockaddr clientaddr)
{
	int i;
	struct sockaddr_in *tmp = (struct sockaddr_in *)(&clientaddr);
	for (i=0; i<ClientNum; i++){
		if (!memcmp(&clientaddr_list[i].sin_addr, &tmp->sin_addr, sizeof(tmp->sin_addr)))
			return 0;
	}
	memcpy(clientaddr_list + ClientNum, &clientaddr, sizeof(struct sockaddr_in));
	clientaddr_list[ClientNum].sin_port = htons(PLAYPORT);
	ClientNum++;
//	printf("Add (%d) %s.\n", ClientNum, inet_ntoa(tmp->sin_addr));
	return 1;
}

int AppendIPToList(const char *ip)
{
	struct sockaddr_in addr;
	bzero(&addr, sizeof(struct sockaddr_in));
	addr.sin_addr.s_addr = inet_addr(ip);
	struct sockaddr *x = (struct sockaddr*)&addr;
	return AppendClientToList(*x);
}

int RecvUdpBuf(char *msg, int MaxSize, struct sockaddr *ClientAddr)
{
	int addrlen = sizeof(struct sockaddr_in);
	int len = recvfrom(udpbindsockfd, msg, MaxSize, 0, ClientAddr ,(socklen_t*)&addrlen);
	if ((len == -1) && (errno != EAGAIN)){
		fprintf(stderr, "errno %d ", errno);
		perror("Recvfrom call failed");
		return 0;
	} else
		msg[len] = '\0';
	AppendClientToList(*ClientAddr);
	return len;
}

inline void CloseUdpSocket(void)
{
	close(udpbindsockfd);
}

static void SendSongRec(const char *cmd, SelectSongNode *rec, struct sockaddr *addr_sin)
{
	char *recstr = SelectSongNodeToStr(cmd, rec);
	sendto(udpbindsockfd, recstr, strlen(recstr), 0, addr_sin, sizeof(struct sockaddr));
	free(recstr);
}

bool ProcessRecv(PlayCmd cmd, char *param, struct sockaddr addr_sin)   // ������������յ�������
{
	int i = 0;
	SelectSongNode rec;
	switch (cmd) {
		case pcAddSong:   // addsong
			StrToSelectSongNode(param, &rec);
			if (SelectedList.count < SongMaxNumber) {
				AddSongToList(&rec, true); // �����µĸ���
				BroadcastDebar(ADDSONG, &rec, (struct sockaddr_in*)&addr_sin);
				SendSongRec(ADDSONG, &rec, &addr_sin); // �������Ӹ����ĵ㲥����
			}
			else{
				char msg[100];
				sprintf(msg, "msg?���������%d��", SongMaxNumber);
				sendto(udpbindsockfd, msg, strlen(msg), 0, (struct sockaddr *)&addr_sin, sizeof(addr_sin));
			}
			return true;
		case pcDelSong:   // delsong
			StrToSelectSongNode(param, &rec);
			DelSongFromList(&rec);
			BroadcastSongRec(DELSONG, &rec);
			return true;
		case pcFirstSong: // firstsong
			StrToSelectSongNode(param, &rec);
			FirstSong(&rec, 1);
			BroadcastSongRec(FIRSTSONG, &rec);
			return true;
		case pcListSong:  // listsong
			for(i=0;i<SelectedList.count;i++)
				SendSongRec(NULL, SelectedList.items+i, &addr_sin);
			sendto(udpbindsockfd, "end", strlen("end"), 0, (struct sockaddr *)&addr_sin, sizeof(addr_sin));
			return true;
		default:
			return false;
	}
}

void BroadcastSongRec(const char *cmd, SelectSongNode *rec)
{
	char *recstr = SelectSongNodeToStr(cmd, rec);
	int i;
	for(i=0;i<ClientNum; i++)
		sendto(udpbindsockfd, recstr, strlen(recstr), 0, \
			(struct sockaddr *)(clientaddr_list+i), sizeof(struct sockaddr_in));
	free(recstr);
}

void BroadcastDebar(const char *cmd, SelectSongNode *rec, struct sockaddr_in *debaraddr) // �㲥��������ų�debaraddr
{
	char *recstr = SelectSongNodeToStr(cmd, rec);
	int i;
	for(i=0;i<ClientNum; i++) {
		if (memcmp(&clientaddr_list[i].sin_addr, &debaraddr->sin_addr, sizeof(debaraddr->sin_addr)))
			sendto(udpbindsockfd, recstr, strlen(recstr), 0, \
				(struct sockaddr *)(clientaddr_list+i), sizeof(struct sockaddr_in));
	}
	free(recstr);
}

void SendToBroadCast(char *msg) // ���͹㲥����
{
	int i;
	for(i=0;i<ClientNum;i++)
		sendto(udpbindsockfd, msg, strlen(msg), 0, \
			(struct sockaddr *)(clientaddr_list+i), sizeof(struct sockaddr_in));
}

//	"��ͣ����", "��������", "ԭ��ģʽ", "����OKģʽ"};
inline void PlayerSendPrompt(int id, struct sockaddr *addr_sin)
{
	if (id >= 0 && id < 4)
		sendto(udpbindsockfd, Prompt[id], strlen(Prompt[id]), 0, addr_sin, sizeof(struct sockaddr));
}

inline void PlayerSendStr(char *msg, struct sockaddr *addr_sin) // ��һ���ͻ������ַ���
{
	sendto(udpbindsockfd, msg, strlen(msg), 0, addr_sin, sizeof(struct sockaddr));
}

inline void PlayerSendInt(int i, struct sockaddr *addr_sin)     // ��һ���ͻ���������
{
	char msg[20];
	sprintf(msg, "%d", i);
	sendto(udpbindsockfd, msg, strlen(msg), 0, addr_sin, sizeof(struct sockaddr));
}
