#define LINUX
#define USE_ISOC99
#define USE_GNU

#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ini.h"

#define ArePtrValid(Sec,Key,Val) ((Sec!=NULL)&&(Key!=NULL)&&(Val!=NULL))

#define BUF_LEN 255
static char Result[125] ={""};

#ifndef DEBUG_OUT
#define DEBUG_OUT(s...)
#endif

#define MakeChild 0
#define MakeNext  1

static struct ENTRY *MakeNewEntry(struct ENTRY *Head, int type);

static void FreeMem (void *Ptr);
static void FreeNode(struct ENTRY *Head);
static struct ENTRY *AddStrToIni(struct ENTRY *Head, char *Str);
static struct ENTRY *FindSection(struct ENTRY *Head, char *pSection);

#if 1
static void trim(char *S)
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
#endif

#ifdef DONT_HAVE_STRUPR
/* DONT_HAVE_STRUPR is set when INI_REMOVE_CR is defined */
void strupr( char *str )
{
	// We dont check the ptr because the original also dont do it.
	while (*str != 0){
		if (islower(*str))
			*str = toupper(*str);
		str++;
	}
}
#endif

static struct ENTRY *AddStrToIni(struct ENTRY *Head, char *Str)
{
	static struct ENTRY *CurSection = NULL;
	if ((strstr (Str, "[") > 0) && (strstr (Str, "]") > 0) && strstr(Str, "=") == NULL){/*	 Is Section*/
		char *pStr = strchr (Str, ']');
		if (pStr != NULL) *pStr = 0;
		pStr = strchr (Str, '[');
		if (pStr != NULL){
			CurSection = MakeNewEntry(Head, MakeChild); // 建一个新段
			CurSection->Node.Type = tpSECTION;          // 读出段信息
			strcpy(CurSection->Node.KeyText, pStr + 1); // 复制段名
		}
	}
	else{
		if (CurSection) { // 如果存在当前段
			if (strstr (Str, "=") > 0){  // 是键
				struct ENTRY *tmpKey = MakeNewEntry(CurSection, MakeNext); // 建立子键
				tmpKey->Node.Type = tpKEYVALUE;
				char *tmp = strtok(Str, "=");
				if (tmp) {
					strcpy(tmpKey->Node.KeyText, tmp);
					trim(tmpKey->Node.KeyText);
				}
				tmp = strtok(NULL, ";");
				if (tmp) {
					strcpy(tmpKey->Node.ValText, tmp);
					trim(tmpKey->Node.ValText);
				}
				tmp = strtok(NULL, "");
				if (tmp) {
					strcpy(tmpKey->Node.Comment, tmp);
					trim(tmpKey->Node.Comment);
				}
			}
		}
	}
	return CurSection;
}

struct ENTRY *OpenIniFileFromMemory(unsigned char *data, long len)
{
	char Str[255];
	char *pStr;
	struct ENTRY *Head = NULL;
	long DataLen = 0;
	int	CurIndex = 0;
	memset(Str, 0x0, 255);
	if (data==NULL || len <= 0)
		return NULL;
	Head = MakeNewEntry(NULL, MakeChild);   // 建一个新段
	if (Head == NULL)
		return NULL;
	while (DataLen < len)
	{
		memset(Str, 0, 255);
		CurIndex = 0;
		while( CurIndex < 253)
		{
			if (DataLen >= len)
				break;
			if (data[DataLen] == 0xA)
			{
				Str[CurIndex++] = data[DataLen++];
				break;
			}
			Str[CurIndex++] = data[DataLen++];
		}
		Str[CurIndex] = '\0';
		trim(Str);
		pStr = strchr (Str, '\n');
		if (pStr != NULL) *pStr = 0;
#ifdef INI_REMOVE_CR
		pStr = (char *)memchr(Str, '\r' , strlen(Str));
		if (pStr!=NULL) *pStr = '\0';
#endif
		AddStrToIni(Head, Str);
	}
	return Head;
}

struct ENTRY *OpenIniFile(char *FileName)
{
	FILE *fd = fopen (FileName, "r");
	if (fd == NULL) return NULL;

	char Str[255];
	char *pStr;
	memset(Str, 0x0, 255);

	struct ENTRY *Head = MakeNewEntry(NULL, MakeChild);   // 建一个新段
	if (Head == NULL) return NULL;

	while (fgets (Str, 255, fd) != NULL)
	{
		trim(Str);
		pStr = strchr (Str, '\n');
		if (pStr != NULL) *pStr = 0;
#ifdef INI_REMOVE_CR
		pStr = (char *)memchr(Str, '\r' , strlen(Str));
		if (pStr!=NULL) *pStr = '\0';
#endif
		AddStrToIni(Head, Str);
	}
	return Head;
}

void CloseIniFile(struct ENTRY *Head)
{
	FreeNode(Head);
}

