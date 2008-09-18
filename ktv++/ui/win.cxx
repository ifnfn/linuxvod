#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include "memtest.h"
#include "gui.h"
#include "win.h"
#include "player.h"
#include "selected.h"
#include "avio/avio.h"

static const char *msg[2] = {"优先","删除"};

bool CSwitchLangWindow::InputProcess(InputEvent *event) // 输入事件处理函数
{
	if (event->option == NULL)
		return CBaseWindow::InputProcess(event);
	switch (event->option->mtv_value)
	{
		case DIKC_1:
		case DIKC_2:
		case DIKC_3:
			theme->LoadThemeFromFile(event->option->name);
			Close();
			return true;
		case DIKC_ENTER:
			return true;
	}
	return CBaseWindow::InputProcess(event);
}

/*******************************************************************************
 *	Class CThemeWindow 歌曲列表的数字定位类
 ******************************************************************************/
void CThemeWindow::LoadTheme(CKtvTheme *ptheme)
{
	CBaseWindow::LoadTheme(ptheme);
	ShowOpt = FindOption("ShowSample");
	themeid = 0;
}

void CThemeWindow::DrawTheme(int move)
{
	if (theme->config->PicThemeCount > 0)
	{
		themeid = (themeid + move + theme->config->PicThemeCount) % theme->config->PicThemeCount;
		char *picfile = theme->config->ThemePicList[themeid];
		if (picfile)
		{
			int64_t size=0;
			char url[512];
			theme->config->geturl(url, PlunginsPath, picfile);
			char *data = (char *)url_readbuf(url, &size);
			if (data)
			{
				gui->DrawImage(data, size, ShowOpt->rect);
				free(data);
			}
		}
	}
}

void CThemeWindow::DrawWindowOpt()
{
	CBaseWindow::DrawWindowOpt();
	DrawTheme(0);
}

bool CThemeWindow::InputProcess(InputEvent *event) /* 输入事件处理函数 */
{
	if (event->option == NULL)
		return CBaseWindow::InputProcess(event);
	switch (event->option->mtv_value)
	{
		case DIKC_PAGE_UP:
			DrawTheme(-1);
			gui->Flip();
			return true;
		case DIKC_PAGE_DOWN:
			DrawTheme(1);
			gui->Flip();
			return true;
		case DIKC_ENTER:
			CString tmp(theme->config->ThemeList[themeid]);
			stack->StackClean(0);
			theme->LoadThemeFromFile(tmp);
			CBaseWindow* main = (CBaseWindow*)theme->FindWindow(MainFormStr);
			if (main) main->Show();
			return true;
	}
	return CBaseWindow::InputProcess(event);
}

/*******************************************************************************
 *	Class CNumLocateWindow 歌曲列表的数字定位类
 ******************************************************************************/
bool CNumLocateWindow::InputProcess(InputEvent *event)
{
	if (event->option == NULL)
		return CBaseWindow::InputProcess(event);
	switch (event->option->mtv_value)
	{
		case DIKC_0:
		case DIKC_1:
		case DIKC_2:
		case DIKC_3:
		case DIKC_4:
		case DIKC_5:
		case DIKC_6:
		case DIKC_7:
		case DIKC_8:
		case DIKC_9:
			datadb->Locate("num", atoi(event->option->title.data() ));
			Close();
			return true;
	}
	return CBaseWindow::InputProcess(event);
}

/*******************************************************************************
 *	Class CPinYinWindow 类
 ******************************************************************************/
CPinYinWindow::CPinYinWindow(const char *name):CBaseWindow(name), \
	EnterOpt(NULL), PyLabelOpt(NULL), NumKeyOpt(NULL), NumKeyOptCount(0), activeboard(0)
{
	CharKeyOpt[0] = NULL;
	CharKeyOpt[1] = NULL;
	CharKeyOpt[2] = NULL;
}

void CPinYinWindow::LoadTheme(CKtvTheme *ptheme)
{
	CBaseWindow::LoadTheme(ptheme);
	PyLabelOpt = FindOption("PyLabel");
	EnterOpt   = FindOption("Enter");
	if (EnterOpt)
		EnterOpt->tagfilter = 1;
	board[0]   = FindOption("Board1");
	board[1]   = FindOption("Board2");
	board[2]   = FindOption("Board3");
	NumKeyOpt  = CreateOptList("Num", '0', '9', &NumKeyOptCount);

	for (int i=0;i<3;i++)
		CharKeyOpt[i] = (CKtvOption **)realloc(CharKeyOpt[i], sizeof(CKtvOption*) *NumKeyOptCount);

	int x,y;
	char opt[10];
	char key[25] = "ABCDEFGHJKLMNOPQRSTWXYZ?";
	for (int i=0;i<25;i++)
	{
		x = i / NumKeyOptCount;
		y = i % NumKeyOptCount;
		sprintf(opt, "Char%c", key[i]);
		CKtvOption *tmp = FindOption(opt);
		if (tmp) {
			CharKeyOpt[x][y] = tmp;
#ifdef DEBUG
		}
		else {
			DEBUG_OUT("No found %s\n", opt);
#endif
		}
	}
	activeboard = 0;
	SetActiveNumKeyOpt();
}

