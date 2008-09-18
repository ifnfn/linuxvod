#ifndef SONGDATA_H
#define SONGDATA_H

#include <stdbool.h>
#include "vstp/vstp.h"
#include "selected.h"

#define mptPause    0
#define mptContinue 1
#define mptSong     2
#define mptMusic    3

int CreateUdpBind(int port);                                         // 创建 UDP 连接
int AppendClientToList(struct sockaddr clientaddr);                  // 增加工作站
int AppendIPToList(const char *ip);                                  //

int RecvUdpBuf(char *msg, int MaxSize, struct sockaddr *ClientAddr); // 接受 UDP 上的数据
inline void CloseUdpSocket(void);                                    // 关闭 UDP 连接
bool ProcessRecv(PlayCmd cmd, char *param, struct sockaddr addr_sin);// 处理从网络中收到的数据
void BroadcastSongRec(char *cmd, SelectSongNode *rec);               // 广播歌曲操作
void BroadcastDebar(char *cmd, SelectSongNode *rec, struct sockaddr_in *debaraddr);
                                                                     // 广播歌操作，排除debaraddr
void SendToBroadCast(char *msg);                                     // 广播消息
bool EnterPressedNonBlocking(char *cmd);                             // 广播字符串
inline void PlayerSendStr(char *msg, struct sockaddr *addr_sin);     // 向一个客户发送字符串
inline void PlayerSendInt(int i, struct sockaddr *addr_sin);         // 向一个客户发送整数

inline void PlayerSendPrompt(int id, struct sockaddr *addr_sin);     // 向一个客户发送字符串

#endif
