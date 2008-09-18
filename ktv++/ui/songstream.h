#ifndef SONGSTREAM_H
#define SONGSTREAM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ktvstdclass.h"  // KTV 的标准类库
#include "data.h"

//==============================================================================
class CEncryptStream: public CMemoryStream  // 加密流
{
public:
	IndexNode *ReadFilter();                           // 读取过滤项
	void ReadToMemSongNode(MemSongNode *Node);         // 读取内存歌曲记录
	void ReadToMemPinYin(struct tagMemPinYin *pinyin); // 读取内存拼音过滤项
private:
	virtual void Encrypt  (char *Buffer, size_t Count, long Pos); // 加密
	virtual void DeEncrypt(char *Buffer, size_t Count, long Pos); // 解密
};
//==============================================================================

#endif