CPinYinWindow::~CPinYinWindow()
{
	for (int i=0;i<3;i++)
		free(CharKeyOpt[i]);
	free(NumKeyOpt);
}

void CPinYinWindow::Show()
{
	PyLabelOpt->title = "";
	CBaseWindow::Show();
}

void CPinYinWindow::Restore()
{
	PyLabelOpt->title = "";
	CBaseWindow::Restore();
}

void CPinYinWindow::DrawWindowOpt()
{
	CBaseWindow::DrawWindowOpt();
	gui->DrawTextOpt(PyLabelOpt);
	gui->DrawRectangle(board[activeboard]->rect, board[activeboard]->font->color);
}

void CPinYinWindow::SetActiveNumKeyOpt()
{
	for (int i=0; i<NumKeyOptCount;i++)
		NumKeyOpt[i]->title = CharKeyOpt[activeboard][i]->title;
}

bool CPinYinWindow::InputProcess(InputEvent *event)
{
	if (event->option == NULL)
		return  CBaseWindow::InputProcess(event);
	switch (event->option->mtv_value)
	{
		case DIKC_ENTER:
			EnterOpt->filter = PyLabelOpt->title;
			break;
		case DIKC_DELETE:
			PyLabelOpt->title = "";
			Paint();
			return true;
#if 0
		case DIKC_LEFT: {
			char tmpbuf[100];
			strncpy(tmpbuf, PyLabelOpt->title.data(), 99);
			tmpbuf[PyLabelOpt->title.length() - 1] = '\0';
			PyLabelOpt->title = tmpbuf;
			Paint();
			return true;
		}
#endif
		case DIKC_UP:
		case DIKC_PAGE_UP:
			activeboard--;
			activeboard = (activeboard + 3) % 3;
			SetActiveNumKeyOpt();
			Paint();
			return true;
		case DIKC_DOWN:
		case DIKC_PAGE_DOWN:
			activeboard++;
			activeboard = (activeboard) % 3;
			SetActiveNumKeyOpt();
			Paint();
			return true;
		case DIKC_1:
		case DIKC_2:
		case DIKC_3:
		case DIKC_4:
		case DIKC_5:
		case DIKC_6:
		case DIKC_7:
		case DIKC_8:

		case DIKC_A:
		case DIKC_B:
		case DIKC_C:
		case DIKC_D:
		case DIKC_E:
		case DIKC_F:
		case DIKC_G:
		case DIKC_H:
		case DIKC_I:
		case DIKC_J:
		case DIKC_K:
		case DIKC_L:
		case DIKC_M:
		case DIKC_N:
		case DIKC_O:
		case DIKC_P:
		case DIKC_Q:
		case DIKC_R:
		case DIKC_S:
		case DIKC_T:
		case DIKC_U:
		case DIKC_V:
		case DIKC_W:
		case DIKC_X:
		case DIKC_Y:
		case DIKC_Z:
			if (PyLabelOpt->title.length() < 8){
				PyLabelOpt->title += event->option->title;
				Paint();
			}
			return true;
	}
	return CBaseWindow::InputProcess(event);
}

/*******************************************************************************
 *	Class CSelectedWindow 类
 ******************************************************************************/
CSelectedWindow::CSelectedWindow(const char *name):CBaseWindow(name),\
	SongOptCount(0), SongListOpt(NULL), SingerOptCount(0), SingerListOpt(NULL), \
	msgopt(NULL), pageopt(NULL), optstate(0), TotalPageCount(0), CurPageID(0)
{}

CSelectedWindow::~CSelectedWindow()
{
	free(SongListOpt);
	free(SingerListOpt);
}

void CSelectedWindow::LoadTheme(CKtvTheme *ptheme)
{
	CBaseWindow::LoadTheme(ptheme);
	SongListOpt  = CreateOptList("Num", '0', '9', &SongOptCount);
	SingerListOpt= CreateOptList("Singer", '0', '9', &SingerOptCount);
	msgopt = FindOption("MsgLabel");
	if (msgopt)
		msgopt->title= "优先";

	optstate = 0;
	pageopt= FindOption("PageNoLabel");
	if (pageopt)
		pageopt->title = "";
}

