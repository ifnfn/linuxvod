#define ALLOW_OS_CODE
#include "rmexternalapi.h"

#include <stdio.h>
#include <stdlib.h>

#include "osd.h"

int main(int argc, char **argv)
{
	OSD_Create(0);
	OSD_CleanScreen();
	OSD_Quit();

	return 0;
}
