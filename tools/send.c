#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#include "osnet.h"

int main(int argc, char** argv)
{
	char sendmsg[1024] = "";
	fgets(sendmsg, 1023, stdin);
//	return UdpSendStr("*", 31016, sendmsg);
	return UdpSendStr("*", 6789, sendmsg);
}