void CSelectedWindow::DrawWindowOpt()
{
	CBaseWindow::DrawWindowOpt();

	DrawMsgText();
	DrawPageText();

	if (Eof()) return;
	int i = 0;
	CKtvOption *tmp = NULL;
	int startid = CurPageID * SongOptCount;

	// 将 for 分成两次，这样就不需要每次都换一次字体，提高效率
	for (i=0;(i<SongOptCount) && (startid+i<SelectedList.count);i++){
		tmp = SingerListOpt[i];
		if (tmp){
			tmp->title = SelectedList.items[startid+i].SingerName;
			gui->DrawTextOpt(tmp);
		}
	}
	for (i=0;(i<SongOptCount) && (startid+i<SelectedList.count);i++){
		tmp = SongListOpt[i];
		if (tmp){
			tmp->title = SelectedList.items[startid+i].SongName;
			gui->DrawTextOpt(tmp);
			tmp->private_data = SelectedList.items + startid + i;
		}
	}

	for (;i<SongOptCount;i++)
	{
		tmp = SongListOpt[i];
		if (tmp)
			tmp->private_data = NULL;
	}
}

void CSelectedWindow::AcitveEvent() // 窗体激活事件
{
//	printf("CSelectedWindow::ActiveEvent\n");
}

void CSelectedWindow::Show()
{
	player->ReloadSongList();
	CurPageID = 0;
	CBaseWindow::Show();
}

void CSelectedWindow::SetFilter(CString& name, CString& desc, int id)
{
	if (name == "优先")
		optstate = 0;
	else
		optstate = 1;
	if (msgopt)
		msgopt->title = msg[optstate];
}

void CSelectedWindow::DrawPageText()
{
	TotalPageCount = SelectedList.count / SongOptCount;
	TotalPageCount = (SelectedList.count % SongOptCount >0) ? TotalPageCount + 1 : TotalPageCount;
	if (pageopt){
		char tempstr[100] = "";
		sprintf(tempstr, "共%2d页, 第%2d页", TotalPageCount, TotalPageCount==0?CurPageID:CurPageID+1);
		pageopt->title = tempstr;
		gui->DrawTextOpt(pageopt);
	}
}

void CSelectedWindow::DrawMsgText(const char *msg)
{
	if (msgopt)
	{
		if (msg)
			msgopt->title = msg;
		gui->DrawTextOpt(msgopt);
	}
}

void CSelectedWindow::FirstPage()       /* 到第一页 */
{
	CurPageID = 0;
}

void CSelectedWindow::EndPage  ()       /* 到最后一页 */
{
	CurPageID = TotalPageCount;
}

bool CSelectedWindow::NextPage ()       /* 下一页 */
{
	if (CurPageID < TotalPageCount-1)
		CurPageID++;
	else
		CurPageID = 0;
	return CurPageID < TotalPageCount;
}

void CSelectedWindow::PriorPage()       /* 上一页 */
{
	if (CurPageID > 0)
		CurPageID--;
	else
		CurPageID = TotalPageCount-1;
}

bool CSelectedWindow::Bof()             /* 是否到了最上面一页 */
{
	return CurPageID == 0;
}

bool CSelectedWindow::Eof()             /* 是否到了最后面一页 */
{
	return CurPageID >= TotalPageCount;
}

bool CSelectedWindow::InputProcess(InputEvent *event)
{
	if (event->option == NULL)
		return  CBaseWindow::InputProcess(event);
	int state = 1;
	switch (event->option->mtv_value)
	{
		case DIKC_PAGE_UP:
			PriorPage();
			Paint();
			return true;
		case DIKC_PAGE_DOWN:
			if (NextPage()) Paint();
			return true;
		case DIKC_FIRST:
			state = 0;
		case DIKC_DELETE:
			optstate = state = state & 1;
			if (msgopt)
				msgopt->title = msg[optstate];
			Paint();
			return true;
		case DIKC_1:
		case DIKC_2:
		case DIKC_3:
		case DIKC_4:
		case DIKC_5:
		case DIKC_6:
		case DIKC_7:
		case DIKC_0:
			if (event->option->private_data != NULL)
			{
				SelectSongNode *tmp;
				tmp = (SelectSongNode *)(event->option->private_data);
				if (optstate)
					player->NetDelSongFromList(tmp);
				else
					player->NetFirstSong(tmp, 1);
				Paint();
			}
			return true;
		case DIKC_8:
		case DIKC_9:
			break;
	}
	return CBaseWindow::InputProcess(event);
}

/*******************************************************************************
 *	Class CInputCodeWindow 类
 ******************************************************************************/
#define MAXLEN 15

void CInputWindow::LoadTheme(CKtvTheme *ptheme)
{
	CBaseWindow::LoadTheme(ptheme);
	Opt = CreateOptList("Opt", '1', '9', &OptCount);
	for (int i=0;i<OptCount;i++)
		Opt[i]->title = "";
	datadb = theme->songdata;
}

void CInputWindow::Show()
{
	for (int i=0;i<OptCount;i++)
		Opt[i]->title = "";
	activeopt = 0;
	CBaseWindow::Show();
}

