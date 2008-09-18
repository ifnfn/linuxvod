/*==============================================================================
   T h e   K T V L i n u x   P r o j e c t
 -------------------------------------------------------------------------------
   Filename   : data.h
   Author(s)  : Silicon
   Copyright  : Silicon

   ʵ�ֶ����ݱ�ķ�װ�������б����������б��������Ԫ��ʵ��
==============================================================================*/

/*============================================================================*/
#ifndef DATA_H
#define DATA_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "avio/avio.h"
#include "ktvstdclass.h"  // KTV �ı�׼���
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
// tagMemSongNode ���ڴ�����ṹ��
typedef struct tagHeader{
	long IndexCount;      // ��������
	long DataPosition;    // ������λ��
	long PyPosition;      // ƴ����λ��
	long SongCount;       // ��¼����
	long PYCount;         // ƴ������
}Header_t;

typedef struct tagMemSongNode{
	long ID;
	char *SongCode;       // �������
	char *SongName;       // ��������
	unsigned char Charset;// �ַ���
	char *Language;       // ����
	char *SingerName;     // ��������
	unsigned char VolumeK;// ����ֵ
	unsigned char VolumeS;// ����ֵ
	char Num;             // ����
	char Klok;            // ԭ������
	char Sound;           // ԭ������
	char SoundMode;       // ����ģʽ
	char *StreamType;     // ����ʽ
	long Password;        //
}MemSongNode;

struct tagMemPinYin {         // �ڴ���е�ƴ����
	char *PinYin;         // ƴ��
	char *WBH;            // ��ʻ�
	char Num;             // ����
	long SongID;          // ���������
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

typedef struct tagSongPage {   // ��ǰҳ
	int Count;                 // ��ǰҳ������
	MemSongNode **RecList;     // ��ǰ�����б�ָ��
}SongPage;

typedef struct tagFilterNode { // һ����������
	long SongID;               // ���ڴ��������е��±�
	char subfilter[1];         // �ӹ���
} FilterNode;

typedef struct tagIndexNode {  // �������ܱ�
	char IndexName[FilterLen];
	int count;                 // ����������
	FilterNode *Filters;       // ����������
}IndexNode;

inline void FreeMemSongNode(MemSongNode *node);

/*==============================================================================
 *  CData: ���ݱ���, ʵ�ֶ����ݿ�����в����������������
 *============================================================================*/
class CData: public CStrHash
{
public:
	SongPage CurPage;
	tagHeader FHead;           // ���ݿ�ͷ
	MemSongNode *SongLists;    // ���и���

	CData(void);
	CData(const char *FileName, bool LoadMemory=true);
	~CData(void);

	bool LoadDataDB(const char *FileName, bool LoadMemory);
	void FreeDataDB(void);                                    // �ͷ���Դ
	void FirstPage (void);                                    // ����һҳ
	void EndPage   (void);                                    // �����һҳ
	bool NextPage  (void);                                    // ��һҳ
	void PriorPage (void);                                    // ��һҳ
	bool Bof       (void);                                    // �Ƿ���������һҳ
	bool Eof       (void);                                    // �Ƿ��������һҳ
	bool Locate(const char *FieldName, long value);           // ���ֶ�λ
	MemSongNode *GetNameByCode(const char *Code);             // �õ�ָ��������ŵĸ�����¼
	char *GetStringByCode(const char *Code);
	void SetPerPageNum(int pagenum);                          // ����ÿҳ��ʾ������
	long GetCurPageNo(void);                                  // �õ���ǰҳ��
	long TotalPageCount;                                      // ��ҳ��
	CString FilterDescription;                                // ������ı�������
	void SetFilterName(CString& name, CString& desc,int id=0);// ���ù�������
	long ActiveFilter(int PageNum, bool update=false);        // ʹ����������Ч
	long PinYinFilter(CString& pinyin);                       // �������õ�����������,������
	void Reload();
protected:
	long FilterSongCount;      // ��ǰ���������£�����������
	tagMemPinYin *FPinYinList; // ƴ����
	int PerPageNum;            // ÿҳ��ʾ������
	long CurRecID;             // ��ǰ��¼��
	FilterNode *FFilter;       // ��ǰ�ڴ������
	IndexNode CharFilter[26];  // ��ĸ�������
	CString filter;            // ��������
	int filterclass;           // ��������  0:�������� 1:ƴ������

	bool CreateSeekTable   (const char *FileName, bool LoadMemory);// ��������������
	void AllocFFilterMemory(long Count);   // �������ݱ�ռ�
	void GetCurrentPage    (void);         // ���ǰҳ����
	long SetFilterIndex    (int PageNum);  // ����������¼��PageNumΪÿҳ��¼��
	long SetPYFilterIndex  (int PageNum);  // ����ƴ����¼��PageNumΪÿҳ��¼��
	long SetWBHFilterIndex (int PageNum);  // ������ʻ���¼��PageNumΪÿҳ��¼��
	long SetHandWriteIndex (int PageNum);  // ������д��¼��PageNumΪÿҳ��¼��
	void CreateNewCharFilerNode();         // ���� FFilter ������ĸ������
	void FreeHashNodes();
private:
	CString DataBaseFile;
};

//==============================================================================
#endif
