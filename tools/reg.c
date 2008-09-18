#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crypt/hd.h"
#include "crypt/des.h"

/*
 * reg -f <standfile> <newecryptfile> [harddisk]
 * reg -x <standfile> <newfile> <regcode>
 * reg -c <oldecryptfile> <oldharddisk> <newecryptfile> <regcode>
 */
int main(int argc, char **argv)
{
#ifdef REGCODE
	if (argc > 1) {
		char despwd[33];
		CreateRegCode(argv[1], despwd);
		printf("%s->%s\n", argv[1], despwd);
	}
	else
		printf("%s <regcode>\n", argv[0]);

#else
	char despwd[33], play2des[100], binplay[100], devhda[100];
	if (argc == 1)
	{ // 没有参数
		if (GetDesPwd(b64str(DEVHDA, devhda), despwd)) {
			if (DesDecryptFile(b64str(PLAYDES, play2des), b64str(BINPLAY, binplay), despwd) == false)
				return -1;
			char cmd[512] = "chmod +x ";
			strcat(cmd, binplay);
			system(cmd);

			strcpy(cmd, binplay);
			strcat(cmd, "& 1> /dev/tty3 2> /dev/tty3");
			system(cmd);

//			strcpy(cmd, "rm -f ");
//			strcat(cmd, binplay);
//			system(cmd);
		}
	}
	else if (argc == 3) {
		if (strcmp(argv[1], "-k") == 0) {
			char publickey[33];
			if (GetPublicKey(argv[2], publickey)) {
				printf("%s\n", publickey);
				return 0;
			}
		}
	}
	else if (argc == 5 || argc == 4) {
		if (strcmp(argv[1], "-f") == 0) {
			bool ok = false;
			if (argc == 4)
				ok = GetDesPwd(b64str(DEVHDA, devhda), despwd);
			else
				ok = GetDesPwd(argv[4], despwd);
			if (ok)
				DesEncryptFile(argv[2], argv[3], despwd);
		}
		else if (strcmp(argv[1], "-x") == 0)
			DesEncryptFile(argv[2], argv[3], argv[4]);
	}
	else if (argc == 6)
	{
		if (strcmp(argv[1], "-c") == 0) {
			if (GetDesPwd(argv[3], despwd)){
				char newdespwd[33];
				CreateRegCode(argv[5], newdespwd);
//				printf("Old = %s\nNew = %s\n", despwd, newdespwd);
				if (strcmp(newdespwd, despwd))
					DecAndEncFile(argv[2], despwd, argv[4], newdespwd);
				else {
					FILE *fp = fopen(argv[4], "wb");
					fprintf(fp, "Fuck You, Hahaha!\n");
					fclose(fp);
				}
			}
		}
	}
#endif
	return 0;
}

