/*==============================================================================
   T h e   K T V L i n u x   P r o j e c t
 -------------------------------------------------------------------------------
   Filename   : xmltheme.h
   Author(s)  : Silicon
   Copyright  : Silicon

        类 CKtvTheme 实现对界面主题的封装
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
	taLeftJustify,  // 左对齐
	taRightJustify, // 右对齐
	taCenter,       // 具中对齐
	taTop,          // 上对齐
	taBottom,       // 下对齐
} TTextLayout;

typedef struct tagAlign{
	TTextLayout align;  // 水平方向
	TTextLayout valign; // 垂直方向
}TAlign;

//#define OPTSORT

#ifdef OPTSORT
typedef enum tagSortType{
	sortOther = 0,
	sortName  = 1,
	sortKey   = 2,
	sortRect  = 3
} SortType; // 排序类型
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
	virtual ~CKtvWindow();                     // 释放窗体
	CKtvOption *FindOption(const char *name);  // 返回指定操作项名称的操作项
	CKtvOption *FindOption(int x, int y);      // 返回指定窗体的坐标处操作项
	CKtvOption *FindOption(int32_t sys_value); // 将键盘按键对应窗体的操作项
	RECT ScreenToClient (RECT rect);           // 将屏幕的绝对址转换成本窗体的相对地址
#ifdef OPTSORT
	void CreateSortOpt  (SortType newsort);    // 建立排序操作项索引表
#endif
	void AddOption      (CKtvOption *newopt);  // 增加新操作项
	void CleanWindow    ();                    // 清空窗体
	void SetImageBuffer (void *data, long len);// 设置背景缓冲区
#ifdef DEBUG
	void PrintOptList();                       // 打印所有操作项
#endif
	CString WinName;                           // 窗体名称
	CKtvFont *font;                            // 窗体的字体
	RECT     workrect;                         // 坐标
protected:
	void FreeImageBuffer();
	unsigned char *ImageBuffer;                // 背景图缓冲区
	long ImageBufferSize;                      // 背景图缓冲区大小
	void *EmptyShadow;                         // 背景图影子
	CKtvOption *CurOption;                     // 当前操作项
	CWindowStack *stack;                       // 堆栈
	CBaseGui *gui;	                           // 图形接口
private:
#ifdef OPTSORT
	CKtvOption  **sortopt;                     // 排序操作项索引
	int         opt_num;                       // 操作项数量
	SortType optsorttype;                      // 操作项的排序类型
	void SortByOpt(SortType newsort);          // 将窗体的操作项按名称的字母顺排列
#endif
	CList<CKtvOption> OptionList;
};

typedef struct tagFileList{
	char filename[21]; // 文件名
	long position;     // 定位
	long size;         // 文件大小
}FileList;

#define MAX_FILES 40
typedef struct tagPlungHead{
	char magic[11];               // 标志位
	int  filecount;               // 文件数量
	FileList filelist[MAX_FILES]; // 文件属性数组
}PlungHead;

class CBaseWindow;

//==============================================================================
class CKtvTheme // 界面控制操作类
{
public:
	CKtvTheme(CMtvConfig *configure);

	~CKtvTheme();
	CMtvConfig *config;
	CKtvWindow *CommonWindow; // 通用操作项值
	CData *songdata;
	CData *singerdata;
	bool LoadThemeFromFile(CString& themefile); // 读界面主题文件
	CKtvFont *FindFont(const char *fontaliasname){return fonts->CreateFont(fontaliasname);}
	void *DownSingerPhoto(const char *singername, size_t& len);
	CKtvWindow *FindWindow(const char *WinName);
private:
	CBaseWindow* CreateWindow(const char *winname);
	void CreateAllWindow();
	void LoadWindowData();
	bool CreateKtvWindow(CKtvWindow *window, TiXmlElement *win=NULL);
	void CloseTheme();                                         // 清除数据缓冲
	int GetSegIndex     (const char *SegName);                 // 得到段在段列表中的位置
	void *ReadSegment   (const char *SegName, long *datasize); // 从界面中读取数据
	void *FindXmlWindow (CString& winname);                    // 查找窗体
private:
	CMemoryStream *DataStream; // 数据流
	CList<CKtvWindow> WindowList;

	PlungHead head; // 界面文件头信息
	void *docfp;    // TiXmlDocument
	void *Memory;   // 读数据的缓冲
	int MemSize;    // 缓冲大小

	CFontFactory *fonts;
	CDiskCache* DiskCache;
};

//==============================================================================
#endif

