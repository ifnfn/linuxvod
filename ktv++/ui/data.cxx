/************************************************************************
   T h e   K T V L i n u x   P r o j e c t
 ------------------------------------------------------------------------
   Filename   : data.cxx
   Author(s)  : Silicon

   实现对数据表的封装，歌曲列表，歌手资料列表都在这个单元中实现
*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include "memtest.h"

#include "data.h"
#include "strext.h"
#include "songstream.h"
#include "xmltheme.h"

static bool IsPrime(long m)
{
	long i,k;
	k = (long)sqrt(m);
	for (i=2; i<=k; i++)
		if ((m % i) ==0) return false;
	return true;
}

static long NextPrime(long x)
{
	if ((x % 2) == 0) x++;
	long i;
	for (i=x;;i+=2)
		if (IsPrime(i)==true)
	return i;
}

void FreeMemSongNode(MemSongNode *node)
{
	if (node ==NULL) return;
	if (node->SongCode) {
		free(node->SongCode);
		node->SongCode = NULL;
	}
	if (node->SongName) {
		free(node->SongName);
		node->SongName = NULL;
	}
	if (node->Language) {
		free(node->Language);
		node->Language = NULL;
	}
	if (node->SingerName) {
		free(node->SingerName);
		node->SingerName= NULL;
	}

	if (node->StreamType) {
		free(node->StreamType);
		node->StreamType = NULL;
	}
}

/*=====================================================================================================
   CData
*=====================================================================================================*/
CData::CData(void):CStrHash(), SongLists(NULL), FilterSongCount(0),\
	FPinYinList(NULL), PerPageNum(6), FFilter(NULL)
{
	CurPage.Count = 0;
	CurPage.RecList = NULL;
	memset(&FHead, 0, sizeof(tagHeader));
	memset(CharFilter, 0, sizeof(IndexNode) * 26);
}

CData::CData(const char *FileName, bool LoadMemory):CStrHash(), SongLists(NULL), FilterSongCount(0),\
	FPinYinList(NULL), PerPageNum(6), FFilter(NULL)
{
	if (LoadDataDB(FileName, LoadMemory) == false) {
		printf("Error: No found %s.\n", FileName);
		abort();
	}
}

CData::~CData(void)
{
	FreeDataDB();
}

bool CData::LoadDataDB(const char *FileName, bool LoadMemory)
{
	CurPage.Count   = 0;
	CurPage.RecList = NULL;
	memset(&FHead, 0, sizeof(tagHeader));
	memset(CharFilter, 0, sizeof(IndexNode) * 26);
	DataBaseFile = FileName;
	FreeDataDB();
	return CreateSeekTable(FileName, LoadMemory);
}

void CData::Reload()
{
	LoadDataDB(DataBaseFile.data(), true);
}

void CData::FreeDataDB(void)
{
	// 释放歌曲数组
	if (SongLists) {
		printf("FHead.SongCount=%ld\n", FHead.SongCount);
		for (int i=0;i<FHead.SongCount;i++)
			FreeMemSongNode(SongLists + i);
		free(SongLists);
		SongLists = NULL;
	}

	if (FPinYinList)
	{
		for (int i=0;i<FHead.PYCount;i++) {
			free(FPinYinList[i].PinYin);
			free(FPinYinList[i].WBH);
		}
		free(FPinYinList);
		FPinYinList = NULL;
	}
	if (CurPage.RecList)
		free(CurPage.RecList);
	FreeHashNodes();
	if (FFilter) {
		free(FFilter);
		FFilter = NULL;
	}
	for (int i=0; i<26; i++)
		if (CharFilter[i].Filters)
			free(CharFilter[i].Filters);
}

