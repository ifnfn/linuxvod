/*==============================================================================
 *  T h e   K T V L i n u x   P r o j e c t
 *------------------------------------------------------------------------------
 *	Filename   : win.h
 *	Author(s)  : Silicon
 *	Copyright  : Silicon
 *
 *	�ӻ���(CMtvBaseWindow)����������������������Ԫ�У������·�װ�����У�
 *	CMainWindow      : ��������,�������⺯��Close(),��ֹ����ر�
 *	CInputCodeWindow : ��ŵ����
 *	CNumLocateWindow : ����б��е�������λ��
 *	CPinYinWindow    : ƴ�������
 *	CDataBaseWindow  : �������ݱ�����Ļ���
 *	CSingerDataWindow: �����б���
 *	CSongDataWindow  : �����б���
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
class CThemeWindow: public CBaseWindow // ����
{
public:
	CThemeWindow(const char *name):CBaseWindow(name), ShowOpt(NULL){}
	~CThemeWindow() {}
	virtual void LoadTheme(CKtvTheme *ptheme);
	virtual bool InputProcess(InputEvent *event); // �����¼�������
protected:
	virtual void DrawWindowOpt();                 // ׼��������Ҫд�ֵĲ���
private:
	CKtvOption *ShowOpt;
	int themeid;
	void DrawTheme(int move); // move=0;��ʾ��ǰ��,-1��ʾ��һ��, 1��ʾ��һ��
};

class CSwitchLangWindow: public CBaseWindow // �����л�
{
public:
	CSwitchLangWindow(const char *name):CBaseWindow(name){}
	~CSwitchLangWindow() {}
	virtual bool InputProcess(InputEvent *event); // �����¼�������
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
class CInputWindow: public CBaseWindow // ��������
{
public:
	CInputWindow(const char *name):CBaseWindow(name), \
		activeopt(0), OptCount(0), Opt(NULL){}
	virtual ~CInputWindow() {
		free(Opt);
	}

	virtual void LoadTheme(CKtvTheme *ptheme);
	virtual bool InputProcess(InputEvent *event); // �����¼�������
	virtual void Show();                          // ��ʾ�������������ɼ����ʱ��Ҫ��ɵĲ���
protected:
	virtual void DrawWindowOpt();                 // ׼��������Ҫд�ֵĲ���
	int activeopt;
	int OptCount;
	CKtvOption **Opt;
};

//==============================================================================
class CInputCodeWindow: public CInputWindow // ��ŵ����
{
public:
	CInputCodeWindow(const char *name): CInputWindow(name), rec(NULL), foundsong(false){}
	virtual bool InputProcess(InputEvent *event); // �����¼�������
protected:
	virtual void DrawWindowOpt(); // ׼��������Ҫд�ֵĲ���
private:
	MemSongNode *rec;             // ���ҵ��ĸ�������
	bool foundsong;               // �Ƿ��ҵ�����
};

//==============================================================================
class CMyLoveWindow: public CInputWindow // �ҵ�������
{
public:
	CMyLoveWindow(const char *name):\
		CInputWindow(name), loadorsave(0){}
	virtual bool InputProcess(InputEvent *event); // �����¼�������
protected:
	virtual void DrawWindowOpt();                 // ׼��������Ҫд�ֵĲ���
	virtual void SetFilter(CString& name, CString& desc,int id=0);
private:
	int loadorsave;                               // ������߱���
	void SaveMyLove(CString& user, CString& pwd); // �����ҵ��
	void LoadMylove(CString& user, CString& pwd); // �����ҵ��
};

//==============================================================================
class CNumLocateWindow: public CBaseWindow // ���ඨλ��
{
public:
	CNumLocateWindow(const char *name):CBaseWindow(name){}
	virtual bool InputProcess(InputEvent *event);
};

//==============================================================================
class CPinYinWindow: public CBaseWindow // ƴ�������
{
public:
	CPinYinWindow(const char *name);
	~CPinYinWindow();
	virtual void LoadTheme(CKtvTheme *ptheme);
	virtual bool InputProcess(InputEvent *event); // �����¼�������
	virtual void Show();   // ��ʾ�������������ɼ����ʱ��Ҫ��ɵĲ���
protected:
	virtual void DrawWindowOpt();                 // ׼��������Ҫд�ֵĲ���
	virtual void Restore();
private:
	CKtvOption *EnterOpt;
	CKtvOption *PyLabelOpt;         // ��ʾƴ����ĸ
	int PyLength;                   // ƴ����ĸ����
	CKtvOption *board[3];           // �������
	CKtvOption **NumKeyOpt;         // ���ּ�������
	CKtvOption **CharKeyOpt[3];     // ��ĸ����
	int NumKeyOptCount;             // ���ּ�����
	int activeboard;                // ��ǰ�����
	void SetActiveNumKeyOpt();      // �����µ����ּ�
};

//==============================================================================
class CSelectedWindow: public CBaseWindow // �ѵ�����б���
{
public:
	CSelectedWindow(const char *name);
	~CSelectedWindow();
	virtual void LoadTheme(CKtvTheme *ptheme);
	virtual bool InputProcess(InputEvent *event); // �����¼�������
	virtual void Show();                          // ��ʾ����������ҳ���ʼ���Ȳ���
protected:
	virtual void AcitveEvent();                   // ���弤���¼�
	virtual void DrawWindowOpt();                 // ׼��������Ҫд�ֵĲ���
	virtual void SetFilter(CString& name, CString& desc,int id=0);
                                                   // ���ò�������
private:
	int SongOptCount;                             // �����б�����
	CKtvOption **SongListOpt;                     // �����б������
	int SingerOptCount;                           // �����б�����
	CKtvOption **SingerListOpt;                   // �����б������
	CKtvOption *msgopt;                           // ��ʾ����Ĳ�����
	CKtvOption *pageopt;                          // ��ʾҳ�ŵĲ�����
	int optstate;                                 // ����״̬: ����ɾ��

	int TotalPageCount;                           // ��ҳ��
	int CurPageID;                                // ��ǰҳ��
	void DrawPageText();                          // ��ʾҳ������
	void DrawMsgText(const char *msg=NULL);       // ��ʾ��������

	void FirstPage();                             // ����һҳ
	void EndPage  ();                             // �����һҳ
	bool NextPage ();                             // ��һҳ
	void PriorPage();                             // ��һҳ
	bool Bof();                                   // �Ƿ���������һҳ
	bool Eof();                                   // �Ƿ��������һҳ
};

//==============================================================================
class CDataBaseWindow: public CBaseWindow
{
public:
	CDataBaseWindow(const char *name);
	~CDataBaseWindow();
	virtual void LoadTheme(CKtvTheme *ptheme);
	virtual void Show();                                    // ��ʾ�������������ɼ���岢��ջ
	virtual bool InputProcess(InputEvent *event);           // �����¼�������
protected:
	int SongOptCount;                                       // �����б�����
	CKtvOption **SongListOpt;                               // �����б������
	int SingerOptCount;                                     // �����б�����
	CKtvOption **SingerListOpt;                             // �����б������
	CString FilterDescription;                              // ������ı�������
	CString subfilterdesc;                                  // �ӹ������������
	CString filter;                                         // ��������

	virtual void SetFilter(CString& name, CString& desc,int id=0); // ���ò�������
	virtual void DrawWindowOpt();                           // ׼��������Ҫд�ֵĲ���
	void ShowActiveChar();                                  // ���ǰ��ĸ������
private:
	int filterclass;                                        // ��������  0:�������� 1:ƴ������
	int activeid;                                           // ��ǰ�������ĸ
	int CharOptCount;                                       // ��ĸ����
	CKtvOption **CharListOpt;                               // ��ĸ������
	CKtvOption *msgopt;                                     // ��ʾ����Ĳ�����
	CKtvOption *pageopt;                                    // ��ʾҳ�ŵĲ�����
};

//==============================================================================
class CWBHWindow: public CDataBaseWindow // ��ʻ������
{
public:
	CWBHWindow(const char *name):CDataBaseWindow(name){}
	virtual void Show();                          // ��ʾ�������������ɼ���岢��ջ
	virtual bool InputProcess(InputEvent *event); // �����¼�������
};

//==============================================================================
class CSongDataWindow: public CDataBaseWindow     // �����б���
{
public:
	CSongDataWindow(const char *name):CDataBaseWindow(name), CurSelectSong(NULL){}
	virtual bool InputProcess(InputEvent *event); // �����¼�������
private:
	SelectSongNode *CurSelectSong;
};

//==============================================================================
class CSingerDataWindow: public CDataBaseWindow // �����б���
{
public:
	CSingerDataWindow(const char *name): CDataBaseWindow(name) {}
	virtual void LoadTheme(CKtvTheme *ptheme);
	virtual bool InputProcess(InputEvent *event);     // �����¼�������
protected:
	virtual void DrawWindowOpt();                     // ׼��������Ҫд�ֵĲ���
	bool DrawSingerPhoto(char *fn, const RECT pRect); // ��ʾ����ͼƬ
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
	virtual bool InputProcess(InputEvent *event);     // �����¼�������
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