void CInputWindow::DrawWindowOpt()
{
	CBaseWindow::DrawWindowOpt();
	for (int i=0;i<OptCount;i++)
		gui->DrawTextOpt(Opt[i]);
}

bool CInputWindow::InputProcess(InputEvent *event) /* 输入事件处理函数 */
{
	if (event->option == NULL)
		return  CBaseWindow::InputProcess(event);
	switch (event->option->mtv_value)
	{
		case DIKC_LEFT:
			if (Opt[activeopt]->title.length() > 0){
				char tmpbuf[100];
				strncpy(tmpbuf, Opt[activeopt]->title.data(), 99);
				tmpbuf[Opt[activeopt]->title.data(), Opt[activeopt]->title.length() - 1] = '\0';
				Opt[activeopt]->title = tmpbuf;
				Paint();
			}
			return true;
		case DIKC_DELETE:
			Opt[activeopt]->title = "";
			Paint();
			return true;
		case DIKC_0:
		case DIKC_1:
		case DIKC_2:
		case DIKC_3:
		case DIKC_4:
		case DIKC_5:
		case DIKC_6:
		case DIKC_7:
		case DIKC_8:
		case DIKC_9:
			if (Opt[activeopt]) {
				if (Opt[activeopt]->title.length() < MAXLEN - 1){
					Opt[activeopt]->title += event->option->title;
					Paint();
				}
			}
			return true;
	}
	return CBaseWindow::InputProcess(event);
}

/*******************************************************************************
 *	Class CInputCodeWindow 类
 ******************************************************************************/
void CInputCodeWindow::DrawWindowOpt()
{
	if (!datadb) return;
	rec = datadb->GetNameByCode(Opt[activeopt]->title.data());
	if (rec)
		Opt[1]->title = rec->SongName;
	else
		Opt[1]->title = "";
	CInputWindow::DrawWindowOpt();
}

bool CInputCodeWindow::InputProcess(InputEvent *event) /* 输入事件处理函数 */
{
	if (event->option == NULL)
		return CInputWindow::InputProcess(event);
	else if (event->option->mtv_value == DIKC_ENTER)
	{
		if (rec)
			player->NetAddSongToList(rec); /* MemSongNode */
		Close();
		return true;
	} else
		return CInputWindow::InputProcess(event);
}

/*******************************************************************************
 *	Class CInputCodeWindow 类
 ******************************************************************************/
void CMyLoveWindow::SetFilter(CString& name, CString& desc, int id)
{
	if (name == "Save")
		loadorsave = 1;
	else
		loadorsave = 0;
}

bool CMyLoveWindow::InputProcess(InputEvent *event) /* 输入事件处理函数 */
{
	if (event->option == NULL)
		return CInputWindow::InputProcess(event);
	switch (event->option->mtv_value)
	{
		case DIKC_PAGE_UP:
		case DIKC_PAGE_DOWN:
			activeopt = (activeopt+1) % 2;
			Paint();
			return true;
		case DIKC_ENTER:
			activeopt++;
			if (activeopt == 1)
				Paint();
			else if (activeopt == 2){
				if (loadorsave == 1)
					SaveMyLove(Opt[0]->title, Opt[1]->title);
				else if (loadorsave == 0)
					LoadMylove(Opt[0]->title, Opt[1]->title);
				Close();
			}
			return true;
	}
	return CInputWindow::InputProcess(event);
}

void CMyLoveWindow::DrawWindowOpt()
{
	TColor c = {0,0xff,0,0};
	if (Opt[activeopt])
		gui->DrawRectangle(Opt[activeopt]->rect,c);
	CInputWindow::DrawWindowOpt();
}

#define CODEBUFSIZE 4096

void CMyLoveWindow::SaveMyLove(CString& user, CString& pwd) // 保存我的最爱
{
	char filename[CODEBUFSIZE];
	sprintf(filename, "/savemylove?%s:%s&", user.data(), pwd.data());
	char url[10240];
	theme->config->getremoteurl(url, "", filename);

	for (int i=0; i<SelectedList.count; i++)
	{
		strcat(url, SelectedList.items[i].SongCode);
		strcat(url, "|");
	}
	offset_t size;
	char *p = (char*)url_readbuf(url, &size);
	if (p) free(p);
}

