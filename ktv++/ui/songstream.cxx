#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "memtest.h"
#include "strext.h"
#include "songstream.h"
#include "avio/avio.h"

IndexNode *CEncryptStream::ReadFilter()
{
	IndexNode *node = (IndexNode*)malloc(sizeof(IndexNode));
	assert(node);
	memset(node, 0, sizeof(IndexNode));
	Read(node->IndexName, FilterLen);
	Read(&node->count, sizeof(int));
	int size = sizeof(FilterNode)*node->count;
	node->Filters = (FilterNode*)malloc(size);
	assert(node->Filters);
	memset(node->Filters, 0, size);
	Read(node->Filters, size);
	return node;
}

void CEncryptStream::ReadToMemSongNode(MemSongNode *Node)
{
	struct tagSongListNode tmprec;
	if (Read(&tmprec, sizeof(struct tagSongListNode))) {
		Node->SongCode   = strdup(tmprec.SongCode);
		Node->SongName   = strdup(tmprec.SongName);
		Node->Charset    = tmprec.Charset;
		Node->Language   = strdup(tmprec.Language);
		Node->SingerName = strdup(tmprec.SingerName);
		Node->VolumeK    = tmprec.VolumeK;
		Node->VolumeS    = tmprec.VolumeS;
		Node->Num        = tmprec.Num;
		Node->Klok       = tmprec.Klok;
		Node->Sound      = tmprec.Sound;
		Node->SoundMode  = tmprec.SoundMode;
		Node->StreamType = strdup(tmprec.StreamType);
	}
}

void CEncryptStream::ReadToMemPinYin(struct tagMemPinYin *pinyin)
{
	Pinyin_t tmppinyin;
	Read(&tmppinyin, sizeof(Pinyin_t));
	pinyin->PinYin = strdup(tmppinyin.PinYin);
	pinyin->WBH    = strdup(tmppinyin.WBH);
	pinyin->Num    = tmppinyin.Num;
	pinyin->SongID = tmppinyin.SongID;
}

void CEncryptStream::Encrypt(char *Buffer, size_t Count, long Pos)
{
	size_t P = Pos;
	for (size_t i=0; i<Count; i++)
		Buffer[i] = Buffer[i] ^(char)(P+i);
}

void CEncryptStream::DeEncrypt(char *Buffer, size_t Count, long Pos)
{
	for (size_t i=0; i<Count; i++)
		Buffer[i] = Buffer[i] ^ (char)(Pos+i);
}

