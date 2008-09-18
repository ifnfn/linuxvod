#ifndef __VSTPSERVER__
#define __VSTPSERVER__

#define MAXCHILD 5

extern char localip[16];

int CreateUDPServer(int MaxNum, int port);
#endif
