#ifndef SONGSTREAM_H
#define SONGSTREAM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ktvstdclass.h"  // KTV �ı�׼���
#include "data.h"

//==============================================================================
class CEncryptStream: public CMemoryStream  // ������
{
public:
	IndexNode *ReadFilter();                           // ��ȡ������
	void ReadToMemSongNode(MemSongNode *Node);         // ��ȡ�ڴ������¼
	void ReadToMemPinYin(struct tagMemPinYin *pinyin); // ��ȡ�ڴ�ƴ��������
private:
	virtual void Encrypt  (char *Buffer, size_t Count, long Pos); // ����
	virtual void DeEncrypt(char *Buffer, size_t Count, long Pos); // ����
};
//==============================================================================

#endif