bool CData::CreateSeekTable(const char *FileName, bool LoadMemory)
{
	CEncryptStream DataBuffer;

	DataBuffer.ClearData();
	if (DataBuffer.LoadFromUrl(FileName, LoadMemory) == false)
		return false;

	memset(&FHead, 0, sizeof(struct tagHeader));
	DataBuffer.Read(&FHead, sizeof(struct tagHeader));
#if 0
	printf("IndexCount   = %ld\n", FHead.IndexCount);
	printf("DataPosition = %ld\n", FHead.DataPosition);
	printf("PyPosition   = %ld\n", FHead.PyPosition);
	printf("SongCount    = %ld\n", FHead.SongCount);
	printf("PYCount      = %ld\n", FHead.PYCount);
#endif
	CreateHashTable(NextPrime(FHead.IndexCount));

	IndexNode *node;
	for(long i=0; i<FHead.IndexCount; i++){
		node = DataBuffer.ReadFilter(); // Free on ktvstdclass.cxx FreeListNode
		HashInsertNode(node->IndexName, node);
	}
#ifdef DEBUG
//	HashPrint();
#endif
	if (SongLists) 
		free(SongLists);
	SongLists = (MemSongNode *) malloc(sizeof(MemSongNode) * FHead.SongCount);
	memset(SongLists, 0, sizeof(MemSongNode) * FHead.SongCount);

	DataBuffer.Seek(FHead.DataPosition, SEEK_SET);
	for (long i=0; i<FHead.SongCount; i++){
		DataBuffer.ReadToMemSongNode(SongLists + i);
//		DEBUG_OUT("%s = %s\n", SongLists[i].SongCode, SongLists[i].SongName);
//		DEBUG_OUT("%s = %d\n", SongLists[i].SongCode, strlen(SongLists[i].SongCode));
//		DEBUG_OUT("%s = %d\n", SongLists[i].SongName, strlen(SongLists[i].SongName));
	}
//------------------------------CreatePyFilter--------------------------------------------
	FPinYinList = (tagMemPinYin *) malloc(FHead.PYCount * sizeof(tagMemPinYin));
	assert(FPinYinList);
	DataBuffer.Seek(FHead.PyPosition, SEEK_SET);
	for (int i=0;i<FHead.PYCount;i++)
		DataBuffer.ReadToMemPinYin(FPinYinList + i);
	return true;
}

void CData::GetCurrentPage(void)
{
	CurPage.Count = 0;
	if( FilterSongCount <= 0) return ;
	while((CurRecID + CurPage.Count < FilterSongCount) && (CurPage.Count < PerPageNum)){
		CurPage.RecList[CurPage.Count] = SongLists + FFilter[CurRecID + CurPage.Count].SongID;
		CurPage.Count++;
	}
}

long CData::GetCurPageNo(void)
{
	return (long)(CurRecID / PerPageNum + 1);
}

void CData::SetPerPageNum(int pagenum)
{
	PerPageNum = pagenum;
	CurPage.RecList = (MemSongNode **)realloc(CurPage.RecList, PerPageNum * sizeof(MemSongNode*));
	memset(CurPage.RecList, 0, sizeof(MemSongNode*) * PerPageNum);
	TotalPageCount = FilterSongCount / pagenum;
	TotalPageCount = (FilterSongCount % pagenum >0) ? TotalPageCount + 1 : TotalPageCount;
}

void CData::FirstPage(void)
{
	CurRecID = 0;
	GetCurrentPage();
}

void CData::EndPage(void)
{
	CurRecID = (TotalPageCount - 1) * PerPageNum;
	GetCurrentPage();
}

bool CData::NextPage(void)
{
	bool eod = true;
	if (CurRecID < (FilterSongCount - PerPageNum))
		CurRecID += PerPageNum;
	else if (CurRecID < FilterSongCount - 1)
		CurRecID += (FilterSongCount - CurRecID);
	else
		eod =false;
	GetCurrentPage();
	return eod;
}

void CData::PriorPage(void)
{
	CurRecID = ((CurRecID - PerPageNum) < 0) ? 0 : (CurRecID - PerPageNum);
	GetCurrentPage();
}

bool CData::Bof(void)
{
	return CurRecID <= 0;
}

bool CData::Eof(void)
{
	return CurRecID >= (FilterSongCount - PerPageNum);
}

/*==============================================================================
 *	name: 过滤条件
 *	desc: 过滤描述
 *	id  : 过滤类型  0:分类索引 1:拼音索引 2: 五笔划索引 3: 手写(歌名)索引
 *============================================================================*/
void CData::SetFilterName(CString& name, CString& desc, int id) // 设置过滤条件
{
	filter = name;
	FilterDescription = desc;
	filterclass = id;
}

long CData::ActiveFilter(int PageNum, bool update)
{
	static CString oldfilter("");
	static int oldfilterclass = -1;
	if (oldfilter != filter || (oldfilterclass != filterclass) || update)
	{
		switch (filterclass)
		{
			case 0:	FilterSongCount = SetFilterIndex(PageNum);   break;
			case 1:	FilterSongCount = SetPYFilterIndex(PageNum); break;
			case 3:	FilterSongCount = SetHandWriteIndex(PageNum);break;
			default:
				FilterSongCount = SetWBHFilterIndex(PageNum);
		}
		SetPerPageNum(PerPageNum);
		CreateNewCharFilerNode(); // 建立字母过滤表
		oldfilter = filter;
		oldfilterclass = filterclass;
	}
	CurRecID = 0;
	GetCurrentPage();
	return FilterSongCount;
}

