/*==============================================================================
 *  T h e   K T V L i n u x   P r o j e c t
 *------------------------------------------------------------------------------
 *	Filename   : win.h
 *	Author(s)  : Silicon
 *	Copyright  : Silicon
 *
 *	从基类(CMtvBaseWindow)派生出来的子类存在这个单元中，已重新封装的类有：
 *	CMainWindow      : 主窗体类,重载虚拟函数Close(),禁止窗体关闭
 *	CInputCodeWindow : 编号点歌类
 *	CNumLocateWindow : 点歌列表中的字数定位类
 *	CPinYinWindow    : 拼音点歌类
 *	CDataBaseWindow  : 带有数据表操作的基类
 *	CSingerDataWindow: 歌星列表类
 *	CSongDataWindow  : 歌曲列表类
 *
 *============================================================================*/
#ifndef WIN_H
#define WIN_H

#include <sys/timeb.h>

#include "windowmanage.h"

#ifdef ENABLE_HANDWRITE
#include "hw/hwim.h"
#endif

//==============================================================================
class CThemeWindow: public CBaseWindow // 换肤
{
public:
	CThemeWindow(const char *name):CBaseWindow(name), ShowOpt(NULL){}
	~CThemeWindow() {}
	virtual void LoadTheme(CKtvTheme *ptheme);
	virtual bool InputProcess(InputEvent *event); // 输入事件处理函数
protected:
	virtual void DrawWindowOpt();                 // 准备窗体需要写字的部分
private:
	CKtvOption *ShowOpt;
	int themeid;
	void DrawTheme(int move); // move=0;显示当前的,-1显示上一个, 1显示下一个
};

class CSwitchLangWindow: public CBaseWindow // 语言切换
{
public:
	CSwitchLangWindow(const char *name):CBaseWindow(name){}
	~CSwitchLangWindow() {}
	virtual bool InputProcess(InputEvent *event); // 输入事件处理函数
};

//==============================================================================
class CSoftInfoWindow: public CBaseWindow
{
public:
	CSoftInfoWindow(const char *name): CBaseWindow(name) {}
};

//==============================================================================
class CMainWindow: public CBaseWindow
{
public:
	CMainWindow(const char *name):CBaseWindow(name){}
	void HotSong() {
		CKtvOption* tmp = FindOption("num7");
		RunLink(tmp);
	}
	virtual void Close(){}
};

//==============================================================================
class CInputWindow: public CBaseWindow // 输入框基类
{
public:
	CInputWindow(const char *name):CBaseWindow(name), \
		activeopt(0), OptCount(0), Opt(NULL){}
	virtual ~CInputWindow() {
		free(Opt);
	}

	virtual void LoadTheme(CKtvTheme *ptheme);
	virtual bool InputProcess(InputEvent *event); // 输入事件处理函数
	virtual void Show();                          // 显示函数，将窗体变成激活窗体时需要完成的操作
protected:
	virtual void DrawWindowOpt();                 // 准备窗体需要写字的部分
	int activeopt;
	int OptCount;
	CKtvOption **Opt;
};

//==============================================================================
class CInputCodeWindow: public CInputWindow // 编号点歌类
{
public:
	CInputCodeWindow(const char *name): CInputWindow(name), rec(NULL), foundsong(false){}
	virtual bool InputProcess(InputEvent *event); // 输入事件处理函数
protected:
	virtual void DrawWindowOpt(); // 准备窗体需要写字的部分
private:
	MemSongNode *rec;             // 已找到的歌曲资料
	bool foundsong;               // 是否找到歌曲
};

//==============================================================================
class CMyLoveWindow: public CInputWindow // 我的最爱点歌类
{
public:
	CMyLoveWindow(const char *name):\
		CInputWindow(name), loadorsave(0){}
	virtual bool InputProcess(InputEvent *event); // 输入事件处理函数
protected:
	virtual void DrawWindowOpt();                 // 准备窗体需要写字的部分
	virtual void SetFilter(CString& name, CString& desc,int id=0);
private:
	int loadorsave;                               // 载入或者保存
	void SaveMyLove(CString& user, CString& pwd); // 保存我的最爱
	void LoadMylove(CString& user, CString& pwd); // 载入我的最爱
};

//==============================================================================
class CNumLocateWindow: public CBaseWindow // 字类定位类
{
public:
	CNumLocateWindow(const char *name):CBaseWindow(name){}
	virtual bool InputProcess(InputEvent *event);
};

//==============================================================================
class CPinYinWindow: public CBaseWindow // 拼音点歌类
{
public:
	CPinYinWindow(const char *name);
	~CPinYinWindow();
	virtual void LoadTheme(CKtvTheme *ptheme);
	virtual bool InputProcess(InputEvent *event); // 输入事件处理函数
	virtual void Show();   // 显示函数，将窗体变成激活窗体时需要完成的操作
protected:
	virtual void DrawWindowOpt();                 // 准备窗体需要写字的部分
	virtual void Restore();
private:
	CKtvOption *EnterOpt;
	CKtvOption *PyLabelOpt;         // 显示拼音字母
	int PyLength;                   // 拼音字母长度
	CKtvOption *board[3];           // 三个框框
	CKtvOption **NumKeyOpt;         // 数字键操作项
	CKtvOption **CharKeyOpt[3];     // 字母分组
	int NumKeyOptCount;             // 数字键数量
	int activeboard;                // 当前激活框
	void SetActiveNumKeyOpt();      // 设置新的数字键
};

