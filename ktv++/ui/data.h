/*==============================================================================
   T h e   K T V L i n u x   P r o j e c t
 -------------------------------------------------------------------------------
   Filename   : data.h
   Author(s)  : Silicon
   Copyright  : Silicon

   实现对数据表的封装，歌曲列表，歌手资料列表都在这个单元中实现
==============================================================================*/

/*============================================================================*/
#ifndef DATA_H
#define DATA_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "avio/avio.h"
#include "ktvstdclass.h"  // KTV 的标准类库
#include "utility.h"

#define FilterLen 16

#define SongCodeLen   9
#define SongNameLen   51
#define ClassLen      10
#define LanguageLen   9
#define SingerNameLen 19
#define PinYinLen     10
#define StreamTypeLen 5

/*============================================================================*/
// tagMemSongNode 是内存歌曲结构体
typedef struct tagHeader{
	long IndexCount;      // 索引数量
	long DataPosition;    // 数据区位置
	long PyPosition;      // 拼音区位置
	long SongCount;       // 记录数量
	long PYCount;         // 拼音数量
}Header_t;

typedef struct tagMemSongNode{
	long ID;
	char *SongCode;       // 歌曲编号
	char *SongName;       // 歌曲名称
	unsigned char Charset;// 字符集
	char *Language;       // 语言
	char *SingerName;     // 歌星姓名
	unsigned char VolumeK;// 音量值
	unsigned char VolumeS;// 音量值
	char Num;             // 字数
	char Klok;            // 原伴音轨
	char Sound;           // 原唱音轨
	char SoundMode;       // 声音模式
	char *StreamType;     // 流格式
	long Password;        //
}MemSongNode;

struct tagMemPinYin {         // 内存表中的拼音表
	char *PinYin;         // 拼音
	char *WBH;            // 五笔划
	char Num;             // 字数
	long SongID;          // 歌曲坐标号
};

typedef struct tagSongListNode
{
	char SongCode[SongCodeLen];
	char SongName[SongNameLen];
	char Language[LanguageLen];
	char SingerName[SingerNameLen];
	char Charset;
	unsigned char VolumeK;
	unsigned char VolumeS;
	char Num;
	char Klok;
	char Sound;
	char SoundMode;
	char StreamType[StreamTypeLen];
	long Password;
}SongListNode;

typedef struct tagPinYin{
	char PinYin[FilterLen];
	char WBH[FilterLen];
	char Num;
	long SongID;
}Pinyin_t;

typedef struct tagSongPage {   // 当前页
	int Count;                 // 当前页歌曲数
	MemSongNode **RecList;     // 当前歌曲列表指针
}SongPage;

typedef struct tagFilterNode { // 一个过滤项结点
	long SongID;               // 在内存歌表数组中的下标
	char subfilter[1];         // 子过滤
} FilterNode;

typedef struct tagIndexNode {  // 过滤项总表
	char IndexName[FilterLen];
	int count;                 // 过滤项数量
	FilterNode *Filters;       // 过滤项数组
}IndexNode;

inline void FreeMemSongNode(MemSongNode *node);

/*==============================================================================
 *  CData: 数据表类, 实现对数据库的所有操作都在这里面完成
 *============================================================================*/
class CData: public CStrHash
{
public:
	SongPage CurPage;
	tagHeader FHead;           // 数据库头
	MemSongNode *SongLists;    // 所有歌曲

	CData(void);
	CData(const char *FileName, bool LoadMemory=true);
	~CData(void);

	bool LoadDataDB(const char *FileName, bool LoadMemory);
	void FreeDataDB(void);                                    // 释放资源
	void FirstPage (void);                                    // 到第一页
	void EndPage   (void);                                    // 到最后一页
	bool NextPage  (void);                                    // 下一页
	void PriorPage (void);                                    // 上一页
	bool Bof       (void);                                    // 是否到了最上面一页
	bool Eof       (void);                                    // 是否到了最后面一页
	bool Locate(const char *FieldName, long value);           // 数字定位
	MemSongNode *GetNameByCode(const char *Code);             // 得到指定歌曲编号的歌曲记录
	char *GetStringByCode(const char *Code);
	void SetPerPageNum(int pagenum);                          // 设置每页显示的行数
	long GetCurPageNo(void);                                  // 得到当前页号
	long TotalPageCount;                                      // 总页数
	CString FilterDescription;                                // 过滤项的标题名称
	void SetFilterName(CString& name, CString& desc,int id=0);// 设置过滤条件
	long ActiveFilter(int PageNum, bool update=false);        // 使过滤条件生效
	long PinYinFilter(CString& pinyin);                       // 在已设置的守滤条件下,再守滤
	void Reload();
protected:
	long FilterSongCount;      // 当前过滤条件下，歌曲总数量
	tagMemPinYin *FPinYinList; // 拼音表
	int PerPageNum;            // 每页显示多少项
	long CurRecID;             // 当前记录号
	FilterNode *FFilter;       // 当前内存过滤项
	IndexNode CharFilter[26];  // 字母过滤项表
	CString filter;            // 过滤条件
	int filterclass;           // 过滤类型  0:分类索引 1:拼音索引

	bool CreateSeekTable   (const char *FileName, bool LoadMemory);// 建立歌曲索引表
	void AllocFFilterMemory(long Count);   // 申请数据表空间
	void GetCurrentPage    (void);         // 读最当前页数据
	long SetFilterIndex    (int PageNum);  // 创建索引记录表，PageNum为每页记录数
	long SetPYFilterIndex  (int PageNum);  // 创建拼音记录表，PageNum为每页记录数
	long SetWBHFilterIndex (int PageNum);  // 创建五笔划记录表，PageNum为每页记录数
	long SetHandWriteIndex (int PageNum);  // 创建手写记录表，PageNum为每页记录数
	void CreateNewCharFilerNode();         // 根据 FFilter 建立字母过滤项
	void FreeHashNodes();
private:
	CString DataBaseFile;
};

//==============================================================================
#endif