void CMyLoveWindow::LoadMylove(CString& user, CString& pwd) // 载入我的最爱
{
	char filename[100];
	sprintf(filename, "/getmylove?%s:%s&", user.data(), pwd.data());
	URLContext *p;
	char url[512];
	theme->config->getremoteurl(url, "", filename);
//	printf("url=%s\n", url);
	if (url_open(&p, url, URL_RDONLY) >= 0)
	{
		offset_t size = url_size(p);
		char *codebuf = (char *)malloc(size + 1);
		memset(codebuf, 0, size + 1);
		unsigned char *curp = (unsigned char *)codebuf;
		offset_t len;
		while (1)
		{
			len = url_read(p, curp, size);
			if (len <= 0) break;
			curp += len;
		}
		url_close(p);
		MemSongNode *rec;
//		printf("codebuf=%s\n", codebuf);
		char *code = strtok(codebuf, "|");
		while (code)
		{
			rec = datadb->GetNameByCode(code);
			if (rec)
				player->NetAddSongToList(rec); /* MemSongNode */
			code = strtok(NULL, "|");
		}
		free(codebuf);
	}
	Close();
}

/*******************************************************************************
 *	Class CDataBaseWindow 类
 ******************************************************************************/
CDataBaseWindow::CDataBaseWindow(const char *name):CBaseWindow(name), \
	SongOptCount(0), SongListOpt(NULL), SingerOptCount(0), SingerListOpt(NULL), \
	filterclass(0), activeid(-1), CharOptCount(0), \
	CharListOpt(NULL), msgopt(NULL), pageopt(NULL)
{
	subfilterdesc = "";
	FilterDescription = "";
	filter = "";
}

void CDataBaseWindow::LoadTheme(CKtvTheme *ptheme)
{
	CBaseWindow::LoadTheme(ptheme);
	SongListOpt  = CreateOptList("Num"   , '1', '9', &SongOptCount);
	SingerListOpt= CreateOptList("Singer", '1', '9', &SingerOptCount);
	CharListOpt  = CreateOptList("Char"  , 'A', 'Z', &CharOptCount);
	msgopt = FindOption("MsgLabel");
	if (msgopt)
		msgopt->title = "";
	pageopt= FindOption("PageNoLabel");
	if (pageopt)
		pageopt->title = "";
	datadb = theme->songdata;
}

CDataBaseWindow::~CDataBaseWindow()
{
	free(SongListOpt);
	free(SingerListOpt);
	free(CharListOpt);
}

void CDataBaseWindow::DrawWindowOpt()
{
	CBaseWindow::DrawWindowOpt();
	CKtvOption *tmp = NULL;
	int i;
	// 将 for 分成两次，这样就不需要每次都换一次字体，提高效率
	for (i=0; (i<datadb->CurPage.Count) && (i<SingerOptCount); i++)
	{
		tmp = SingerListOpt[i];
		if (tmp){
			tmp->title =  datadb->CurPage.RecList[i]->SingerName;
			if (tmp->title != FilterDescription)
				gui->DrawTextOpt(tmp);

		}
	}
	for (i=0; (i<datadb->CurPage.Count) && (i<SongOptCount); i++) {
		if ((tmp = SongListOpt[i])) {
			tmp->title = datadb->CurPage.RecList[i]->SongName;
			tmp->font->SetFont(datadb->CurPage.RecList[i]->Charset);
			tmp->active = SongCodeInList(datadb->CurPage.RecList[i]->SongCode);
			gui->DrawTextOpt(tmp, NULL);
			tmp->private_data = datadb->CurPage.RecList[i];
		}
	}
	for (;i<SongOptCount;i++) {
		tmp = SongListOpt[i];
		if (tmp) 
			tmp->private_data = NULL;
	}
	if (msgopt)  // 显示标题
	{
		gui->SetFont(msgopt->font);
		if (subfilterdesc.length() > 0){
			if (FilterDescription.length() > 0) {
				char tmpstr[512];
				sprintf(tmpstr, "%s-%s",FilterDescription.data(), subfilterdesc.data());
				msgopt->title = tmpstr;
			}
			else
				msgopt->title = subfilterdesc;
		}else
			msgopt->title = FilterDescription;
		gui->DrawTextOpt(msgopt);
	}
	if (pageopt){ // 显示页码
		char tmpstr[512];
		sprintf(tmpstr, "共%2ld页, 第%2ld页", datadb->TotalPageCount, datadb->GetCurPageNo());
		pageopt->title = tmpstr;
		gui->DrawTextOpt(pageopt);
	}
	ShowActiveChar();
}

void CDataBaseWindow::SetFilter(CString& name, CString& desc, int id)
{
//	printf("SetFilter %s, %s, %d)\n", name.data(), desc.data(), id);
	filter = name;
	FilterDescription = desc;
	filterclass = id;
}

void CDataBaseWindow::Show() /* 显示函数，将窗体变成激活窗体时需要完成的操作   */
{
	subfilterdesc = "";
	activeid = -1;
	if (datadb)
	{
		datadb->SetFilterName(filter, FilterDescription, filterclass);
		datadb->ActiveFilter(SongOptCount, true);
	}
	CBaseWindow::Show();
}

