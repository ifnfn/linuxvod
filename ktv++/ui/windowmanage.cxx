#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "memtest.h"
#include "windowmanage.h"
#include "selected.h"
#include "timer.h"
#include "xmltheme.h"
#include "player.h"

#define MSG_TIMER_ID 123

CMsgWindow* CMsgWindow::pMsgWindow    = NULL;

/*******************************************************************************************
	Class CMtvBaseWindow 类
********************************************************************************************/
CBaseWindow::CBaseWindow(const char *name) :CKtvWindow(name)
{
	showing     = false;
	CurOption   = NULL;
	CurPlayOpt  = NULL;   // 正在播放的操作项
	NextPlayOpt = NULL;   // 下一首的操作项
	datadb      = NULL;
}

void CBaseWindow::LoadTheme(CKtvTheme *ptheme)
{
	theme = ptheme;
	CurPlayOpt  = FindOption("PlayingSongLabel");
	NextPlayOpt = FindOption("NextSongLabel"   );

	if (EmptyShadow) {
		gui->FreeBackground(EmptyShadow);
		EmptyShadow = NULL;
	}
	datadb = theme->songdata;
}

CBaseWindow::~CBaseWindow(void)
{
//	printf("Free CBaseWindow %s \n", WinName.data());
}

bool CBaseWindow::InputProcess(InputEvent *event)
{
	if (event == NULL) return false;
	return RunLink(event->option);
}

bool CBaseWindow::RunLink(CKtvOption *opt) /* 运行操作项的联接    */
{
	if (opt == NULL) return false;
	if (opt->link.length() > 0) {
		CBaseWindow *tmp = (CBaseWindow*)theme->FindWindow(opt->link.data());
		if (tmp) {
			if ((opt->filter != "") && (opt->mtv_value != DIKC_NULL))
				tmp->SetFilter(opt->filter, opt->title, opt->tagfilter);
			tmp->Show();
			return true;
		}
	}
	return false;
}

void CBaseWindow::Close() // 关闭
{
	if (showing)
	{
		showing = false;
		stack->WindowPop();
		CBaseWindow *tmp = stack->WindowTop();
		if (tmp)
			tmp->Restore();
	}
}

void CBaseWindow::Restore()
{
	CreateShadow();
	gui->UpdateShadow(&workrect);
	DrawWindowOpt();
	gui->Flip();
	gui->GuiRest();
	AcitveEvent();
}

void CBaseWindow::CreateShadow()
{
	if (EmptyShadow){
		gui->SetShadow(EmptyShadow);
	}
	else{
		EmptyShadow = gui->CreateBackground(ImageBuffer, ImageBufferSize, workrect);
		gui->SetShadow(EmptyShadow);
		FreeImageBuffer();
	}
}

void CBaseWindow::Paint()
{
	gui->UpdateShadow(&workrect);
	DrawWindowOpt();
	gui->Flip();
	gui->GuiRest();
}

void CBaseWindow::Show() // 显示函数
{
	showing = true;
	stack->WindowPush(this);
	Restore();
}

CKtvOption **CBaseWindow::CreateOptList(char *key, char startid, char endid, int *num)
{
	char opt[50];
	int count = 0;
	CKtvOption **optlist = NULL;
	for (char i=startid;i<=endid;i++) {
		sprintf(opt, "%s%c", key,i);
		CKtvOption *tmp = FindOption(opt);
		if (tmp)
		{
			optlist = (CKtvOption **)realloc(optlist, sizeof(CKtvOption*)*(count + 1));
			optlist[count++] = tmp;
		}
#ifdef DEBUG
		else
			DEBUG_OUT("CreateOptList no found %s\n", opt);
#endif
	}
	*num = count;
	return optlist;
}

void CBaseWindow::DrawWindowOpt()   // 准备窗体需要写字的部分
{
	DrawPlayStatus();
}

void CBaseWindow::DrawPlayStatus()
{
	int len;
	if (CurPlayOpt)
	{
		if (SelectedList.count > 0)
			len = strlen(SelectedList.items[0].SongName) + 10;
		else
			len = 1;
		char tmpstr[512] = "";
		if (SelectedList.count > 0)
			sprintf(tmpstr, "正播：%s", SelectedList.items[0].SongName);
		CurPlayOpt->title = tmpstr;
		gui->DrawTextOpt(CurPlayOpt);
	}
	if (NextPlayOpt)
	{
		if (SelectedList.count > 1)
			len = strlen(SelectedList.items[1].SongName) + 10;
		else
			len = 1;
		char tmpstr[512] = "";
		if (SelectedList.count > 1)
			sprintf(tmpstr, "下首：%s", SelectedList.items[1].SongName);
		NextPlayOpt->title =tmpstr;
		gui->DrawTextOpt(NextPlayOpt);
//		if (SelectedList.count > 1) {
//			char tmp[512] = "osdtext=-p 1 -x 100 -y 30 -s 32 -f 255,0,0,255 -t ";
//			strcat(tmp, tmpstr);
//			printf("tmp=%s\n", tmp);
//			player->SendPlayerCmdNoRecv(tmp);
//		}
	}
}

void CMsgWindow::ShowMsgBox(const char *msg, int timeout)
{
	if (msg) {
		if (box == VOLUME_BOX) CleanMsg();
		TColor c = {0xFF,0,0, 0xFF};
		gui->DrawFillRect(msgrect, c);
		TAlign align = {taCenter, taCenter};
		gui->SetFont(MsgFont);
		gui->DrawText(msg, msgrect, align);
		gui->Flip(&msgrect);
		if (timeout > 0)
			StartTimer(MSG_TIMER_ID, timeout);
	} else 
		CleanMsg();
	box = MSG_BOX;
}

void CMsgWindow::ShowVolume(int volume)
{
	if (box == MSG_BOX) CleanMsg();
	gui->DrawSoundBar(volume);
	StartTimer(MSG_TIMER_ID, 3000);
	box = VOLUME_BOX;
}

CMsgWindow *CMsgWindow::Create(CKtvTheme *ptheme)
{
	if (!ptheme) {
		printf("CMsgWindow *CMsgWindow::Create(CKtvTheme *ptheme)\n");
		return NULL;
	}
	CKtvFont *font = ptheme->FindFont("msgfont");
	if (!pMsgWindow) {
		pMsgWindow = new CMsgWindow("msgform");
	}
	pMsgWindow->MsgFont = font;
	return pMsgWindow;
}

CMsgWindow::CMsgWindow(const char *name):CKtvWindow(name), MsgFont(NULL), box(NONE)
{
	msgrect.top    = MSGBOX_TOP;
	msgrect.left   = MSGBOX_LEFT;
	msgrect.right  = msgrect.left + MSGBOX_WIDTH;
	msgrect.bottom = msgrect.top  + MSGBOX_HEIGHT;
}

void CMsgWindow::CleanMsg()
{
	CBaseWindow *tmp = stack->WindowTop();
	if (tmp)
		tmp->Paint();
}

void CMsgWindow::TimerWork()
{
	CleanMsg();
	KillTimer(MSG_TIMER_ID);
	box = NONE;
}

void ShowMsgBox(const char *msg, int timeout)
{
	CMsgWindow *tmp = CMsgWindow::GetMsgWindow();
	if (tmp)
		tmp->ShowMsgBox(msg, timeout);
}

void ShowVolumeBox(int volume)
{
	CMsgWindow *tmp = CMsgWindow::GetMsgWindow();
	if (tmp)
		tmp->ShowVolume(volume);
}