char *ReadString(struct ENTRY *Head, char *pSection, char *pKey, char *Default)
{
	if (!ArePtrValid(pSection, pKey, Default)){
		puts("Program normaly");
		strcpy(Result, Default);
		return Result;
	}

	struct ENTRY *section = FindSection(Head, pSection);
	if (section)
	{
		struct ENTRY *pEntry = FindpKey(section, pKey);
		if ( pEntry != NULL){
			strcpy (Result, pEntry->Node.ValText);
			return Result;
		}

		strcpy(Result, Default);
		return Result;
	}
	return NULL;
}

bool ReadBool(struct ENTRY *Head, char * pSection, char * pKey, bool Default)
{
	char Val[2] = {"0"};
	if (Default != 0)
		Val[0] = '1';
	return (atoi (ReadString (Head, pSection, pKey, Val)) ? 1 : 0);	/* Only 0 or 1 allowed */
}

int ReadInt(struct ENTRY *Head, char * pSection, char * pKey, int Default)
{
	int i;
	char Val[100];
	sprintf(Val, "%d", Default);
	sscanf(ReadString (Head, pSection,  pKey, Val), "%i", &i);
	return i;
}

double ReadDouble(struct ENTRY *Head, char * pSection, char * pKey, double Default)
{
	double Val;
	sprintf (Result, "%1.10lE", Default);
	sscanf (ReadString (Head, pSection, pKey, Result), "%lE", &Val);
	return Val;
}

static void FreeMem(void *Ptr)
{
	if (Ptr != NULL)
		free (Ptr);
}

static void FreeNode(struct ENTRY *Head) // 后序遍历
{
	if (Head) {
		FreeNode(Head->pChild);
		FreeNode(Head->pNext);
		FreeMem(Head);
	}
}

static struct ENTRY *FindSection(struct ENTRY *Head, char *pSection)
{
	if (Head == NULL) return NULL;

	struct ENTRY *pEntry = Head->pChild;
	while (pEntry != NULL){
		if (pEntry->Node.Type == tpSECTION){
			if (strcasecmp (pEntry->Node.KeyText, pSection) == 0) {
				return pEntry;
			}
		}
		pEntry = pEntry->pChild;
	}
	return NULL;
}

struct ENTRY *FindpKey(struct ENTRY *pSection, char * pKey)
{
	struct ENTRY *pEntry = pSection;
	if (pEntry)
	{
		pEntry = pEntry->pNext;
		while (pEntry != NULL)
		{
			if (pEntry->Node.Type == tpKEYVALUE)
			{
				if (strcasecmp(pEntry->Node.KeyText, pKey) == 0)
					return pEntry;
			}
			pEntry = pEntry->pNext;
		}
	}
	return NULL;
}

int ReadKeyValueList(struct ENTRY *Head, char *pSection, char ***Key, char ***value)
{
	struct ENTRY *pEntry = FindSection (Head, pSection);
	if (pEntry == NULL)
		return 0;
	pEntry = pEntry->pNext;
	int count = 0;
	while (pEntry != NULL)
	{
		if (pEntry->Node.Type == tpKEYVALUE)
		{
			count++;
			if (value) {
				*value = (char **)realloc(*value, count * sizeof(char*));
				(*value)[count - 1] = strdup(pEntry->Node.ValText);
			}
			if (Key) {
				*Key   = (char **)realloc(*Key,  count * sizeof(char *));
				(*Key)[count - 1]   = strdup(pEntry->Node.KeyText);
			}
		}
		pEntry = pEntry->pNext;
	}
	return count;
}

static struct ENTRY * MakeNewEntry(struct ENTRY *Head, int type)
{
	struct ENTRY *pEntry;
	pEntry = (struct ENTRY *) malloc (sizeof(struct ENTRY));
	if (pEntry == NULL)
		return NULL;
	pEntry->pChild = NULL;
	pEntry->pNext  = NULL;
	memset(&pEntry->Node, 0, sizeof(NODE));
	pEntry->Node.Type = tpNULL;

	if (Head == NULL) return pEntry;
	if (type == MakeChild){
		pEntry->pChild = Head->pChild;
		Head->pChild = pEntry;
	}
	else {
		pEntry->pNext = Head->pNext;
		Head->pNext = pEntry;
	}
	return pEntry;
}

bool ValueExists(struct ENTRY *Head, char *pSection, char *pKey)
{
	struct ENTRY *section = FindSection(Head, pSection);
	if (section)
		return FindpKey(section, pKey) != NULL;
	return false;
}

void PrintIniFile(struct ENTRY *Head)
{
	if (Head)
	{
		if (Head->Node.Type == tpSECTION)
			DEBUG_OUT("[%s]\n", Head->Node.KeyText);
		else
			DEBUG_OUT("%s=%s;%s\n", Head->Node.KeyText, Head->Node.ValText, Head->Node.Comment);
		PrintIniFile(Head->pNext);
		PrintIniFile(Head->pChild);
	}
}

