#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <windows.h>
#ifdef LINUX
#include <ftw.h>
#define PATHCHAR '/'
#else
#define PATHCHAR '\\'
#endif

#define index strchr 
#define rindex strrchr 

#include "headerdetection.h"

const char * strStreamType[] = {
  "UNKNOWN",
  "MPEG1_VIDEO",
  "MPEG1_AUDIO",
  "MPEG1_SYSTEM",
  "MPEG1_AVI",
  "MPEG2_VIDEO",
  "MPEG2_SYSTEM",
  "MPEG2_TRANSPORT",
  "DVD_VMG",
  "DVD_VTS",
  "AC3_AUDIO",
  "PES",
  "MPEG4_VIDEO",
  "DTS_AUDIO",
  "MPEG4_SYSTEM",
  "MPEG4_SYSTEMAUDIO",
  "MPEG4_SYSTEMVIDEO",
  "QUICKTIME",
  "DVD_VOB"
};

DWORD getStreamType(char * filename, FileStreamType *streamType)
{
  if (streamType == NULL) {
    FileStreamType mm;
    GetFileProperties(filename, &mm);
    return mm.fileType;
  } 
  else {
    GetFileProperties(filename, streamType);
    return streamType->fileType;
  }
}

static int ExtractFilePath(const char *filename, char *path)
{
	strcpy(path, filename);
	char *p = rindex(path, PATHCHAR);
	if (p)
		*(p+1) = '\0';
	return p != NULL;
}

static int ExtractFileName(const char *filename, char *file)
{
	char tmp[1024];
	strcpy(tmp, filename);
	char *p = rindex(tmp, PATHCHAR);
	if (p){
		char *p1 = rindex(tmp, '.');
		if (p1 != NULL)
			*p1 = '\0';
		strcpy(file, p+1);
	} else
		file[0] = '\0';
	return p != NULL;
}

static int ExtractFileExt(const char *filename, char *extname)
{
	char *p = rindex(filename, '.');
	if (p)
		strcpy(extname, p + 1);
	else
		extname[0] = '\0';
	return p != NULL;
}

#ifndef FTW_F
#define FTW_F 1
#endif

static int fn(const char *file, const struct stat *sb, int flag)
{
	char code[20], path[200], extname[10];
	DWORD x;
	static int count=0;

	if (flag == FTW_F){
		ExtractFilePath(file, path);
		ExtractFileName(file, code);
		ExtractFileExt(file, extname);
		x = getStreamType((char *)file, NULL);
		if (strcasecmp(extname, "dat") == 0){
			if ((x != FT_MPEG_VIDEO) && (x != FT_MPEG_SYSTEM)){ // DVD
				printf("delete from system where code=%s;\n", code);
//				printf("(%s) mv %s %s%s.vob\n", strStreamType[x], file, path,code);
				fprintf(stderr, "count(%d): %s %s\n", count++, file, strStreamType[x]);
			}
		}
		else if	(strcasecmp(extname, "vob") == 0){
			if ((x != FT_MPEG2_SYSTEM) && (x != FT_MPEG2_VIDEO) && (x != FT_DVD_VOB)){ // VCD
				printf("delete from system where code=%s\n", code);
//				printf("(%s) mv %s %s%s.dat\n", strStreamType[x], file, path,code);
				fprintf(stderr, "count(%d): %s %s\n", count++, file, strStreamType[x]);
			}
		}
		fprintf(stderr, "count: %d\n", count++);
	}
	return 0;
}

//使用API方式实现删除不为空的目录
bool FindDirectory(const char* pszDir)
{
       DWORD x;
       WIN32_FIND_DATA fd;
       char szTempFileFind[MAX_PATH] = { 0 };
       bool bIsFinish = false;
       FileStreamType streamType;
    
	   char code[20], path[200], extname[10];

       ZeroMemory(&fd, sizeof(WIN32_FIND_DATA));
    
       sprintf(szTempFileFind, "%s\\*.*", pszDir);
    
       HANDLE hFind = FindFirstFile(szTempFileFind, &fd);
       if (hFind == INVALID_HANDLE_VALUE)
          return false;
    
       while (!bIsFinish) {
          bIsFinish = (FindNextFile(hFind, &fd)) ? false : true;
          if ((strcmp(fd.cFileName, ".") != 0) && (strcmp(fd.cFileName, "..") != 0)) {
             char szFoundFileName[MAX_PATH] = { 0 };
             strcpy(szFoundFileName, fd.cFileName);
    
             if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                char szTempDir[MAX_PATH] = { 0 };
                sprintf(szTempDir, "%s\\%s", pszDir, szFoundFileName);
                FindDirectory(szTempDir);
             } 
             else {
                char szTempFileName[MAX_PATH] = { 0 };
                sprintf(szTempFileName, "%s\\%s", pszDir, szFoundFileName);

		        ExtractFilePath(szTempFileName, path);
          		ExtractFileName(szTempFileName, code);
            	ExtractFileExt (szTempFileName, extname);
//            	printf("Path: %s\nCode=%s\nextname=%s\n", path, code, extname);

                x = getStreamType(szTempFileName, &streamType);
			    if ((x == FT_MPEG_VIDEO) || (x == FT_MPEG_SYSTEM)){ // VCD
  			       printf("ren %s %s.dat\n", szTempFileName, code);
  			       fprintf(stderr, "ren %s %s.dat\n", szTempFileName, code);
//			  printf("ren %s %s%s.dat\n", szTempFileName, path,code);
//				  printf("(%s) mv %s %s%s.dat\n", strStreamType[x], szTempFileName, path,code);
			    }
             }
       }
 }
 FindClose(hFind);

 if (!RemoveDirectory(pszDir))
  return false;
 return true;
}

int main(int argc, char *argv[])
{
    printf("%d\n", sizeof(unsigned long));
      FindDirectory(argv[1]);
//  FindDirectory("\\\\192.168.0.147\\08");
//  FindDirectory("D:\\song");
  system("PAUSE");	
  return 0;
}
