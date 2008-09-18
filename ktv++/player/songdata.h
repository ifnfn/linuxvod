#ifndef SONGDATA_H
#define SONGDATA_H

#include <stdbool.h>
#include "vstp/vstp.h"
#include "selected.h"
#include "playcontrol.h"

#define mptPause    0
#define mptContinue 1
#define mptSong     2
#define mptMusic    3
#define mptMute     4
#define mptClean    5

int CreateUdpBind(int port);
int AppendClientToList       (struct sockaddr clientaddr);            // ���ӹ���վ
int AppendIPToList           (const char *ip);
inline void CloseUdpSocket   (void);                                  // �ر� UDP ����
void BroadcastSongRec        (const char *cmd, SelectSongNode *rec);  // �㲥��������
void SendToBroadCast         (char *msg);                             // �㲥��Ϣ
inline void PlayerSendPrompt (int id);
int process(INFO *pInfo, const char *cmd, const char *param, int sockfd);
#endif
