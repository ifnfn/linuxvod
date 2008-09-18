#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/reboot.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>

#include "ghttpd.h"
#include "../lib/crypt/aes.h"

static void init_reboot(unsigned long magic)
{
	pid_t pid;
	pid = vfork();
	if (pid == 0) { /* child */
		reboot(magic);
		_exit(0);
	}
	waitpid(pid, NULL, 0);
}

extern char assocNames[][2][100];
static const char *getmimetype(const char *filename);

static int SendHttpHeader(int sockfd, off_t size, const char *filename)
{
	char         tempstring[1024];
	struct stat  buf;

	if (send(sockfd, "HTTP/1.1 200 OK\n", 16, 0)==-1)
		return -1;
	if (send(sockfd, "Server: "SERVERNAME"\n", strlen("Server: "SERVERNAME"\n"), 0)==-1)
		return -1;

	if (size == 0) {
		if ( stat(filename, &buf) == 0)
			size = buf.st_size;
	}

	sprintf(tempstring, "Content-Length: %d\n", size);
	if (send(sockfd, tempstring, strlen(tempstring), 0)==-1)
		return -1;
	
	sprintf(tempstring, "Content-Type: %s\n\n", getmimetype(filename));
	if (send(sockfd, tempstring, strlen(tempstring), 0)==-1)
		return -1;

	return 0;
}

static int GetFile(int sockfd, const char *filename)
{
	FILE*   in;
	int     err = -1;
	char    tempstring[8192];
	int     numbytes=0;

	if ((in = fopen(filename, "rb"))==NULL)
		return -1;

	if (SendHttpHeader(sockfd, 0, filename) == -1)
		goto faliend;

	fseek(in, 0, SEEK_SET);
	while(!feof(in)) {
		numbytes = fread(tempstring, 1, 8192, in);
		if (send(sockfd, tempstring, numbytes, 0)==-1)
			goto faliend;
	}
	err = 0;

faliend:
	fclose(in);

	return err;
}

static int PostFile(int sockfd, const char *filename)
{
	unsigned int   loop = 0, flag = 0;
	int            numbytes=0;
	int            tno;
	fd_set         rfds;
	int            len, fd_max, ret;
	struct timeval tv;
	FILE*          out;
	char           tempdata[8192];

	if ((out = fopen(filename, "wb"))==NULL)
		return -1;

	for (;;) {
		fd_max = sockfd;
		FD_ZERO(&rfds);
		FD_SET(sockfd, &rfds);
		tv.tv_sec = 0;
		tv.tv_usec = 100 * 1000;
		ret = select(fd_max + 1, &rfds, NULL, NULL, &tv);
		if (ret > 0 && FD_ISSET(sockfd, &rfds)) 
		{
			len = recv(sockfd, tempdata, 4096, 0);
			if (len <= 0)
				break;
			else
				fwrite(tempdata, 1, len, out);
		} 
		else if (ret < 0)
			break;
	}
	fclose(out);

	return 0;
}

static char find_code[256] = "";
static int find_count;

static int find_fn(const char *file, const struct stat *sb, int flag)
{
	if (flag == FTW_F) {
		char code[100];
		char *p = rindex(file, '/');
		if (p) 
			p++; 
		else 
			p = (char *)file;

		if (strncmp(p, find_code, strlen(find_code)) == 0) {
			strncpy(find_code, file, 255);
			find_count ++;

			return 1;
		}
	}
	return 0;
}

static char *GetFileByName(const char *path, const char *code)
{
	find_count = 0;
	strncpy(find_code, code, 255);
	ftw(path, find_fn, 50);

	if (find_count > 0)
		return find_code;

	return NULL;
}

