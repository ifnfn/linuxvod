/*==============================================================================
   T h e   K T V L i n u x   P r o j e c t
 -------------------------------------------------------------------------------
   Filename   : strext.h
   Author(s)  : Silicon
   Copyright  : Silicon

   一些扩展的字符串操作函数，以及相关需要的函数
==============================================================================*/

/*============================================================================*/
#ifndef STREXT_H
#define STREXT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#ifdef WIN32
	#include <windows.h>
#endif

//#define DEBUG
#ifndef DEBUG
#	define DEBUG_OUT(s...)
#else
#	define DEBUG_OUT(s...) printf(s)
#endif

/*============================================================================*/
#ifdef __cplusplus
extern "C" {
#endif

void trim(char *S);          // 清除字符串 S 两字的空格符
void strupper(char *str);    // 将字符串转成大写
void strlow(char *str);      // 将字符串转成小写
void StrPasToC(char *src);   // 将 pascal 的字符串转换成 C 的字符串
long getfilesize(int fp);    // 返回文件大小

#define Trim trim
#define lcase strlow
#define ucase strupper

int  atoidef(const char *nptr, int v);
bool atobooldef(const char *nptr, bool v);
bool FileExists(const char *fn);
void pathcat(char *url, const char* path, const char *filename);

int ExtractFilePath(const char *filename, char *path);
int ExtractFileName(const char *filename, char *file);
int ExtractFileExt (const char *filename, char *extname);

int Unicode(const char* charset, const char* inbuf, char *outbuf, int MaxLen);

#ifdef __cplusplus
}
#endif

/*============================================================================*/
#endif
