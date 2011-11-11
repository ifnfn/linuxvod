#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <net/if_arp.h>
#include <net/if.h>


#include "serial.h"

void init_daemon(void)
{
	int pid;
	int i;
	if (pid = fork())
		exit(0); // end parent
	else if (pid < 0)
		exit(1);
	setsid();
	if (pid=fork())
		exit(0);
	else if (pid<0)
		exit(1);
	for (i=0;i<NOFILE;i++)
		close(i);
	chdir("/tmp");
	umask(0);
	return;
}


static struct sockaddr_in PlayAddr;
static sckfd;

int CreateSocket(int port)
{
	memset(&PlayAddr, 0, sizeof(PlayAddr));
	PlayAddr.sin_family = AF_INET;
	PlayAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	PlayAddr.sin_port = htons(port);
	if( (sckfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))== -1){
		perror("Don't get a socket.\n");
		return -1;
	}
//	SetSocketTimeout(sckfd, 2000);
	return 0;
}

int SendStr(char *cmd)
{
	sendto(sckfd, cmd, strlen(cmd), 0, (struct sockaddr *)&PlayAddr, sizeof(PlayAddr));
}

int main(int argc, char **argv)
{
	int portid;
	init_daemon();
	portid = OpenComm(0, 19200, 8, "1", 'N');
	if (portid == -1){
		fprintf (stderr, "Make sure /dev/ttyS%d not in use or you have enough privilege.\n\n", 0);
		exit(-1);
	}

	CreateSocket(6789);
	int nread;
	char buf[512];
	while (1) {
		nread = SyncReadComm(portid, buf, 512);
		if (nread > 0) {
			 buf[nread] = '\0';
			if (buf[0] == '1')
				SendStr("audio=music");
			else if (buf[0] == '0')
			SendStr("audio=sound");
//			if ((buf[0] == '1') || (buf[0] == '0'))
//				printf("%s\n", buf);
//			fflush(stdout);
		}
	}
	CloseComm(portid);
	close(sckfd);
	return 0;
}