static int DecodeFile(int sockfd, const char *param, t_vhost *thehost)
{
	int    err = -1;
	char   *filename;
	int    numbytes;
	char   tempstring[4096];
	struct AesFile *aes = NULL;
	char  *code, *key, *name, *passwd;
	char  *tmp, *p;

	code = key = name = passwd = NULL;
	tmp = AesDecryptAndBase64DefaultPwd(param);
//	printf("tmp=%s\n", tmp);
	p = strtok(tmp, "&");

	while (p) {
		char *value = strchr(p, '=');
		if (value) {
			*value= '\0';
			value++;
		}
		if (strcmp(p, "code") == 0)        code   = strdup(value);
		else if (strcmp(p, "key") == 0)    key    = strdup(value);
		else if (strcmp(p, "name") == 0)   name   = strdup(value);
		else if (strcmp(p, "passwd") == 0) passwd = strdup(value);

		p = strtok(NULL, "&");
	}

	printf("code=%s, key=%s, name=%s, passwd=%s\n", code, key, name, passwd);
	filename = GetFileByName(thehost->VIDEODIR, code);
	printf("filename=%s\n", filename);
	if (filename == NULL)
		goto faliend;
	aes = AesOpenFile(filename, passwd, 1);
	if (aes == NULL) {
		printf("aes == NULL\n");
		goto faliend;
	}

	if (SendHttpHeader(sockfd, GetAesFileSize(aes), filename) == -1)
		goto faliend;

	while(1) {
		numbytes = AesReadFile(aes, tempstring, 4096);
		if (numbytes <= 0)
			break;
		if (send(sockfd, tempstring, numbytes, 0)==-1)
			goto faliend;
	}
	err = 0;
faliend:
	if (tmp)    free(tmp);
	if (code)   free(code);
	if (key)    free(key);
	if (name)   free(name);
	if (passwd) free(passwd);
	if (aes)    AesCloseFile(aes);

	return err;
}

static void ShowDirectory(char *directory, int sockfd, t_vhost *thehost)
{
	struct dirent **namelist;
	int n, loop;
	unsigned long size=0;
	FILE *in;
	char tempstring[255];
	char dirheader[2048];
	const char dirfooter[] = "</TABLE><hr><center>\
	      	   <font size=-1>This page is being provided to you by ghttpd<br>\
 		   <A HREF=\"http://members.xoom.com/gaztek\">http://members.xoom.com/gaztek</a>\
		   </TD></TR></TABLE></BODY></HTML>\n";

	memset(dirheader, 0, 2048);

	sprintf(dirheader, "HTTP/1.1 200 OK\nServer: "SERVERNAME"\nContent-Type: text/html\n\n<HTML><BODY bgcolor=\"#33CCFF\"><center><TABLE BORDER=3 COLS=1 WIDTH=\"75%\" BGCOLOR=\"#66FFFF\"><tr><td><center><h2><u>Directory listing for %s</u></h2></center><TABLE BORDER=3 COLS=2 WIDTH=100% BGCOLOR=\"#FFFFFF\"><tr><td><b><center>Filename</center></b></td><td><b><center>Size (bytes)</center></b></td></tr>", directory+strlen(thehost->DOCUMENTROOT));

	if (send(sockfd, dirheader, strlen(dirheader), 0) == -1)
		return;

	n = scandir(directory, &namelist, 0, alphasort);
	directory += strlen(thehost->DOCUMENTROOT);

	if (n < 0)
    		perror("scandir");
	else
    		for(loop=0; loop<n; loop++)
    		{
    			sprintf(tempstring, "%s%s/%s", thehost->DOCUMENTROOT, directory, namelist[loop]->d_name);
			size = get_file_size(tempstring);
			if (isDirectory(tempstring)) size=0;
    			sprintf(tempstring, "<tr><td><center><A HREF=\"%s/%s\">%s</a></center></td><td><center>%d</center></td></tr>\n", directory,namelist[loop]->d_name, namelist[loop]->d_name, size);
    			if (send(sockfd, tempstring, strlen(tempstring), 0)==-1)
    				return;
    		}

	send(sockfd, dirfooter, sizeof(dirfooter), 0);
}