long CData::SetFilterIndex(int PageNum)
{
	char sctBuf[100];
	long size = 0;
	IndexNode *OtherRec;

	PerPageNum = PageNum;
	if (filter == "0字歌")
	{
		for(int i=1; i<=9; i++){
			sprintf((char *)sctBuf, "%d字歌", i);
			if ((OtherRec = (IndexNode *)FindNode(sctBuf))){
				AllocFFilterMemory(size + OtherRec->count);
				memcpy(FFilter+size, OtherRec->Filters, OtherRec->count * sizeof(FilterNode));
				size += OtherRec->count;
			}
		}
	}
	else if ((OtherRec = (IndexNode *)FindNode(filter.data()))) {
		size = OtherRec->count;
		AllocFFilterMemory(size);
		memcpy(FFilter, OtherRec->Filters, size * sizeof(FilterNode));
	}
	return size;
}

long CData::PinYinFilter(CString& pinyin)
{
	int len = pinyin.length();
	if (len == 0)
		return FilterSongCount;
	long tmpCount = 0;
	int id =0;
	for (int i=0; i<len; i++)
	{
		id = pinyin[i] - 'A';
		if (id>=0 && id < 26)
		{
			tmpCount += CharFilter[id].count;
			AllocFFilterMemory(tmpCount);
			memcpy(FFilter+tmpCount-CharFilter[id].count,CharFilter[id].Filters,
				sizeof(FilterNode)*CharFilter[id].count);
		}
	}
	FilterSongCount = tmpCount;
	SetPerPageNum(PerPageNum);
	CurRecID = 0;
	GetCurrentPage();
	return tmpCount;
}

long CData::SetPYFilterIndex(int PageNum)
{
	long Result  = 0 ;
	long Len     = filter.length();
	long WordNum = 0;
	int i1 = 0;
	int i2 = 0;
	char cPinYin[Len + 1];
	char Num[Len + 1];

	memset(cPinYin, '\0', Len + 1);
	memset(Num, '\0', Len + 1);
	PerPageNum = PageNum;

	for(long i = 0; i<Len; i++){
		if (filter[i]>='A' && filter[i]<='Z')
			cPinYin[i1++] = filter[i];
		else if(filter[i]>='0' && filter[i]<='9')
			Num[i2++] = filter[i];
	}
	cPinYin[i1] = '\0';
	Num[i2] = '\0';
	sscanf(Num, "%ld", &WordNum);
	Len = strlen(cPinYin);
	Result = 0;
	AllocFFilterMemory( 400 );
	memset(FFilter, 0, 400 * sizeof(FilterNode));
	for(long i=0; i<FHead.PYCount; i++){
		if (strncmp(FPinYinList[i].PinYin, cPinYin, Len) == 0){
			if (WordNum == 0 || FPinYinList[i].Num == WordNum){
				if (FPinYinList[i].PinYin)
					FFilter[Result].subfilter[0] = FPinYinList[i].PinYin[0];
				FFilter[Result++].SongID = FPinYinList[i].SongID;
				if( Result % 400 == 0)
					AllocFFilterMemory(Result + 400);
			}
		}
	}
	AllocFFilterMemory(Result);
	return Result;
}

long CData::SetWBHFilterIndex(int PageNum)
{
	long Len = filter.length();
	PerPageNum = PageNum;
	if (Len == 0)
	{
		AllocFFilterMemory(FHead.PYCount);
		memset(FFilter, 0, FHead.PYCount * sizeof(FilterNode));
		for(long i=0; i<FHead.PYCount; i++)
		{
			if (FPinYinList[i].PinYin)
				FFilter[i].subfilter[0] = FPinYinList[i].PinYin[0];
			FFilter[i].SongID = FPinYinList[i].SongID;
		}
		return FHead.PYCount;
	}
	long Result = 0 ;
	AllocFFilterMemory( 400 );
	memset(FFilter, 0, 400 * sizeof(FilterNode));
	for(long i=0; i<FHead.PYCount; i++)
	{
		if (strncmp(FPinYinList[i].WBH, filter.data(), Len) == 0)
		{
			if (FPinYinList[i].PinYin)
				FFilter[Result].subfilter[0] = FPinYinList[i].PinYin[0];
			FFilter[Result++].SongID = FPinYinList[i].SongID;
			if( Result % 400 == 0)
				AllocFFilterMemory(Result + 400);
		}
	}
	AllocFFilterMemory(Result);
	return Result;
}

long CData::SetHandWriteIndex(int PageNum)
{
	long Result = 0;
	AllocFFilterMemory( 400 );
#if 1
	for(long i=0; i<FHead.PYCount; i++)
	{
		if ( strstr(SongLists[FPinYinList[i].SongID].SongName, filter.data() ) != NULL)
		{
			if (FPinYinList[i].PinYin)
				FFilter[Result].subfilter[0] = FPinYinList[i].PinYin[0];
			FFilter[Result++].SongID = FPinYinList[i].SongID;
			if (Result % 400 == 0)
				AllocFFilterMemory(Result + 400);
		}
	}
#else
	for(int i=0; i<FHead.SongCount; i++) {
		if ( strstr(SongLists[i].SongName, filter) != NULL ) {
			FFilter[Result].subfilter[0] = 0;
			FFilter[Result++].SongID = i;
			if (Result % 400 == 0)
					AllocFFilterMemory(Result + 400);
		}
	}
#endif
	AllocFFilterMemory(Result);
	return Result;
}

