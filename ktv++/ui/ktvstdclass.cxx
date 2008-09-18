#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//#include "memtest.h"
#include "strext.h"
#include "ktvstdclass.h"

/*==============================================================================
 *  CStrHash
 *============================================================================*/
CStrHash::CStrHash(void)
{
	HashLen = 0;
	FHashTable = NULL;
}

CStrHash::~CStrHash(void)
{
	FreeHashList();
}

void CStrHash::CreateHashTable(long Length)
{
	HashLen = Length;
	FHashTable = (StrHashTable **)calloc(Length, sizeof(StrHashTable));
	for(long i=0; i<Length; i++){
		FHashTable[i] =(StrHashTable *) malloc(sizeof(StrHashTable));
		assert(FHashTable[i]);
		memset(FHashTable[i], 0, sizeof(StrHashTable));
	}
}

void CStrHash::FreeHashList(void)
{
	if (!FHashTable) return;
	struct StrHashTable *SongNode;
	struct StrHashTable *TmpSNode;
	for (int i=0; i < HashLen; i++){
		SongNode= FHashTable[i]->Next;
		while (SongNode != NULL){
			TmpSNode = SongNode;
			SongNode = SongNode->Next;

			free(TmpSNode->data);
			free(TmpSNode->Key);
			free(TmpSNode);
		}
		free(FHashTable[i]);
	}
	free(FHashTable);
	FHashTable = NULL;
}

void CStrHash::HashInsertNode(const char *StrKey, void *data)
{
	struct StrHashTable *Node = NULL;
	long Pos = HashKey(StrKey);
	if( (Pos >= 0) && (Pos < HashLen) ){
		Node = (struct StrHashTable *) malloc(sizeof(struct StrHashTable));
		assert(Node);
		Node->data = data;
		Node->Key = strdup(StrKey);
		Node->Next= FHashTable[Pos]->Next;
		FHashTable[Pos]->Next = Node;
	} else
		DEBUG_OUT("HashInsertNode Error.\n");
}

int CStrHash::HashKey(const char *ToHash)
{
#define HASHWORDBITS 32
	if (HashLen == 0) return -1;
	unsigned long int hval, g;
	const char *str = ToHash;  /* Compute the hash value for the given string.  */
	hval = 0;
	while (*str != '\0') {
		hval <<= 4;
		hval += (unsigned long int) *str++;
		g = hval & ((unsigned long int) 0xf << (HASHWORDBITS - 4));
		if (g != 0){
			hval ^= g >> (HASHWORDBITS - 8);
			hval ^= g;
		}
	}
	return hval % HashLen;
}

void *CStrHash::FindNode(const char *StrKey)
{
	long Pos;
	struct StrHashTable *Node;

	Pos = HashKey(StrKey);
	if(Pos < 0) return NULL;
	Node = FHashTable[Pos]->Next;
	while( Node != NULL){
		if (strcmp(Node->Key, StrKey)==0)
			break;
		Node = Node->Next;
	}
	if (Node)
		return Node->data;
	else
		return NULL;
}

#ifdef DEBUG
void CStrHash::HashPrint()
{
	struct StrHashTable *Node;
	for(long i=0;i<HashLen;i++) {
		DEBUG_OUT("[%ld] : ", i);
		Node = FHashTable[i]->Next;
		while (Node) {
			DEBUG_OUT("->[%s]", Node->Key);
			Node = Node->Next;
		};
		DEBUG_OUT("\n");
	}
}
#endif

/*=====================================================================================================
  CMemoryStream
*=====================================================================================================*/
CMemoryStream::CMemoryStream()
{
	fp       = NULL;
	FileSize = 0;       /* 数据文件的大小                  */
	Position = 0;       /* 当前数据文件指读写指计位置      */
	Data     = NULL;    /* 数据文件内容                    */
}

CMemoryStream::~CMemoryStream()
{
	ClearData();
	if (fp)
		url_close(fp);
}

bool CMemoryStream::LoadFromUrl(const char *url, bool memory)
{
	if (fp) url_close(fp);
	if(url_open(&fp, url, URL_RDONLY) < 0 )
		return false;
	if (memory){
		uint8_t buf[2049];
		int size;
		ClearData();
		while (1) {
			size = url_read(fp, buf, 2048);
			if (size <= 0) break;
			Write(buf, size);
		}
		url_close(fp);
		fp = NULL;
		Position = 0L;
	}
	else {
		free(Data);
		Data = NULL;
	}
	return true;
}

void CMemoryStream::ClearData()
{
	if (Data) {
		free(Data);
		Data = NULL;
	}
	FileSize = 0;       /* 数据文件的大小                  */
	Position = 0;       /* 当前数据文件指读写指计位置      */
}

long CMemoryStream::Seek(long Offset, int Origin)  /* 数据定位 */
{
	if (Data){
		switch (Origin){
			case SEEK_SET:
				Position = Offset;
				break;
			case SEEK_CUR:
				Position += Offset;
				break;
			case SEEK_END:
				Position = FileSize - Offset;
				break;
		}
		if (Position > FileSize)
			Position = FileSize - 1;
		return Position;
	} else
		return url_seek(fp, Offset, Origin);
}

long CMemoryStream::Read(void *Buffer, long Count) /* 从缓冲区中读数据 */
{
	long Result;
	long Pos;
	if (Data){
		Result = FileSize - Position > Count ? Count :FileSize - Position;
		memcpy(Buffer, Data + Position, Result);
		Pos = Position;
		Position += Result;
	}
	else{
		Pos = url_seek(fp, 0L, SEEK_CUR);
		Result = url_read(fp, (unsigned char *)Buffer, Count);
	}
	return Result;
}

long CMemoryStream::Write(void *Buffer, long Count)/* 写数据库到文件 */
{
	Data = (unsigned char *) realloc(Data, FileSize + Count);
	if (Data == NULL) {
		DEBUG_OUT("Memory realloc error.\n");
		return 0;
	}
	DeEncrypt((char *)Buffer, Count, FileSize);
	memcpy(Data + FileSize, Buffer, Count);
	FileSize += Count;
	return Count;
}

long CMemoryStream::GetFileLen()
{
	long CurPos = Seek(0L, SEEK_CUR);
	long Len    = Seek(0L, SEEK_END);
	Seek(CurPos, SEEK_SET);
	return Len;
}

