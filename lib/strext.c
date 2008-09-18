#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>

#include "strext.h"

#define BUF_LEN 255
void trim(char *S)
{
	int l = strlen(S);
	int i;
	for(i=0;(i<l) && ((S[i] & 0x7F) <=' ') ;i++);
	if (i>=l)
		S[0] = '\0';
	else {
		for (;(S[l] & 0x7F) <= ' ';l--);
			strncpy(S, S+i,l-1+1);
	}
	S[l-i+1]= '\0';
}

void strupper( char *str )
{
	while (*str != 0){
		if ( islower( *str ) )
			*str = toupper( *str );
		str++;
	}
}

void strlow( char *str )
{
	while( *str != 0) {
		if (isupper(*str))
			*str = tolower(*str);
		str++;
	}
}

void StrPasToC(char *src)
{
	int count = src[0];
	memcpy(src, src + 1, count);
	src[count] = '\0';
}

long getfilesize(int fp)
{
	long CurPos = lseek(fp, 0L, SEEK_CUR);
	long Len    = lseek(fp, 0L, SEEK_END);
	lseek(fp, CurPos, SEEK_SET);
	return Len;
}

int atoidef(const char *nptr, int v)
{
	if (nptr)
		return atoi(nptr);
	else
		return v;
}

bool atobooldef(const char *nptr, bool v)
{
	if (nptr)
		return strcasecmp(nptr, "false");
	else
		return v;
}

bool FileExists(const char *fn)
{
#ifdef WIN32
#else
	struct stat buf;
	return stat(fn, &buf) == 0;
#endif
}

void pathcat(char *url, const char* path, const char *filename)
{
	strcpy(url, path);
	strcpy(url, path);
	int len = strlen(url);
	if (url[len-1] != '/') {
		url[len] = '/';
		url[len+1] ='\0';
	}
	const char *p = filename;
	if (filename[0]=='/')
		p = filename+1;	
	strcat(url, p);
}

int ExtractFilePath(const char *filename, char *path)
{       
	strcpy(path, filename);
	char *p = rindex(path, '/');
	if (p != NULL)
		*(p+1) = '\0';
	return p != NULL;
}       
        
int ExtractFileName(const char *filename, char *file) 
{       
	char tmp[1024];
	strcpy(tmp, filename);
	char *p = rindex(tmp, '/');
	if (p != NULL)
	{               
		char *p1 = rindex(tmp, '.');
		if (p1 != NULL)
			*p1 = '\0';
		strcpy(file, p+1);
	} else
		file[0] = '\0';
	return p != NULL;
}

int ExtractFileExt(const char *filename, char *extname)
{
	char *p = rindex(filename, '.');
	if (p != NULL)
		strcpy(extname, p + 1);
	else
		extname[0] = '\0';
	return p != NULL;
}