bool CData::Locate(const char *FieldName, long value)
{
	bool Result = false;
	long Position = 0;
	if(strcasecmp(FieldName, "num") == 0 ){
		for(int i=0; i<FilterSongCount; i++) {
			Position = FFilter[i].SongID;
			if (SongLists[Position].Num == value) {
				CurRecID = i;
				Result = true;
				break;
			}
		}
	}
	GetCurrentPage();
	return Result;
}

MemSongNode *CData::GetNameByCode(const char *Code)
{
	if (strlen(Code) <= 0) return false;
	for(int i=0; i<FHead.SongCount; i++){
		if( strcasecmp(SongLists[i].SongCode, Code) == 0)
			return SongLists + i;
	}
	return NULL;
}

#define STRMCAT(str, s1, s2) {           \
	if (s2) {                        \
		char tmpbuf[100];        \
		sprintf(tmpbuf, s1, s2); \
		strcat(str, tmpbuf);     \
	}                                \
}
char *CData::GetStringByCode(const char *Code)
{             
	if (Code == NULL) return NULL;

	MemSongNode *rec = GetNameByCode(Code);
	char msg[1024] = "";

	sprintf(msg, "id=%ld", rec->ID);
	STRMCAT(msg, "&code=%s"     , rec->SongCode);
	STRMCAT(msg, "&name=%s"     , rec->SongName);
	STRMCAT(msg, "&charset=%d"  , rec->Charset);
	STRMCAT(msg, "&language=%s" , rec->Language);
	STRMCAT(msg, "&singer=%s"   , rec->SingerName);
	STRMCAT(msg, "&volk=%d"     , rec->VolumeK);
	STRMCAT(msg, "&vols=%d"     , rec->VolumeS);
	STRMCAT(msg, "&num=%d"      , rec->Num);
	STRMCAT(msg, "&klok=%d"     , rec->Klok);
	STRMCAT(msg, "&sound=%d"    , rec->Sound);
	STRMCAT(msg, "&soundmode=%d", rec->SoundMode);
	STRMCAT(msg, "&type=%s"     , rec->StreamType);
	STRMCAT(msg, "&password=%ld", rec->Password);
	
	return strdup(msg);
}
#undef STRMCAT

void CData::AllocFFilterMemory(long Count)
{
	if (Count <=0 ) return;
	FFilter =  (FilterNode *) realloc(FFilter, Count * sizeof(FilterNode));
}

void CData::CreateNewCharFilerNode()
{
	int id;

	int memsize = 0;
	// 清空计数
	for (int i=0;i<26;i++)
		CharFilter[i].count = 0;

	// 分配数据
	for (int i=0; i< FilterSongCount; i++)
	{
		id = (FFilter+i)->subfilter[0] - 'A';
		if (id >=0 && id<26)
		{
			if (CharFilter[id].count % 200 == 0)
				CharFilter[id].Filters= (FilterNode *)realloc(CharFilter[id].Filters, \
					(CharFilter[id].count + 200) * sizeof(FilterNode));
			CharFilter[id].Filters[CharFilter[id].count] = FFilter[i];
			CharFilter[id].count++;
		}
	}
	// 整理内存
	for (int i=0; i<26; i++){
		if (CharFilter[i].count > 0)
		{
			CharFilter[i].Filters= (FilterNode *)realloc(CharFilter[i].Filters, \
				CharFilter[i].count * sizeof(FilterNode));
			memsize += CharFilter[i].count * sizeof(FilterNode);
		}
		else if (CharFilter[i].Filters)
		{
			free(CharFilter[i].Filters);
			CharFilter[i].Filters= NULL;
		}
	}
}

void CData::FreeHashNodes()
{
	if (!FHashTable) return;
	IndexNode *node;
	struct StrHashTable *SongNode;
	struct StrHashTable *TmpSNode;
	for (int i=0; i < HashLen; i++)
	{
		SongNode= FHashTable[i]->Next;
		while (SongNode != NULL)
		{
			TmpSNode = SongNode;
			SongNode = SongNode->Next;

			node = (IndexNode*)TmpSNode->data;
			if (node) {
				free(node->Filters);
				free(node);
			}
			free(TmpSNode->Key);
			free(TmpSNode);
		}
		free(FHashTable[i]);
	}
	free(FHashTable);
	FHashTable = NULL;
}
