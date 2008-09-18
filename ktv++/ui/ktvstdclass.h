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
class CStrHash //�ַ���ϣ���ٲ��ұ���
{
public:
	CStrHash(void);
	~CStrHash(void);

	void CreateHashTable(long Length);        // ���� Hash �Ŀռ��С
	void FreeHashList   (void);               // �ͷ� Hash �Ŀռ�
	void *FindNode      (const char *StrKey); // ���ҽ��
	void HashInsertNode (const char *StrKey, void *data); // ������
#ifdef DEBUG
	void HashPrint();
#endif
protected:
	long HashLen;
	struct StrHashTable **FHashTable;
private:
	int  HashKey(const char *ToHash); // ����HASH ֵ
};

//==============================================================================
class CMemoryStream // �ڴ�����
{
public:
	CMemoryStream();
	virtual ~CMemoryStream();
	void ClearData();                               // ����ڴ�����
	bool LoadFromUrl(const char *Url, bool memory); // ���ļ������ڴ���
	long Seek(long Offset, int Origin);             // ���ݶ�λ
	long Read(void *Buffer, long Count);            // �ӻ������ж�����
	long Write(void *Buffer, long Count);           // ���ڴ���д������
	long GetFileLen();                              // �õ��ļ���С
protected:
	virtual void Encrypt(char *Buffer, size_t Count, long Pos){}   // ����
	virtual void DeEncrypt(char *Buffer, size_t Count, long Pos){} // ����
private:
	URLContext *fp;
	long FileSize;       // �����ļ��Ĵ�С
	long Position;       // ��ǰ�����ļ�ָ��дָ��λ��
	unsigned char *Data; // �����ļ�����
};
//==============================================================================

#endif
