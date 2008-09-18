#ifndef CREATEINDEX_H
#define CREATEINDEX_H

#include "data.h"

#define USESQLITEDB
#ifdef USESQLITEDB
#	include "sqlite.h"
#	define SQL_SINGER "SELECT * FROM PlySinger WHERE Visible= 1 ORDER BY Num DESC"
#	define SQL_HOT    "SELECT * FROM PlaySong_View ORDER BY PlayNum DESC, Code DESC"
#	define SQL_SONG   "SELECT * FROM PlaySong_View ORDER BY Num, PinYin, Name"
#	define SQL_INDEX  "SELECT * FROM index"
#endif

#define SAVE_FILE

typedef enum tagFieldID {F_CLASS, F_LANGUAGE, F_SINGER, F_SEX, F_NUM, F_HOT, F_NEWSONG, F_UNKOWN} FieldID;

typedef struct tagFieldClass {
	FieldID fieldid;
	int count;
	IndexNode *node;
	bool sort;
}FieldClass;

typedef struct tagSystemFields {
	char SongCode  [SongCodeLen];   // 歌曲编号
	union {
		char SongName  [SongNameLen];   // 歌曲名称
		char SingerName[SongNameLen];
	};
	union {
		char Class     [ClassLen]  ;// 语言
		char Sex       [ClassLen]  ;
	};
	char Language  [LanguageLen]   ;// 语言
	char SingerName1[SingerNameLen];// 歌星姓名
	char SingerName2[SingerNameLen];// 歌星姓名
	char SingerName3[SingerNameLen];// 歌星姓名
	char SingerName4[SingerNameLen];// 歌星姓名
	char PinYin[PinYinLen];
	char WBH[PinYinLen];
	int	PlayNum;
	uint8_t Charset;                // 字符集
	uint8_t VolumeK;                // 音量值
	uint8_t VolumeS;                // 音量值
	uint8_t Num;                    // 字数
	uint8_t Klok;                   // 原伴音轨
	uint8_t Sound;                  // 原唱音轨
	uint8_t SoundMode;              // 声音模式
	char StreamType[StreamTypeLen]; // 视频格式
//	long Password;
	uint8_t IsNewSong;              // 新歌推荐
}SystemFields;

#define GROUPSIZE 1000
typedef int codegroup[GROUPSIZE];

struct group {
	int id;
	codegroup codes;
};

class CodeHash{
public:
	CodeHash() {count=0; have=NULL;}
	~CodeHash() { if (have) free(have);}
	bool HaveCode(char *code, int id)
	{
		int x = atoi(code);
		int a = x / GROUPSIZE;
		int b = x % GROUPSIZE;
		int i;
		for (i=0;i<count;i++)
		{
			if (have[i].id == a)
			{
				if (have[i].codes[b])
					return true;
				break;
			}
		}
		if (i>=count)
		{
			count++;
			have = (struct group *)realloc(have, count*sizeof(struct group));
			memset(have+(count-1), 0, sizeof(struct group));
			have[i].id = a;
		}
		have[i].codes[b] = id;
		return false;
	}
	int FindCode(const char *Code, int Delete)
	{
		int x = atoi(Code);
		return FindCode(x, Delete);
	}
	int FindCode(int iCode, int Delete)
	{
		group key;
		int a = iCode / GROUPSIZE;
		int b = iCode % GROUPSIZE;
		key.id = a;

		group *p =(group *)bsearch(&key, have, count, sizeof(struct group), qcompar);
		if (p)
			return p->codes[b];
		else
			return -1;
	}
	static int qcompar(const void *p1, const void *p2)
	{
		group *g1 = (group*)p1;
		group *g2 = (group*)p2;
		return g1->id - g2->id;
	}
	void SortGroup()
	{
		qsort(have, count, sizeof(group), qcompar);
	}

	void print()
	{
		for (int i=0;i<count;i++)
			printf("(%d) id=%d,count=%d\n", i, have[i].id, 0);
		printf("print end\n");
	}
private:
	int count;
	group *have;
};

class CKeywordIndex // 关键字索引类
{
public:
	CKeywordIndex();
	~CKeywordIndex();
	FieldClass* AddIndexNode(const char *IndexName, FieldID id);
	void AddSongData(SystemFields *data, bool check=true);
	void printIndex();
	void printSong();
	bool readdata(SystemFields *data);
	IndexNode *FindIndexNode(FieldClass *Class, const char *IndexName);
	void FieldClassSort();
	void SaveFile(const char *url);
	void SaveTo(int fp);
	void CodeHashSort() { codehash.SortGroup();}
protected:
	CodeHash codehash;
	void AddIndexNodeFilter(IndexNode *indexnode, FilterNode node);
private:
	int ClassCount;
	FieldClass *ClassList;
	int SongCount;
#ifdef SAVE_FILE
	FILE *tmpfp;
#else
	SongListNode *SongLists;
#endif
	int IndexCount;
	tagHeader FHead;

	int PinyinCount;
	Pinyin_t *PinyinList;

	void Data2Data(SongListNode* To, SystemFields* From);
};

class CSongIndex:public CKeywordIndex
{
public:
	void AddHotSongData(SystemFields *data);
	CSongIndex():CKeywordIndex()
	{
		char tmp[20];
		for (int i=0;i<10;i++) {
			sprintf(tmp, "%d字歌", i);
			AddIndexNode(tmp, F_NUM);
		}
		AddIndexNode("国语"    , F_LANGUAGE);
		AddIndexNode("粤语"    , F_LANGUAGE);
		AddIndexNode("台语"    , F_LANGUAGE);
		AddIndexNode("英语"    , F_LANGUAGE);
		AddIndexNode("日韩"    , F_LANGUAGE);
		AddIndexNode("其他"    , F_LANGUAGE);

		AddIndexNode("生日喜庆", F_CLASS);
		AddIndexNode("Disco"   , F_CLASS);
		AddIndexNode("戏曲"    , F_CLASS);
		AddIndexNode("民歌"    , F_CLASS);
		AddIndexNode("电影"    , F_CLASS);
		AddIndexNode("合唱"    , F_CLASS);
		AddIndexNode("革命"    , F_CLASS);
		AddIndexNode("怀旧"    , F_CLASS);

		AddIndexNode("新歌推荐", F_NEWSONG);
		FieldClass *HotField = AddIndexNode("热门歌曲" , F_HOT);
		if (HotField)
			HotIndexNode = FindIndexNode(HotField, "热门歌曲");
		else
			HotIndexNode = NULL;
	}
private:
	IndexNode  *HotIndexNode;

};

class CSingerIndex: public CKeywordIndex
{
public:
	CSingerIndex():CKeywordIndex()
	{
		AddIndexNode("港台男"  , F_SEX);
		AddIndexNode("港台女"  , F_SEX);
		AddIndexNode("大陆男"  , F_SEX);
		AddIndexNode("大陆女"  , F_SEX);
		AddIndexNode("乐队"    , F_SEX);
		AddIndexNode("国外"    , F_SEX);
	}
};

#endif /*CREATEINDEX_H_*/
