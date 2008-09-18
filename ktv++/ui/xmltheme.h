/*==============================================================================
   T h e   K T V L i n u x   P r o j e c t
 -------------------------------------------------------------------------------
   Filename   : xmltheme.h
   Author(s)  : Silicon
   Copyright  : Silicon

        �� CKtvTheme ʵ�ֶԽ�������ķ�װ
==============================================================================*/

//==============================================================================
#ifndef XMLTHEME_H
#define XMLTHEME_H

#ifdef WIN32
	#include <windows.h>
#endif
#include "ktvstdclass.h"
#include "strext.h"
#include "mtvconfig.h"
#include "font.h"
#include "utility.h"
#include "color.h"
#include "data.h"

//==============================================================================
#define BUF_LEN 255

#define COLORCOMP(c1, c2) ((c1.a ==c2.a) && (c1.r==c2.r) && (c1.g==c2.g) && (c1.b==c2.b))
#define rgb2uint32(r,g,b) (b<<16 | g<<8 | r)
#define COLOR2UINT32(cl) (cl.b<<16 |cl.g<<8 | cl.r)
#define POINT_IN_RECT(x, y, r,len) ((r.left+len<x) && (r.right-len>x) && (r.top+len < y) && (r.bottom-len>y))


extern const char *MainFormStr;
extern const char *SingerClassFrmStr;
extern const char *YuYanFormStr;
extern const char *ClassFormStr;
extern const char *WordNumFormStr;
extern const char *SelectedFormStr;
extern const char *OtherFormStr;
extern const char *PinYinFormStr;
extern const char *MyLoveFormStr;
extern const char *WBHFormStr;
extern const char *SongListFormStr;
extern const char *InputCodeFormStr;
extern const char *NumLocateFormStr;
extern const char *SingerFormStr;
extern const char *SoftInfoFormStr;
extern const char *PlunginsFormStr;

#ifdef ENABLE_FOODMENU
extern const char *FoodMenuFormStr;
extern const char *MenuTypeFormStr;
extern const char *MenuLoginFormStr;
#endif

#ifdef ENABLE_HANDWRITE
extern const char *HandWriteFormStr;
#endif

#ifdef ENABLE_MULTLANGIAGE
extern const char *SwitchLangFormStr;
#endif

extern const char *PlunginsPath;
extern const char *SingerPhotoPath;

#ifndef WIN32
typedef struct tagRECT{
	int left;
	int top;
	int right;
	int bottom;
} RECT, *PRECT, *LPRECT;

#endif

//==============================================================================
typedef enum tagTextLayout{
	taLeftJustify,  // �����
	taRightJustify, // �Ҷ���
	taCenter,       // ���ж���
	taTop,          // �϶���
	taBottom,       // �¶���
} TTextLayout;

typedef struct tagAlign{
	TTextLayout align;  // ˮƽ����
	TTextLayout valign; // ��ֱ����
}TAlign;

//#define OPTSORT

#ifdef OPTSORT
typedef enum tagSortType{
	sortOther = 0,
	sortName  = 1,
	sortKey   = 2,
	sortRect  = 3
} SortType; // ��������
#endif

class CWindowStack;
class CBaseGui;

class CBaseObject: public CListObject
{
public:
	bool StartTimer(const uint8_t timerID, const uint32_t dwInterval);
	bool KillTimer(const uint8_t timerID);
	virtual void TimerWork(){}
	virtual ~CBaseObject() {}
};

class CKtvOption :public CBaseObject
{
public:
	CKtvOption(): font(NULL),sys_value(0),mtv_value(0)
	{
		name = "";
		title = "";
		link = "";
		filter = "";
		keyname = "";
		rect.top = rect.left = rect.bottom = rect.right = 0;
		tag = 0;
		activecolor = argb2color(0,255,0,0);
		sys_value = mtv_value = 0;
		tagfilter = 0;
		align.align  = taLeftJustify;
		align.valign = taTop;
	}
	CKtvOption(const char *Name)
	{
		CKtvOption();
		name = Name;
	}
	~CKtvOption() {}
#ifdef DEBUG
	void PrintOption();
#endif
	long tag;
	CKtvFont *font;
	TColor activecolor;
	char tagfilter;
	TAlign align;

