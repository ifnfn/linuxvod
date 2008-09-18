#ifndef SONGDATA_H
#define SONGDATA_H

#include <stdbool.h>
#include "vstp/vstp.h"
#include "selected.h"

#define mptPause    0
#define mptContinue 1
#define mptSong     2
#define mptMusic    3

int CreateUdpBind(int port);                                         // ���� UDP ����
int AppendClientToList(struct sockaddr clientaddr);                  // ���ӹ���վ
int AppendIPToList(const char *ip);                                  //

int RecvUdpBuf(char *msg, int MaxSize, struct sockaddr *ClientAddr); // ���� UDP �ϵ�����
inline void CloseUdpSocket(void);                                    // �ر� UDP ����
bool ProcessRecv(PlayCmd cmd, char *param, struct sockaddr addr_sin);// ������������յ�������
void BroadcastSongRec(char *cmd, SelectSongNode *rec);               // �㲥��������
void BroadcastDebar(char *cmd, SelectSongNode *rec, struct sockaddr_in *debaraddr);
                                                                     // �㲥��������ų�debaraddr
void SendToBroadCast(char *msg);                                     // �㲥��Ϣ
bool EnterPressedNonBlocking(char *cmd);                             // �㲥�ַ���
inline void PlayerSendStr(char *msg, struct sockaddr *addr_sin);     // ��һ���ͻ������ַ���
inline void PlayerSendInt(int i, struct sockaddr *addr_sin);         // ��һ���ͻ���������

inline void PlayerSendPrompt(int id, struct sockaddr *addr_sin);     // ��һ���ͻ������ַ���

#endif
