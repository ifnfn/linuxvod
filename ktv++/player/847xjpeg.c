#include <stdio.h>

#define ALLOW_OS_CODE 1
#include "showjpeg.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("%s <jpegfile>\n", argv[0]);
		return -1;
	}
	ShowJpegNew(argv[1], argc == 3);
	return 0;
}