int serveconnection(int sockfd)
{
	char *tempdata, *ptr, *ptr2=NULL, *host_ptr1, *host_ptr2;
	char *tempstring;
	char filename[256];
	unsigned int loop=0, flag=0;
	int numbytes=0;
	int tno, err = -1;
	struct sockaddr_in sa;
	int addrlen = sizeof(struct sockaddr_in);
	t_vhost *thehost;
	char * stringpos = NULL;
	char status[10];

	tempdata = (char *)malloc(8192);
	tempstring = (char *)malloc(8192);

	thehost = &defaulthost;

	while(!strstr(tempdata, "\r\n\r\n") && !strstr(tempdata, "\n\n")) {
		if ((numbytes=recv(sockfd, tempdata+numbytes, 4096-numbytes, 0))==-1)
			goto end;
	}
	for(loop=0; loop<4096 && tempdata[loop]!='\n' && tempdata[loop]!='\r'; loop++)
		tempstring[loop] = tempdata[loop];

	tempstring[loop] = '\0';
	ptr = strtok(tempstring, " ");

	if (ptr == NULL) 
		goto end;

	if (strcmp(ptr, "GET") && strcmp(ptr, "POST")) {
		Log("Connection from %s, cmderror = \"GET %s\"", inet_ntoa(sa.sin_addr), ptr);
		sprintf(filename, "%s/cmderror.html", SERVERROOT);
		err = GetFile(sockfd, filename);
		goto end;
	}
	strcpy(status, ptr);
	ptr = strtok(NULL, " ");
	if (ptr == NULL) {
		Log("Connection from %s, cmderror = \"GET\"", inet_ntoa(sa.sin_addr));
		sprintf(filename, "%s/cmderror.html", SERVERROOT);
		err = GetFile(sockfd, filename);
		goto end;
	}
	host_ptr1 = strstr(tempdata, "Host:");
	if (host_ptr1) {
		host_ptr2 = strtok(host_ptr1+6, " \r\n\t");

		for(loop=0; loop<no_vhosts; loop++)
			if (!gstricmp(vhosts[loop].host, host_ptr2))
				thehost = &vhosts[loop];
	} else
		thehost = &defaulthost;
	if (strstr(ptr, "/..")) {
		Log("Connection from %s, 404 = \"GET %s\"", inet_ntoa(sa.sin_addr), ptr);
		sprintf(filename, "%s/404.html", SERVERROOT);
		err = GetFile(sockfd, filename);
		goto end;
	}

	getpeername(sockfd, (struct sockaddr *)&sa, (socklen_t*)&addrlen);
	Log("Connection from %s, request = \"%s %s\"", inet_ntoa(sa.sin_addr), status, ptr);

	// ***** PATCH *****
	// Replaces %20s of the message string by blanks
	while (strstr(ptr,"%20")!=NULL) {
		stringpos = strstr(ptr,"%20");
		*stringpos = (char)32;
		strcpy(stringpos+1,stringpos+3);
	}
	// ***** END *****

	if (strcmp(status, "GET") == 0) {
		if (!strncmp(ptr,thehost->CGIBINDIR,strlen(thehost->CGIBINDIR))){/* Trying to execute a cgi-bin file ? lets check */
			ptr2 = strstr(ptr, "?");
			if (ptr2!=NULL) { 
				ptr2[0] = '\0'; 
				flag = 1; 
				ptr2++;
			}

			strcpy(filename, thehost->CGIBINROOT);
			ptr += strlen(thehost->CGIBINDIR);
			tno = strlen(filename);
			strncat(filename, ptr, sizeof(filename)-tno);

			if (does_file_exist(filename)==TRUE && isDirectory(filename)==FALSE) {
				if (send(sockfd, "HTTP/1.1 200 OK\n", 16, 0)==-1)
					goto end;
				if (send(sockfd, "Server: "SERVERNAME"\n", strlen("Server: "SERVERNAME"\n"), 0)==-1)
					goto end;

				// It is a CGI-program that needs executing
				if (0 != dup2(sockfd, 0) || 1 != dup2(sockfd, 1))
					goto end;

				setbuf(stdin, 0);
				setbuf(stdout, 0);
				if (flag==1) 
					setenv("QUERY_STRING", ptr2, 1);
				chdir(thehost->CGIBINROOT);
				if (flag==1)
					execlp(filename, "", ptr2, NULL);
				else
					execlp(filename, NULL);
				goto end;
			}
			Log("Connection from %s, cgierror = \"EXEC %s %s\"", inet_ntoa(sa.sin_addr), filename ,ptr2);
			sprintf(filename, "%s/cgierror.html", SERVERROOT);
			err = GetFile(sockfd, filename);
			goto end;
		}

		if (strncmp(ptr, "/decode", strlen("/decode")) == 0) {
			ptr2 = strstr(ptr, "?");
			if (ptr2!=NULL) { 
				ptr2[0] = '\0'; 
				flag = 1; 
				ptr2++;
			}

			err = DecodeFile(sockfd, ptr2, thehost);
			goto end;
		}

		if (strncmp(ptr, "/poweroff", strlen("/poweroff")) == 0) {
			init_reboot(RB_POWER_OFF);
			err = 0;
			goto end;
		}
		else if (strncmp(ptr, "/reboot", strlen("/reboot")) == 0) {
			init_reboot(RB_AUTOBOOT);
			err = 0;
			goto end;
		}

		snprintf(filename, 255, "%s%s", thehost->DOCUMENTROOT, ptr);
		if (does_file_exist(filename)==FALSE) {
			if (filename[strlen(filename)-1] == '/')
				strcat(filename, thehost->DEFAULTPAGE);
			else {
				strcat(filename, "/");
				strcat(filename, thehost->DEFAULTPAGE);
			}
			if (does_file_exist(filename) == FALSE) {
				filename[strlen(filename)-strlen(thehost->DEFAULTPAGE)-1] = '\0'; // Get rid of the /index..
				if (isDirectory(filename) == TRUE) { 
					ShowDirectory(filename, sockfd, thehost); 
					goto end;
				}

				// File does not exist, so we need to display the 404 error page..
				sprintf(filename, "%s/404.html", SERVERROOT);
			}
		}
		err = GetFile(sockfd, filename);
		goto end;
	}
	else if (strcmp(status, "POST") == 0){
		snprintf(filename, 255, "%s%s", thehost->DOCUMENTROOT, ptr);
		err = PostFile(sockfd, filename);
		goto end;
	}
	err = 0;
end:
	free(tempdata);
	free(tempstring);
	close(sockfd);
	return err;
}

