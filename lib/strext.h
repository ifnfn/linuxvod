/*==============================================================================
   T h e   K T V L i n u x   P r o j e c t
 -------------------------------------------------------------------------------
   Filename   : strext.h
   Author(s)  : Silicon
   Copyright  : Silicon

   һЩ��չ���ַ��������������Լ������Ҫ�ĺ���
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

void trim(char *S);          // ����ַ��� S ���ֵĿո��
void strupper(char *str);    // ���ַ���ת�ɴ�д
void strlow(char *str);      // ���ַ���ת��Сд
void StrPasToC(char *src);   // �� pascal ���ַ���ת���� C ���ַ���
long getfilesize(int fp);    // �����ļ���С

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
