/*==============================================================================
 *   T h e   K T V L i n u x   P r o j e c t
 *------------------------------------------------------------------------------
 *  Filename   : windowmanage.h
 *  Author(s)  : Silicon
 *  Copyright  : Silicon
 *
 *      ���д���Ļ���(CMtvBaseWindow)��ʵ�ִ���ĳ�ʼ������ʾ����������
 *  ��괦��������Ļ滭�Ȳ�����
 *      ������Ҫ��ʾ�Ĵ���ֻ��Ҫ�������⺯��DrawWindowOpt()��ʵ�����ֻ�ͼ
 *      �����Ҫʵ������ļ��̡���괦����Ҫ�������⺯��InputProcess()
 *	CWindowStack�࣬ʵ�ִ�������ʾ���رչ�������Ҫ��¼��������˳�����
 *	    �����õ�������ȫ�ֱ�����
 *	        stack: �������ջ
 *	        gui  : ͼ�νӿ�
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
	char *Filter;          // ���ݹ���������

	CBaseWindow(const char *name);
	virtual ~CBaseWindow(void);

	virtual void LoadTheme(CKtvTheme *ptheme);   // �ӽ����ж�������
	virtual void Paint();                        // ���ؽ���������,�ػ洰����ʾ
	virtual void Show();                         // ��ʾ�������������ɼ����ʱ��Ҫ��ɵĲ���
	virtual void Close();                        // �����˳�ʱ��ɵĲ���
	virtual bool InputProcess(InputEvent *event);// �����¼�������
protected:
	CKtvOption *CurPlayOpt;                      // ���ڲ��ŵĲ�����
	CKtvOption *NextPlayOpt;                     // ��һ�׵Ĳ�����
	CKtvTheme *theme;

	bool RunLink(CKtvOption *opt);               // ���в����������
	void DrawPlayStatus();                       // ��ʾ����״̬
	virtual void AcitveEvent(){}                 // ���弤���¼�
	virtual void Destory(){}                     // ����freeʱ��Ҫ��ɵĲ���
	virtual void DrawWindowOpt();                // ׼��������Ҫд�ֵĲ���
	virtual void Restore();                      // �ؽ���������,���ָ��������ʾ
	CKtvOption **CreateOptList(char *key, char startid, char ednid, int *num); // ��������������б�
	virtual void SetFilter(CString& name, CString& desc, int id=0){} // ���ò�������
	CData *datadb;
private:
	bool showing;                                // �������ʾ״̬
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