const char *getmimetype(const char *filename)
{
	char tempstring[50];
	char tempstring2[50];
	unsigned int loop=0;

	memset(tempstring, 0, 50);

	// Extract extension (will be reversed)
	for(loop=1; loop<strlen(filename) && filename[strlen(filename)-loop]!='.'; loop++)
		tempstring[loop-1] = filename[strlen(filename)-loop];

	// Now we need to put the string around the right way..
	for(loop=0; loop<strlen(tempstring); loop++)
		tempstring2[loop] = tempstring[strlen(tempstring)-loop-1];

	tempstring2[loop] = '\0';

	// tempstring2 now contains the extension of the file, we now need
	// to search for the mimetype of the file

	while (strcmp(assocNames[loop][0], "") != 0 ) {
		if (strcasecmp(assocNames[loop][0], tempstring2) == 0)
			return assocNames[loop][1];
		loop++;
	}
	
	return "application/octet-stream";
}

char assocNames[][2][100] =
{
	{ "mp2"  , "audio/x-mpeg"             },
	{ "mpa"  , "audio/x-mpeg"             },
	{ "abs"  , "audio/x-mpeg"             },
	{ "mpega", "audio/x-mpeg"             },
	{ "mpeg" , "video/mpeg"               },
	{ "mpg"  , "video/mpeg"               },
	{ "mpe"  , "video/mpeg"               },
	{ "mpv"  , "video/mpeg"               },
	{ "vbs"  , "video/mpeg"               },
	{ "mpegv", "video/mpeg"               },
	{ "bin"  , "application/octet-stream" },
	{ "com"  , "application/octet-stream" },
	{ "dll"  , "application/octet-stream" },
	{ "bmp"  , "image/x-MS-bmp"           },
	{ "exe"  , "application/octet-stream" },
	{ "mid"  , "audio/x-midi"             },
	{ "midi" , "audio/x-midi"             },
	{ "htm"  , "text/html"                },
	{ "html" , "text/html"                },
	{ "txt"  , "text/plain"               },
	{ "gif"  , "image/gif"                },
	{ "tar"  , "application/x-tar"        },
	{ "jpg"  , "image/jpeg"               },
	{ "jpeg" , "image/jpeg"               },
	{ "png"  , "image/png"                },
	{ "ra"   , "audio/x-pn-realaudio"     },
	{ "ram"  , "audio/x-pn-realaudio"     },
	{ "sys"  , "application/octet-stream" },
	{ "wav"  , "audio/x-wav"              },
	{ "xbm"  , "image/x-xbitmap"          },
	{ "zip"  , "application/x-zip"        },
	{ ""     , "application/octet-stream" }
};
