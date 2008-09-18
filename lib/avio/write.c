#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "avio.h"

int main(int argc, char **argv)
{
	av_register_all();
	URLContext *x = NULL;
	if (url_open(&x, argv[1], URL_WRONLY) >= 0)
	{
		url_write(x, argv[2], strlen(argv[2]));
//		url_close(x);
	} else
		printf("open error.\n");
	return 0;
}