bool CDataBaseWindow::InputProcess(InputEvent *event) /* 输入事件处理函数 */
{
	if (event->option == NULL)
		return CBaseWindow::InputProcess(event);
	switch (event->option->mtv_value)
	{
		case DIKC_PAGE_UP:
			if (datadb->Bof())
				datadb->EndPage();
			else
				datadb->PriorPage();
			Paint();
			return true;
		case DIKC_PAGE_DOWN:
			if (datadb->Eof())
				datadb->FirstPage();
			else
				datadb->NextPage();
			Paint();
			return true;
		case DIKC_A:
		case DIKC_B:
		case DIKC_C:
		case DIKC_D:
		case DIKC_E:
		case DIKC_F:
		case DIKC_G:
		case DIKC_H:
		case DIKC_I:
		case DIKC_J:
		case DIKC_K:
		case DIKC_L:
		case DIKC_M:
		case DIKC_N:
		case DIKC_O:
		case DIKC_P:
		case DIKC_Q:
		case DIKC_R:
		case DIKC_S:
		case DIKC_T:
		case DIKC_U:
		case DIKC_V:
		case DIKC_W:
		case DIKC_X:
		case DIKC_Y:
		case DIKC_Z:
			subfilterdesc = event->option->title;
			datadb->PinYinFilter(subfilterdesc);
			for (int i=0;i<CharOptCount;i++)
			{
				if (event->option->name == CharListOpt[i]->name)
				{
					activeid= i;
					break;
				}
			}
			Paint();
			return true;
		case DIKC_LEFT:
		case DIKC_RIGHT:
			if (event->option->mtv_value == DIKC_LEFT)
				activeid = (activeid + CharOptCount - 1) % CharOptCount;
			else
				activeid = (activeid + 1) % CharOptCount;
			ShowActiveChar();
		case DIKC_ENTER:
			if ((activeid>= 0) && (activeid<CharOptCount)) {
				subfilterdesc = CharListOpt[activeid]->title;
				datadb->PinYinFilter(subfilterdesc);
				Paint();
			}
			return true;
	}
	return CBaseWindow::InputProcess(event);
}

void CDataBaseWindow::ShowActiveChar()
{
	if (activeid < CharOptCount && activeid >= 0)
	{
		gui->DrawRectangle(CharListOpt[activeid]->rect,CharListOpt[activeid]->font->color);
//		activeid = newid;
	}
}

void CWBHWindow::Show() /* 显示函数，将窗体变成激活窗体时需要完成的操作    */
{
	CString t1(""), t2("");
	SetFilter(t1, t2, 2);
	CDataBaseWindow::Show();
	Paint();
}

#define MAXWBHLEN 8
bool CWBHWindow::InputProcess(InputEvent *event)
{
	if (event->option == NULL)
		return CBaseWindow::InputProcess(event);
	MemSongNode *tmp;
	switch (event->option->mtv_value)
	{
		case DIKC_LEFT:
		case DIKC_1:
		case DIKC_2:
		case DIKC_3:
		case DIKC_4:
		case DIKC_5:{
			if (event->option->mtv_value == DIKC_LEFT)
			{
				if (subfilterdesc.length() > 0)
					subfilterdesc ="";
				else if (filter.length() > 0)  {
					char tmpbuf[100];
					strncpy(tmpbuf, filter.data(), 99);
					tmpbuf[filter.length() - 1] = '\0';
					filter= tmpbuf;
				}
			}
			else if (filter.length() < MAXWBHLEN){
				filter += event->option->title;
			}
			CString tmp("五笔划");
			datadb->SetFilterName(filter, tmp, 2);
			FilterDescription = filter;
			if (subfilterdesc.length() > 0)
				datadb->PinYinFilter(subfilterdesc);
			else
				datadb->ActiveFilter(SongOptCount);
			Paint();
			return true;
		}
		case DIKC_6:
		case DIKC_7:
		case DIKC_8:
		case DIKC_9:
		case DIKC_0:
			if (event->option->private_data != NULL)
			{
				tmp = (MemSongNode *)(event->option->private_data);
				player->NetAddSongToList(tmp); /* MemSongNode */
				event->option->active = true;
				gui->DrawTextOpt(event->option, NULL);
				gui->Flip(&event->option->rect);
			}
			return true;
	}
	return CDataBaseWindow::InputProcess(event);
}

/*******************************************************************************
 *	Class CSongDataWindow 类
 ******************************************************************************/
