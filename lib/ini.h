#ifndef INIPLUS_H
#define	INIPLUS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define	LINUX

#define tpNULL       0
#define tpSECTION    1
#define tpKEYVALUE   2
#define tpCOMMENT    3
#define DEFAULT_STRING  ( "")

typedef struct tagNODE {
	char Type;            // 结点的类型，是null Section Key or Comment
	char KeyText[128];
	char ValText[128];
	char Comment[255];
} NODE;

typedef struct ENTRY {
	struct tagNODE Node;
	struct ENTRY *pChild; // 子
	struct ENTRY *pNext;  // 兄
}iniENTRY;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LINUX /* Remove CR, on unix systems. */
	#define INI_REMOVE_CR
	#define DONT_HAVE_STRUPR
	void strupr(char *str);
#endif

struct ENTRY *OpenIniFile (char *FileName);
struct ENTRY *OpenIniFileFromMemory (unsigned char *data, long len);
void   PrintIniFile(struct ENTRY *Head);
void   CloseIniFile(struct ENTRY *Head);
struct ENTRY *FindpKey(struct ENTRY *pSection, char *pKey);
bool   ReadBool    (struct ENTRY *Head, char *pSection, char *pKey, bool   Default);
int    ReadInt     (struct ENTRY *Head, char *pSection, char *pKey, int    Default);
double ReadDouble  (struct ENTRY *Head, char *pSection, char *pKey, double Default);
char  *ReadString  (struct ENTRY *Head, char *pSection, char *pKey, char  *Default);
bool   ValueExists (struct ENTRY *Head, char *pSection, char *pKey);
int    ReadKeyValueList (struct ENTRY *Head, char *pSection, char ***Key, char ***value);
#ifdef __cplusplus
}
#endif

#endif