//==============================================================================
class CSelectedWindow: public CBaseWindow // 已点歌曲列表类
{
public:
	CSelectedWindow(const char *name);
	~CSelectedWindow();
	virtual void LoadTheme(CKtvTheme *ptheme);
	virtual bool InputProcess(InputEvent *event); // 输入事件处理函数
	virtual void Show();                          // 显示函数，加入页码初始化等部分
protected:
	virtual void AcitveEvent();                   // 窗体激活事件
	virtual void DrawWindowOpt();                 // 准备窗体需要写字的部分
	virtual void SetFilter(CString& name, CString& desc,int id=0);
                                                   // 设置查找条件
private:
	int SongOptCount;                             // 歌曲列表数量
	CKtvOption **SongListOpt;                     // 歌曲列表操作项
	int SingerOptCount;                           // 歌星列表数量
	CKtvOption **SingerListOpt;                   // 歌星列表操作项
	CKtvOption *msgopt;                           // 显示标题的操作项
	CKtvOption *pageopt;                          // 显示页号的操作项
	int optstate;                                 // 操作状态: 优先删除

	int TotalPageCount;                           // 总页数
	int CurPageID;                                // 当前页号
	void DrawPageText();                          // 显示页号文字
	void DrawMsgText(const char *msg=NULL);       // 显示标题文字

	void FirstPage();                             // 到第一页
	void EndPage  ();                             // 到最后一页
	bool NextPage ();                             // 下一页
	void PriorPage();                             // 上一页
	bool Bof();                                   // 是否到了最上面一页
	bool Eof();                                   // 是否到了最后面一页
};

//==============================================================================
class CDataBaseWindow: public CBaseWindow
{
public:
	CDataBaseWindow(const char *name);
	~CDataBaseWindow();
	virtual void LoadTheme(CKtvTheme *ptheme);
	virtual void Show();                                    // 显示函数，将窗体变成激活窗体并入栈
	virtual bool InputProcess(InputEvent *event);           // 输入事件处理函数
protected:
	int SongOptCount;                                       // 歌曲列表数量
	CKtvOption **SongListOpt;                               // 歌曲列表操作项
	int SingerOptCount;                                     // 歌星列表数量
	CKtvOption **SingerListOpt;                             // 歌星列表操作项
	CString FilterDescription;                              // 过滤项的标题描述
	CString subfilterdesc;                                  // 子过滤项标题描述
	CString filter;                                         // 过滤条件

	virtual void SetFilter(CString& name, CString& desc,int id=0); // 设置查找条件
	virtual void DrawWindowOpt();                           // 准备窗体需要写字的部分
	void ShowActiveChar();                                  // 激活当前字母操作项
private:
	int filterclass;                                        // 过滤类型  0:分类索引 1:拼音索引
	int activeid;                                           // 当前激活的字母
	int CharOptCount;                                       // 字母数量
	CKtvOption **CharListOpt;                               // 字母操作项
	CKtvOption *msgopt;                                     // 显示标题的操作项
	CKtvOption *pageopt;                                    // 显示页号的操作项
};

//==============================================================================
class CWBHWindow: public CDataBaseWindow // 五笔划点歌类
{
public:
	CWBHWindow(const char *name):CDataBaseWindow(name){}
	virtual void Show();                          // 显示函数，将窗体变成激活窗体并入栈
	virtual bool InputProcess(InputEvent *event); // 输入事件处理函数
};

//==============================================================================
class CSongDataWindow: public CDataBaseWindow     // 歌曲列表类
{
public:
	CSongDataWindow(const char *name):CDataBaseWindow(name), CurSelectSong(NULL){}
	virtual bool InputProcess(InputEvent *event); // 输入事件处理函数
private:
	SelectSongNode *CurSelectSong;
};

//==============================================================================
class CSingerDataWindow: public CDataBaseWindow // 歌星列表类
{
public:
	CSingerDataWindow(const char *name): CDataBaseWindow(name) {}
	virtual void LoadTheme(CKtvTheme *ptheme);
	virtual bool InputProcess(InputEvent *event);     // 输入事件处理函数
protected:
	virtual void DrawWindowOpt();                     // 准备窗体需要写字的部分
	bool DrawSingerPhoto(char *fn, const RECT pRect); // 显示歌星图片
};
//==============================================================================

#ifdef ENABLE_HANDWRITE
class CHandWriteWindow: public CBaseWindow
{
public:
	CHandWriteWindow(const char *name);
	~CHandWriteWindow();
	virtual void LoadTheme(CKtvTheme *ptheme);
	virtual bool InputProcess(InputEvent *event);
	virtual void Show();
protected:
	virtual void DrawWindowOpt();
private:
	void DisplayHwOut();
	void WriteFinish(CKtvOption *opt, bool ok);
	CKtvOption *Input[2];
	CKtvOption *SongNameOpt;
	CKtvOption *CancelOpt;

	int HzOptCount;
	CKtvOption **HzListOpt;

	int oldx, oldy;
	bool startwrite;
	int activeinput;
	int hzcount;
	hwInput *hw;
};
#endif

//==============================================================================
class COtherWindow: public CBaseWindow
{
public:
	COtherWindow(const char *name): CBaseWindow(name) {}
	virtual bool InputProcess(InputEvent *event);     // 输入事件处理函数
};
//==============================================================================
class CFoodMenuWindow: public CBaseWindow
{
public:
	CFoodMenuWindow(const char *name):CBaseWindow(name){}
//	virtual void LoadTheme(CKtvTheme *ptheme);
//	virtual void Show();
//	virtual bool InputProcess(InputEvent *event);
private:
//	int MenuOptCount;
//	CKtvOption **MenuOpt;
};
//==============================================================================

class CMenuTypeWindow: public CBaseWindow
{
public:
	CMenuTypeWindow(const char *name): CBaseWindow(name) {}
};

class CMenuLoginWindow: public CBaseWindow
{
public:
	CMenuLoginWindow(const char *name): CBaseWindow(name) {}
};

#endif