bool CSongDataWindow::InputProcess(InputEvent *event)
{
	if (event->option == NULL)
		return CBaseWindow::InputProcess(event);
	MemSongNode *tmp;

	switch (event->option->mtv_value)
	{
		case DIKC_1:
		case DIKC_2:
		case DIKC_3:
		case DIKC_4:
		case DIKC_5:
		case DIKC_6:
		case DIKC_7:
			if (event->option->private_data != NULL)
			{
				tmp = (MemSongNode *)(event->option->private_data);
				player->NetAddSongToList(tmp); /* MemSongNode */
				event->option->active = true;
				gui->DrawTextOpt(event->option, NULL);
				gui->Flip(&event->option->rect);
			}
			return true;
#if 0
		case DIKC_DELETE:
			if (CurSelectSong)
			{
				DelSongFromList(CurSelectSong);
				CurSelectSong = NULL;
			}
			return true;
		case DIKC_FIRST:
			if (CurSelectSong)
			{
				FirstSong(CurSelectSong, 1);
				CurSelectSong = NULL;
			}
			return true;
#endif
	}
	return CDataBaseWindow::InputProcess(event);
}

/*******************************************************************************
 *	Class CSingerDataWindow 类
 ******************************************************************************/
void CSingerDataWindow::LoadTheme(CKtvTheme *ptheme)
{
	CDataBaseWindow::LoadTheme(ptheme);
	for (int i=0; i<SongOptCount;i++){
		SongListOpt[i]->title = "";
		SongListOpt[i]->tagfilter = 0;
	}
	for (int i=0; i<SingerOptCount;i++){
		char tmpstr[10];
		sprintf(tmpstr, "%d", i+1);
		SingerListOpt[i]->title = tmpstr;
	}
	datadb = theme->singerdata;
}

bool CSingerDataWindow::InputProcess(InputEvent *event)
{
	if (event->option == NULL)
		return CBaseWindow::InputProcess(event);
	switch (event->option->mtv_value)
	{
		case DIKC_1:
		case DIKC_2:
		case DIKC_3:
		case DIKC_4:
		case DIKC_5:
		case DIKC_6:
		case DIKC_7:
		case DIKC_8:
		case DIKC_9:
		case DIKC_0:
			event->option->filter = event->option->title;
			if (event->option->private_data == NULL)
				return true;
	}
	return CDataBaseWindow::InputProcess(event);
}

void CSingerDataWindow::DrawWindowOpt()
{
	TAlign align;
	align.align = taLeftJustify;
	align.valign = taTop;
	CKtvOption *tmp = NULL;
	int i;
	for (i=0; (i<datadb->CurPage.Count) && (i<SongOptCount); i++){
		tmp = SongListOpt[i];
		if (tmp){
			tmp->title = datadb->CurPage.RecList[i]->SongName;
			DrawSingerPhoto(datadb->CurPage.RecList[i]->SongName, SingerListOpt[i]->rect);
			gui->DrawTextOpt(tmp);
			tmp->private_data = (void*)1;
		}
	}
	for (;i<SongOptCount;i++)
	{
		tmp = SongListOpt[i];
		if (tmp)
			tmp->private_data = (void*)0;
	}

	// 将 for 分成两次，这样就不需要每次都换一次字体，提高效率
	for (i=0; (i<datadb->CurPage.Count) && (i<SingerOptCount); i++)
		gui->DrawTextOpt(SingerListOpt[i]);
	DrawPlayStatus();
	ShowActiveChar();
}

bool CSingerDataWindow::DrawSingerPhoto(char *fn, const RECT Rect) /* 显示歌星图片 */
{
	if (fn == NULL) return false;

	char cBuf[BUF_LEN] = {'\0'};

	trim(fn);
	char *data = NULL;
	size_t size = 0;

	sprintf(cBuf, "%s%s%s", DATAPATH"photos/", fn, SINGERPICTYPE);

	int fp = -1;
	if( (fp = open((const char *)cBuf, O_RDONLY, 0)) != -1)
	{
		size = lseek(fp, 0L, SEEK_END);
		data = (char*)malloc(size);
		assert(data);
		if (data != NULL)
		{
			lseek(fp, 0L, SEEK_SET);
			read(fp, data, size);
			close(fp);
		}
	}
	else if (theme->config->haveserver)// 下载数据
		data = (char *)theme->DownSingerPhoto(fn, size);
	if (data)
	{
		gui->DrawImage(data, size, Rect);
		free(data);
		return true;
	}
	return false;
}

bool COtherWindow::InputProcess(InputEvent *event)
{
	if (event->option == NULL)
		return CBaseWindow::InputProcess(event);
	switch (event->option->mtv_value)
	{
		case DIKC_7:
			player->PlayDisc();
			return true;
	}
	return CBaseWindow::InputProcess(event);
}

/*******************************************************************************/
#ifdef ENABLE_HANDWRITE
CHandWriteWindow::CHandWriteWindow(const char *name):CBaseWindow(name), \
	SongNameOpt(NULL), CancelOpt(NULL), HzOptCount(0), HzListOpt(NULL), \
	oldx(0),oldy(0),startwrite(false), activeinput(0), hzcount(0)
{
	hw = new hwInput();
	Input[0] = Input[1] = SongNameOpt = NULL;
}

