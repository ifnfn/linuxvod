/*==============================================================================
 *   T h e   K T V L i n u x   P r o j e c t
 *------------------------------------------------------------------------------
 *  Filename   : windowmanage.h
 *  Author(s)  : Silicon
 *  Copyright  : Silicon
 *
 *      所有窗体的基类(CMtvBaseWindow)，实现窗体的初始化、显示、按键处理、
 *  鼠标处理，操作项的绘画等操作。
 *      所有需要显示的窗体只需要重载虚拟函数DrawWindowOpt()来实现文字绘图
 *      如果需要实现特殊的键盘、鼠标处理，需要重载虚拟函数InputProcess()
 *	CWindowStack类，实现窗体在显示、关闭过程中需要记录的先入后出顺序的类
 *	    另外用到了两个全局变量：
 *	        stack: 即窗体堆栈
 *	        gui  : 图形接口
 *============================================================================*/
#ifndef WINDOWMANAGE_H
#define WINDOWMANAGE_H

#include <stdint.h>
#include <sys/timeb.h>

#include "config.h"
#include "keybuf.h"
#include "gui.h"

class CBaseGui;
class CWindowStack;

class CBaseWindow: public CKtvWindow
{
public:
	char *Filter;          // 数据过过滤条件

	CBaseWindow(const char *name);
	virtual ~CBaseWindow(void);

	virtual void LoadTheme(CKtvTheme *ptheme);   // 从界面中读入数据
	virtual void Paint();                        // 不重建背景缓冲,重绘窗体显示
	virtual void Show();                         // 显示函数，将窗体变成激活窗体时需要完成的操作
	virtual void Close();                        // 窗体退出时完成的操作
	virtual bool InputProcess(InputEvent *event);// 输入事件处理函数
protected:
	CKtvOption *CurPlayOpt;                      // 正在播放的操作项
	CKtvOption *NextPlayOpt;                     // 下一首的操作项
	CKtvTheme *theme;

	bool RunLink(CKtvOption *opt);               // 运行操作项的联接
	void DrawPlayStatus();                       // 显示播放状态
	virtual void AcitveEvent(){}                 // 窗体激活事件
	virtual void Destory(){}                     // 窗体free时需要完成的操作
	virtual void DrawWindowOpt();                // 准备窗体需要写字的部分
	virtual void Restore();                      // 重建背景缓冲,并恢复窗体的显示
	CKtvOption **CreateOptList(char *key, char startid, char ednid, int *num); // 建立操作项分组列表
	virtual void SetFilter(CString& name, CString& desc, int id=0){} // 设置查找条件
	CData *datadb;
private:
	bool showing;                                // 窗体的显示状态
	void CreateShadow();
};

#define MAXSTACK 10
class CWindowStack
{
private:
	int maxnum;
	int tos;
	CBaseWindow *stack[MAXSTACK];
	static CWindowStack *pWinStack;
protected:
	CWindowStack():maxnum(MAXSTACK), tos(0){}
	~CWindowStack(void);
public:
	static void FreeStack();
	static CWindowStack *CreateStack();

	void WindowPush(CBaseWindow *window);
	CBaseWindow *WindowPop();
	CBaseWindow *WindowTop();
	void StackClean(int num=0);
	int stacknum() { return tos;}
};

class CMsgWindow: public CKtvWindow
{
public:
	CKtvFont *MsgFont;
	void ShowMsgBox(char *msg, int timeout);
	void ShowVolume(int volume);

	static CMsgWindow *Create(CKtvTheme *ptheme);
	static CMsgWindow *GetMsgWindow() { return pMsgWindow; }
protected:
	CMsgWindow(const char *name);
	virtual ~CMsgWindow() {}
	virtual void TimerWork();
private:
	enum {
		NONE,
		MSG_BOX,
		VOLUME_BOX
	} box;
	RECT msgrect;
	static CMsgWindow* pMsgWindow;
	void Rest();
};

void ShowMsgBox(char *msg, int timeout);
void ShowVolumeBox(int volume);

//==============================================================================

#endif
