#ifndef KTVSTDCLASS_H
#define KTVSTDCLASS_H

#include <stdio.h>
#include <stdlib.h>

#include "avio/avio.h"

struct StrHashTable{
	char *Key;
	void *data;
	struct StrHashTable *Next;
};

//==============================================================================
class CStrHash //字符哈希快速查找表类
{
public:
	CStrHash(void);
	~CStrHash(void);

	void CreateHashTable(long Length);        // 建立 Hash 的空间大小
	void FreeHashList   (void);               // 释放 Hash 的空间
	void *FindNode      (const char *StrKey); // 查找结点
	void HashInsertNode (const char *StrKey, void *data); // 插入结点
#ifdef DEBUG
	void HashPrint();
#endif
protected:
	long HashLen;
	struct StrHashTable **FHashTable;
private:
	int  HashKey(const char *ToHash); // 计算HASH 值
};

//==============================================================================
class CMemoryStream // 内存流类
{
public:
	CMemoryStream();
	virtual ~CMemoryStream();
	void ClearData();                               // 清除内存数据
	bool LoadFromUrl(const char *Url, bool memory); // 将文件读入内存中
	long Seek(long Offset, int Origin);             // 数据定位
	long Read(void *Buffer, long Count);            // 从缓冲区中读数据
	long Write(void *Buffer, long Count);           // 向内存中写入数据
	long GetFileLen();                              // 得到文件大小
protected:
	virtual void Encrypt(char *Buffer, size_t Count, long Pos){}   // 加密
	virtual void DeEncrypt(char *Buffer, size_t Count, long Pos){} // 解密
private:
	URLContext *fp;
	long FileSize;       // 数据文件的大小
	long Position;       // 当前数据文件指读写指计位置
	unsigned char *Data; // 数据文件内容
};
//==============================================================================

#endif