CHandWriteWindow::~CHandWriteWindow()
{
	free(HzListOpt);
}

void CHandWriteWindow::LoadTheme(CKtvTheme *ptheme)
{
	CBaseWindow::LoadTheme(ptheme);
	Input[0] = FindOption("Input1");
	Input[1] = FindOption("Input2");
	SongNameOpt = FindOption("SongNameLabel");

	if (SongNameOpt)
		SongNameOpt->title = "";
	CKtvOption *tmp = FindOption("OKLabel");
	if (tmp){
		tmp->tagfilter = 3;
	}

	CancelOpt  = FindOption("cancel");
	HzListOpt  = CreateOptList("Num", '1', '5', &HzOptCount);
	for (int i=0;i<HzOptCount;i++)
		HzListOpt[i]->title = "";
}

void CHandWriteWindow::DisplayHwOut()
{
	char buf[255], *c = buf;
	char hz[3];
	int id;
	memset(buf, 0, 255);
	hw->process(buf);
	id = 0;
	while (*c)
	{
		if ( (*c > (char)0x80) && ( *(c+1) > (char)0x80 ) )
		{
			hz[0] = *c;
			hz[1] = *(c+1);
			hz[2] = '\0';
			c+=2;
		}
		else {
			hz[0] = *c;
			hz[1] = '\0';
			c++;
		}
		if (id < HzOptCount) {
			HzListOpt[id]->title = hz;
			gui->UpdateShadow(&HzListOpt[id]->rect);
			gui->DrawTextOpt(HzListOpt[id], NULL);
			id++;
		}
	}
	hzcount = id;
	while (id < HzOptCount){
		HzListOpt[id]->title = "";
		id++;
	}
	gui->Flip();
}

void CHandWriteWindow::WriteFinish(CKtvOption *opt, bool ok)
{
	if (opt) {
		if (ok && (SongNameOpt->title.length() + opt->title.length() < 58) && (hzcount > 0) )
			SongNameOpt->title += opt->title;
	}
	for (int i=0; i<HzOptCount; i++)
		HzListOpt[i]->title[0]='\0';
	gui->UpdateShadow(&SongNameOpt->rect);
	gui->DrawTextOpt(SongNameOpt, NULL, true);
//	gui->Flip();
	hzcount = 0;
	hw->clearPoint();
	activeinput = (activeinput + 1) % 2;
}

void CHandWriteWindow::Show()
{
	SongNameOpt->title = "";
	CBaseWindow::Show();
}

void CHandWriteWindow::DrawWindowOpt()
{
	CBaseWindow::DrawWindowOpt();
	gui->DrawTextOpt(SongNameOpt, NULL, true);
}

bool CHandWriteWindow::InputProcess(InputEvent *event)
{
	if (event->type == IT_MOUSELEFT_DOWN)
	{
		oldx = event->x;
		oldy = event->y;
		if ( POINT_IN_RECT(oldx, oldy, Input[activeinput]->rect, 5) ||
			 POINT_IN_RECT(oldx, oldy, Input[(activeinput+1)%2]->rect, 5) )
		{
			if (!POINT_IN_RECT(oldx, oldy, Input[activeinput]->rect, 5)) // not in active
			{
				WriteFinish(HzListOpt[0], true);
				gui->UpdateShadow(&Input[activeinput]->rect);
			}
			startwrite = true;
			hw->startStroke();
		}
	}
	else if (event->type == IT_MOUSELEFT_UP)
	{
		if (startwrite)
		{
			startwrite = false;
			hw->endStroke();
			DisplayHwOut();
		}
	}
	else if (event->type == IT_MOUSEMOVE)
	{
		if ( (startwrite) && POINT_IN_RECT(event->x, event->y, Input[activeinput]->rect, 5) ) {
			gui->DrawHandWrite(oldx, oldy, event->x, event->y);
			oldx = event->x;
			oldy = event->y;
			hw->addPoint((oldx - Input[activeinput]->rect.left) / 10,
			             (oldy - Input[activeinput]->rect.top) / 10);
		}
	}
	if (event->option == NULL)
		return CBaseWindow::InputProcess(event);
	switch (event->option->mtv_value)
	{
		case DIKC_1:
		case DIKC_2:
		case DIKC_3:
		case DIKC_4:
		case DIKC_5:
			WriteFinish(event->option, true);
			return true;
		case DIKC_6:
			WriteFinish(event->option, false);
			return true;
		case DIKC_7:
		case DIKC_8:
			SongNameOpt->title[0]='\0';
			WriteFinish(NULL, false);
			return true;
		case DIKC_9:
		case DIKC_ENTER:
			WriteFinish(HzListOpt[0], true);
			break;
		case DIKC_0:
			if (event->option->private_data == NULL)
				return true;
	}
	return CBaseWindow::InputProcess(event);
}
#endif
