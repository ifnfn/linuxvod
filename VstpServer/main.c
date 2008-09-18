#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <sys/timeb.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>

#include <assert.h>

#include <sqlite.h>

#include "vstpserver.h"
#include "osnet.h"
#include "strext.h"

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

const char *usage="usage: %s [-d] [-h]\n\t-d: create daemon\n\t-h: help\n";

int main(int argc ,char **argv)
{
	int ch;
	GetLocalIP(localip);
	while ((ch = getopt(argc, argv, "dh"))!= -1)
	{
		switch (ch)
		{
			case 'd':
				init_daemon();
				break;
			case 'h':
				printf(usage, argv[0]);
				exit(0);
				break;
		}
	}
	int n = fork();
	if (n == 0) {
		int mainfd = CreateUDPServer(MAXCHILD, VSTPPORT);
		if (mainfd < 0) {
			perror("Bind MainServer error.\n");
			exit(1);
		}
	}
	wait();
	return 0;
}