	int32_t sys_value;
	char mtv_value;
	RECT rect;
	CString name;
	CString title;
	CString link;
	CString filter;
	CString keyname;
};

class CKtvWindow: public CBaseObject
{
public:
	CKtvWindow(const char *name);
	virtual ~CKtvWindow();                     // �ͷŴ���
	CKtvOption *FindOption(const char *name);  // ����ָ�����������ƵĲ�����
	CKtvOption *FindOption(int x, int y);      // ����ָ����������괦������
	CKtvOption *FindOption(int32_t sys_value); // �����̰�����Ӧ����Ĳ�����
	RECT ScreenToClient (RECT rect);           // ����Ļ�ľ���ַת���ɱ��������Ե�ַ
#ifdef OPTSORT
	void CreateSortOpt  (SortType newsort);    // �������������������
#endif
	void AddOption      (CKtvOption *newopt);  // �����²�����
	void CleanWindow    ();                    // ��մ���
	void SetImageBuffer (void *data, long len);// ���ñ���������
#ifdef DEBUG
	void PrintOptList();                       // ��ӡ���в�����
#endif
	CString WinName;                           // ��������
	CKtvFont *font;                            // ���������
	RECT     workrect;                         // ����
protected:
	void FreeImageBuffer();
	unsigned char *ImageBuffer;                // ����ͼ������
	long ImageBufferSize;                      // ����ͼ��������С
	void *EmptyShadow;                         // ����ͼӰ��
	CKtvOption *CurOption;                     // ��ǰ������
	CWindowStack *stack;                       // ��ջ
	CBaseGui *gui;	                           // ͼ�νӿ�
private:
#ifdef OPTSORT
	CKtvOption  **sortopt;                     // �������������
	int         opt_num;                       // ����������
	SortType optsorttype;                      // ���������������
	void SortByOpt(SortType newsort);          // ������Ĳ�������Ƶ���ĸ˳����
#endif
	CList<CKtvOption> OptionList;
};

typedef struct tagFileList{
	char filename[21]; // �ļ���
	long position;     // ��λ
	long size;         // �ļ���С
}FileList;

#define MAX_FILES 40
typedef struct tagPlungHead{
	char magic[11];               // ��־λ
	int  filecount;               // �ļ�����
	FileList filelist[MAX_FILES]; // �ļ���������
}PlungHead;

class CBaseWindow;

//==============================================================================
class CKtvTheme // ������Ʋ�����
{
public:
	CKtvTheme(CMtvConfig *configure);

	~CKtvTheme();
	CMtvConfig *config;
	CKtvWindow *CommonWindow; // ͨ�ò�����ֵ
	CData *songdata;
	CData *singerdata;
	bool LoadThemeFromFile(CString& themefile); // �����������ļ�
	CKtvFont *FindFont(const char *fontaliasname){return fonts->CreateFont(fontaliasname);}
	void *DownSingerPhoto(const char *singername, size_t& len);
	CKtvWindow *FindWindow(const char *WinName);
private:
	CBaseWindow* CreateWindow(const char *winname);
	void CreateAllWindow();
	void LoadWindowData();
	bool CreateKtvWindow(CKtvWindow *window, TiXmlElement *win=NULL);
	void CloseTheme();                                         // ������ݻ���
	int GetSegIndex     (const char *SegName);                 // �õ����ڶ��б��е�λ��
	void *ReadSegment   (const char *SegName, long *datasize); // �ӽ����ж�ȡ����
	void *FindXmlWindow (CString& winname);                    // ���Ҵ���
private:
	CMemoryStream *DataStream; // ������
	CList<CKtvWindow> WindowList;

	PlungHead head; // �����ļ�ͷ��Ϣ
	void *docfp;    // TiXmlDocument
	void *Memory;   // �����ݵĻ���
	int MemSize;    // �����С

	CFontFactory *fonts;
	CDiskCache* DiskCache;
};

//==============================================================================
#endif

