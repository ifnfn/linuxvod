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
int AppendClientToList       (struct sockaddr clientaddr);            // 增加工作站
int AppendIPToList           (const char *ip);
inline void CloseUdpSocket   (void);                                  // 关闭 UDP 连接
void BroadcastSongRec        (const char *cmd, SelectSongNode *rec);  // 广播歌曲操作
void SendToBroadCast         (char *msg);                             // 广播消息
inline void PlayerSendPrompt (int id);
int process(INFO *pInfo, const char *cmd, const char *param, int sockfd);
#endif
