#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>

#include "memtest.h"
#include "xmltheme.h"
#include "strext.h"
#include "key.h"
#include "timer.h"
#include "win.h"

const char *MainFormStr      = "MainForm"      ;
const char *SingerClassFrmStr= "SingerClassFrm";
const char *YuYanFormStr     = "YuYanForm"     ;
const char *ClassFormStr     = "ClassForm"     ;
const char *WordNumFormStr   = "WordNumForm"   ;
const char *SelectedFormStr  = "SelectedForm"  ;
const char *OtherFormStr     = "OtherForm"     ;
const char *PinYinFormStr    = "PinYinForm"    ;
const char *MyLoveFormStr    = "MyLoveForm"    ;
const char *WBHFormStr       = "WBHForm"       ;
const char *SongListFormStr  = "SongListForm"  ;
const char *InputCodeFormStr = "InputCodeForm" ;
const char *NumLocateFormStr = "NumLocateForm" ;
const char *SingerFormStr    = "SingerForm"    ;
const char *SoftInfoFormStr  = "SoftInfoForm"  ;
const char *PlunginsFormStr  = "PlunginsForm"  ;

#ifdef ENABLE_FOODMENU
const char *FoodMenuFormStr  = "FoodMenuForm"  ;
const char *MenuTypeFormStr  = "MenuTypeForm"  ;
const char *MenuLoginFormStr = "MenuLoginForm" ;
#endif

#ifdef ENABLE_HANDWRITE
const char *HandWriteFormStr = "HandWriteForm" ;
#endif

#ifdef ENABLE_MULTLANGIAGE
const char *SwitchLangFormStr= "SwitchLangForm" ;
#endif

const char *PlunginsPath     = "plungins"      ;
const char *SingerPhotoPath  = "photos"        ;

#ifdef DEBUG
void CKtvOption::PrintOption()
{
	DEBUG_OUT("\tOpt name       = %s\n", name.data() );
	DEBUG_OUT("\tOpt keyname    = %s\n", keyname.data() );
	DEBUG_OUT("\tOpt sys_value  = %d\n", sys_value);
	DEBUG_OUT("\tOpt mtv_value  = %d\n", mtv_value);
	DEBUG_OUT("\tOpt title      = %s\n", title.data() );
	DEBUG_OUT("\tOpt link       = %s\n", link.data() );
	DEBUG_OUT("\tOpt filter     = %s\n", filter.data() );
}

void CKtvWindow::PrintOptList()
{
	DEBUG_OUT("[%s]\n", WinName.data());
	for (CKtvOption *i = OptionList.First(); i; i = OptionList.Next(i) )
	{
		i->PrintOption();
		DEBUG_OUT("\n");
	}
}
#endif

/*******************************************************************************************
	funname: CleanWindow
********************************************************************************************/
void CKtvWindow::CleanWindow()
{
#ifdef OPTSORT
	opt_num =0;

	if (sortopt) {
		free(sortopt);
		sortopt = NULL; // 释放操作项索引
	}
	optsorttype = sortOther;
#endif
	FreeImageBuffer();
	CurOption = NULL;
//	PrintOptList();
	OptionList.Clear();
//TODO
}

CKtvWindow::CKtvWindow(const char *name): font(NULL), ImageBuffer(NULL), \
	ImageBufferSize(0), EmptyShadow(NULL), CurOption(NULL)
#ifdef OPTSORT
	, sortopt(NULL), opt_num(0), optsorttype(sortOther)
#endif
{
	WinName = name;
	workrect.left   = workrect.top = 0;
	workrect.right  = SCREEN_WIDTH;
	workrect.bottom = SCREEN_HEIGHT;
	gui   = CScreen::CreateScreen()->GetGui();
	stack = CWindowStack::CreateStack();
}

CKtvWindow::~CKtvWindow()
{
	CleanWindow();
}

void CKtvWindow::FreeImageBuffer()
{
	if (ImageBuffer) {
		free(ImageBuffer);
		ImageBuffer = NULL;
	}
	ImageBufferSize = 0;
}

bool CBaseObject::StartTimer(const uint8_t timerID, const uint32_t dwInterval)
{
	return GetTimerManager()->StartTimer(this, timerID, dwInterval);
}

bool CBaseObject::KillTimer(const uint8_t timerID)
{
	return GetTimerManager()->KillTimer(this, timerID);
}

// 将屏幕的绝对址转换成本窗体的相对地址
RECT CKtvWindow::ScreenToClient(RECT rect)
{
	rect.left   += workrect.left;
	rect.top    += workrect.top;
	rect.right  += workrect.left;
	rect.bottom += workrect.top;
	return rect;
}

#ifdef OPTSORT
void CKtvWindow::SortByOpt(SortType newsort)
{
	if (optsorttype == newsort) return;
	optsorttype = newsort;
	CKtvOption *tmpBuf;
	int k, cmpk;
	for(int i=0; i< opt_num-1; i++)
	{
		k = i;
		for(int j=i; j < opt_num; j++)
		{
			switch (optsorttype)
			{
				case sortKey:
					cmpk = sortopt[k]->sys_value - sortopt[j]->sys_value;
					break;
				case sortName:
				default:
					cmpk = strcasecmp(sortopt[k]->name.data(), sortopt[j]->name.data());
					break;
			}
			if (cmpk > 0)
				k = j;
		}
		if( i != k)
		{
			tmpBuf     = sortopt[i];
			sortopt[i] = sortopt[k];
			sortopt[k] = tmpBuf;
		}
	}
}

void CKtvWindow::CreateSortOpt(SortType newsort) /* 建立排序操作项列表 */
{
	if (sortopt) free(sortopt);
	sortopt = (CKtvOption **)malloc(sizeof(CKtvOption*) * opt_num);
	assert(sortopt);
	memset(sortopt, 0, sizeof(CKtvOption*) * opt_num);
	int i=0;
	for (CKtvOption *node = OptionList.First(); node = OptionList.Next(node) )
		sortopt[i++] = node;
	SortByOpt(newsort);
}
#endif

void CKtvWindow::AddOption(CKtvOption *newopt)
{
	OptionList.Add(newopt);
#ifdef OPTSORT
	opt_num++;
#endif
}

CKtvOption *CKtvWindow::FindOption(const char *name)
{
#ifdef OPTSORT
	SortByOpt(sortName);   // 如果已经排序了，就不会再排序了
	if (opt_num == 0) {
		return NULL;
	}
	int Low = 0, High = opt_num - 1, Mid;
	int k;
	int count = 0;
	while(Low <= High)
	{
		Mid = (Low + High) / 2;
		k = strcasecmp(sortopt[Mid]->name.data(), name);
		if (k < 0)
			Low = Mid + 1;
		else if(k > 0)
			High = Mid - 1;
		else
			return sortopt[Mid];
		count++;
	}
#else
	if (CurOption)
	{
		if (strcasecmp(CurOption->name.data(), name) == 0) {
			return CurOption;
		}
	}
	for (CKtvOption *p = OptionList.First(); p; p = OptionList.Next(p) )
	{
		if (strcasecmp(p->name.data(), name) == 0)
		{
			CurOption = p;
			return CurOption;
		}
	}
#endif
	return NULL;
}

CKtvOption *CKtvWindow::FindOption(int x, int y)
{
	if (CurOption)
	{
		if ((CurOption->mtv_value!= DIKC_NULL) &&
			(x >= CurOption->rect.left && x <= CurOption->rect.right) &&
			(y >= CurOption->rect.top && y <= CurOption->rect.bottom))
		{
			return CurOption;
		}
	}
#ifdef OPTSORT
	for(int i=0; i<opt_num; i++)
	{
		CurOption = sortopt[i];
		if ((CurOption->mtv_value!= DIKC_NULL) &&
			(x >= CurOption->rect.left && x <= CurOption->rect.right) &&
			(y >= CurOption->rect.top && y <= CurOption->rect.bottom))
		{
			return CurOption;
		}
	}
	CurOption = NULL;
#else
	for (CKtvOption *i = OptionList.First(); i; i = OptionList.Next(i) )
	{
		if ( (i->mtv_value != DIKC_NULL) &&
			(x >= i->rect.left && x <= i->rect.right) &&
			(y >= i->rect.top  && y <= i->rect.bottom) )
		{
			CurOption = i;
			return CurOption;
		}
	}
#endif
	return NULL;
}

// 将键盘按键转成MtvKey键值
CKtvOption *CKtvWindow::FindOption(int32_t sys_value)
{
	if (CurOption)
	{
		if (CurOption->sys_value == sys_value)
		{
			return CurOption;
		}
	}
#ifdef OPTSORT
	SortByOpt(sortKey);   // 如果已经排序了，就不会再排序了

	int Low = 0, High = opt_num - 1, Mid;
	long k;
	int count = 0;
	while(Low <= High)
	{
		count++;
		Mid = (Low + High) / 2;
		k = sortopt[Mid]->sys_value - sys_value;
		if (k < 0)
			Low = Mid + 1;
		else if(k > 0)
			High = Mid - 1;
		else {
			CurOption = sortopt[Mid];
			return sortopt[Mid];
		}
	}
#else
	for (CKtvOption *i = OptionList.First(); i; i = OptionList.Next(i) )
	{
		if (i->sys_value == sys_value) {
			CurOption = i;
			return CurOption;
		}
	}
#endif
	return NULL;
}

void CKtvWindow::SetImageBuffer(void *data, long len)           // 设置背景缓冲区
{
	FreeImageBuffer();
	ImageBufferSize = len;
	ImageBuffer = (unsigned char *)malloc(len);
	memcpy(ImageBuffer, data, len);
}

/*============================================================================*/
CKtvTheme::CKtvTheme(CMtvConfig *configure): config(configure), docfp(NULL)
{
	songdata   = new CData(configure->songdata);
	singerdata = new CData(configure->singerdata);
	DiskCache = CDiskCache::GetDiskCache(NULL, NULL);
	DataStream   = new CMemoryStream();
	CommonWindow = new CKtvWindow("commonoptions");
	fonts = CFontFactory::CreateFontFactory();
	CreateAllWindow();

	if (LoadThemeFromFile(config->defaulttheme) == false) {
		fprintf(stderr, "open theme %s error.\n", config->defaulttheme.data());
		abort();
	}
}

CKtvTheme::~CKtvTheme()
{
	CloseTheme();
	delete songdata;
	delete singerdata;
	delete DataStream;
}

void CKtvTheme::CloseTheme()
{
//TODO
	WindowList.Clear();
}

void *CKtvTheme::DownSingerPhoto(const char *singername, size_t& len)
{
	char fn[512];
	pathcat(fn, "/photos", singername);
	strcat(fn, SINGERPICTYPE);
	printf("Download Singer Photo: %s\n", fn);
	return DiskCache->ReadBuffer((char*)fn, len, true);
}

bool CKtvTheme::LoadThemeFromFile(CString& themefile) // 读界面主题
{
	char url[1024];

	config->geturl(url, PlunginsPath, themefile.data());
	DataStream->ClearData();
	if (DataStream->LoadFromUrl(url, true) == false)
		return false;

	if(DataStream->Read(&head, sizeof(PlungHead)) == 0){
		DEBUG_OUT("Read theme head error.\n");
		return false;
	}

	for(int i=0; i<head.filecount; i++){
		Trim(head.filelist[i].filename);
		lcase(head.filelist[i].filename);
//		DEBUG_OUT("%s\n", head.filelist[i].filename);
	}

	if (docfp)
		delete (TiXmlDocument *)docfp;
	docfp = new TiXmlDocument();

	TiXmlDocument *doc = (TiXmlDocument *)docfp;
	doc->Clear();

	long MtvDataLen = 0;
	char *MtvXml = (char *)ReadSegment("ktv", &MtvDataLen);
	doc->Parse(MtvXml);
	free(MtvXml);
	CreateKtvWindow(CommonWindow);
	LoadWindowData();
//--------------------------------------------------------------------------//
	CommonWindow->CleanWindow();
	DataStream->ClearData();
	if (docfp) {
		((TiXmlDocument *)docfp)->Clear();
		delete ((TiXmlDocument *)docfp);
		docfp  = NULL;
	}
	return true;
}

static TTextLayout atoaligndef(const char *sptr, TTextLayout align)
{
	if (sptr == NULL)
		return align;
	else if (!strcasecmp("left", sptr))
		return taLeftJustify;
	if (!strcasecmp("right", sptr))
		return taRightJustify;
	else if (!strcasecmp("center", sptr))
		return taCenter;
	else if (!strcasecmp("top", sptr))
		return taTop;
	else if (!strcasecmp("bottom", sptr))
		return taBottom;
	else
		return align;
}

void *CKtvTheme::FindXmlWindow(CString& winname)
{
	TiXmlNode *node = ((TiXmlDocument *)docfp)->FirstChild("mtv");
	if (node != NULL)
	{
		TiXmlElement* WindowNode = node->FirstChildElement("window");
		while (WindowNode)
		{
			if (winname == WindowNode->Attribute("name"))
				return WindowNode;
			WindowNode = WindowNode->NextSiblingElement("window");
		}
	} else
		DEBUG_OUT("XmlNode (mtv) no found\n");
	return NULL;
}

bool CKtvTheme::CreateKtvWindow(CKtvWindow* window, TiXmlElement *win)
{
	bool usecommon = window->WinName != "commonoptions";
	TiXmlElement *WindowNode;
	if (win)
		WindowNode = win;
	else
		WindowNode = (TiXmlElement *)FindXmlWindow(window->WinName);
	if (WindowNode == NULL) {
		DEBUG_OUT("FindXmlWindow(%s) error.\n", window->WinName.data());
		return false;
	}

	TiXmlElement *OptionNode, *tmpNode = NULL;
	CKtvOption *tmpoption = NULL;
	window->WinName = WindowNode->Attribute("name");

	window->workrect.left   = atoidef(WindowNode->Attribute("left"  ), 0);
	window->workrect.top    = atoidef(WindowNode->Attribute("top"   ), 0);
	window->workrect.right  = atoidef(WindowNode->Attribute("right" ), 0);
	window->workrect.bottom = atoidef(WindowNode->Attribute("bottom"), 0);

	tmpNode = WindowNode->FirstChildElement("font");
	while (tmpNode)
	{
		fonts->CreateFont(tmpNode->Attribute("name"), tmpNode->Attribute("use"));
		tmpNode = tmpNode->NextSiblingElement("font");
	}
	window->font = FindFont(WindowNode->Attribute("font"));

	if ((window->font == NULL) && usecommon)
		window->font = CommonWindow->font;
	CKtvOption *tmp = NULL;
	OptionNode = WindowNode->FirstChildElement("option");
	const char *optname = NULL;
	while (OptionNode)
	{
		optname = OptionNode->Attribute("name");
		tmpoption = new CKtvOption();
		tmpoption->name = optname;

		if (usecommon)
			tmp = CommonWindow->FindOption(tmpoption->name.data());
		else
			tmp = NULL;

		tmpoption->align.align  = atoaligndef(OptionNode->Attribute("align"), taLeftJustify);
		tmpoption->align.valign = atoaligndef(OptionNode->Attribute("valign"), taTop);
		tmpNode = OptionNode->FirstChildElement("font");
		if (tmpNode)
			tmpoption->font = fonts->CreateFont(tmpNode->Attribute("name"), tmpNode->Attribute("use"));
		else if (window->font)
			tmpoption->font = window->font;
		else if (usecommon)
			tmpoption->font = CommonWindow->font;

		tmpNode = OptionNode->FirstChildElement("activecolor");
		if (tmpNode)
			tmpoption->activecolor = StrToColor(tmpNode->Attribute("use"));

//		DEBUG_OUT("tmpNode = OptionNode->FirstChildElement(\"key\");\n");
		tmpNode = OptionNode->FirstChildElement("key");
		if (tmpNode)
		{
			tmpoption->keyname = tmpNode->Attribute("use");
			key_struct *ks = FindMtvKeyByKeyName(tmpoption->keyname.data());
			if (ks){
				tmpoption->sys_value = ks->sys_value;
				tmpoption->mtv_value = ks->mtv_value;
			}
		}
		else if (tmp)
		{
			tmpoption->keyname = tmp->keyname;
			tmpoption->sys_value = tmp->sys_value;
			tmpoption->mtv_value = tmp->mtv_value;
		}

//		DEBUG_OUT("tmpNode = OptionNode->FirstChildElement(\"rect\");\n");
		tmpNode = OptionNode->FirstChildElement("rect");
		if (tmpNode)
		{
			tmpoption->rect.left  = atoidef(tmpNode->Attribute("left"  ), 0);
			tmpoption->rect.top   = atoidef(tmpNode->Attribute("top"   ), 0);
			tmpoption->rect.right = atoidef(tmpNode->Attribute("right" ), 0);
			tmpoption->rect.bottom= atoidef(tmpNode->Attribute("bottom"), 0);
			tmpoption->rect = window->ScreenToClient(tmpoption->rect);
		}
		else if (tmp)
			tmpoption->rect  = tmp->rect;

//		DEBUG_OUT("tmpNode = OptionNode->FirstChildElement(\"title\");\n");
		tmpNode = OptionNode->FirstChildElement("title");
		if (tmpNode)
			tmpoption->title = tmpNode->Attribute("use");
		else if (tmp)
			tmpoption->title = tmp->title;

//		DEBUG_OUT("tmpNode = OptionNode->FirstChildElement(\"link\");\n");
		tmpNode = OptionNode->FirstChildElement("link");
		if (tmpNode)
			tmpoption->link = tmpNode->Attribute("use");
		else if (tmp)
			tmpoption->link = tmp->link;
//		DEBUG_OUT("tmpNode = OptionNode->FirstChildElement(\"filter\");\n");
		tmpNode = OptionNode->FirstChildElement("filter");
		if (tmpNode)
			tmpoption->filter = tmpNode->Attribute("use");
		else if (tmp)
			tmpoption->filter = tmp->filter;
		window->AddOption(tmpoption);
//		tmpoption->PrintOption();
		OptionNode = OptionNode->NextSiblingElement("option");

	}
#ifdef DEBUG
	window->PrintOptList();
#endif

#ifdef OPTSORT
	window->CreateSortOpt(sortName);
#endif
	long len;
	unsigned char *tmpImage = (unsigned char *)ReadSegment(WindowNode->Attribute("backgroup"), &len);
	window->SetImageBuffer(tmpImage, len);
	free(tmpImage);
	return true;
}

int CKtvTheme::GetSegIndex(const char *SegName)
{
	if (SegName == NULL) return -1;
	for(int i=0; i < head.filecount; i++)
	{
		if(!strcasecmp(SegName, head.filelist[i].filename))
			return i;
	}
	return -1;
}

void *CKtvTheme::ReadSegment(const char *SegName, long *datasize)
{
	int index = GetSegIndex(SegName);
	if (index >= 0)
	{
		DataStream->Seek(head.filelist[index].position, SEEK_SET);
		MemSize = head.filelist[index].size;
		char *Memory = (char *)malloc(MemSize);
		if (Memory)
			*datasize = DataStream->Read(Memory, MemSize);
		else
			*datasize = 0;
		return Memory;
	}
	*datasize = 0;
	return NULL;
}

CKtvWindow *CKtvTheme::FindWindow(const char *WinName)
{
	for (CKtvWindow *i = WindowList.First(); i; i = WindowList.Next(i) )
		if (i->WinName == WinName) return i;
	return NULL;
}

CBaseWindow* CKtvTheme::CreateWindow(const char *winname)
{
	CBaseWindow *tmp = NULL;
	if (!strcasecmp(winname, MainFormStr))            tmp = new CMainWindow      (winname);
	else if (!strcasecmp(winname, SingerClassFrmStr)) tmp = new CBaseWindow      (winname);
	else if (!strcasecmp(winname, YuYanFormStr))      tmp = new CBaseWindow      (winname);
	else if (!strcasecmp(winname, ClassFormStr))      tmp = new CBaseWindow      (winname);
	else if (!strcasecmp(winname, WordNumFormStr))    tmp = new CBaseWindow      (winname);
	else if (!strcasecmp(winname, SelectedFormStr))   tmp = new CSelectedWindow  (winname);
	else if (!strcasecmp(winname, OtherFormStr))      tmp = new COtherWindow     (winname);
	else if (!strcasecmp(winname, PinYinFormStr))     tmp = new CPinYinWindow    (winname);
	else if (!strcasecmp(winname, MyLoveFormStr))     tmp = new CMyLoveWindow    (winname);
	else if (!strcasecmp(winname, WBHFormStr))        tmp = new CWBHWindow       (winname);
	else if (!strcasecmp(winname, SongListFormStr))   tmp = new CSongDataWindow  (winname);
	else if (!strcasecmp(winname, InputCodeFormStr))  tmp = new CInputCodeWindow (winname);
	else if (!strcasecmp(winname, NumLocateFormStr))  tmp = new CNumLocateWindow (winname);
	else if (!strcasecmp(winname, SingerFormStr))     tmp = new CSingerDataWindow(winname);
	else if (!strcasecmp(winname, PlunginsFormStr))   tmp = new CThemeWindow     (winname);
#ifdef ENABLE_HANDWRITE
	else if (!strcasecmp(winname, HandWriteFormStr))  tmp = new CHandWriteWindow (winname);
#endif
	else if (!strcasecmp(winname, SoftInfoFormStr))   tmp = new CSoftInfoWindow  (winname);
#ifdef ENABLE_FOODMENU
	else if (!strcasecmp(winname, FoodMenuFormStr))   tmp = new CFoodMenuWindow  (winname);
	else if (!strcasecmp(winname, MenuTypeFormStr))   tmp = new CMenuTypeWindow  (winname);
	else if (!strcasecmp(winname, MenuLoginFormStr))  tmp = new CMenuLoginWindow (winname);
#endif
#ifdef ENABLE_MULTLANGIAGE
	else if (!strcasecmp(winname, SwitchLangFormStr)) tmp = new CSwitchLangWindow(winname);
#endif
	if (tmp)
		WindowList.Add(tmp);
	return tmp;
}

void CKtvTheme::CreateAllWindow()
{
	CreateWindow(MainFormStr      );
	CreateWindow(SingerClassFrmStr);
	CreateWindow(YuYanFormStr     );
	CreateWindow(ClassFormStr     );
	CreateWindow(WordNumFormStr   );
	CreateWindow(SelectedFormStr  );
	CreateWindow(OtherFormStr     );
	CreateWindow(PinYinFormStr    );
	CreateWindow(MyLoveFormStr    );
	CreateWindow(WBHFormStr       );
	CreateWindow(SongListFormStr  );
	CreateWindow(InputCodeFormStr );
	CreateWindow(NumLocateFormStr );
	CreateWindow(SingerFormStr    );
	CreateWindow(SoftInfoFormStr  );
	CreateWindow(PlunginsFormStr  );
#ifdef ENABLE_FOODMENU
	CreateWindow(FoodMenuFormStr  );
	CreateWindow(MenuLoginFormStr );
	CreateWindow(MenuTypeFormStr  );
#endif

#ifdef ENABLE_HANDWRITE
	CreateWindow(HandWriteFormStr );
#endif
#ifdef ENABLE_MULTLANGIAGE
	CreateWindow(SwitchLangFormStr);
#endif
}

void CKtvTheme::LoadWindowData()
{
	for (CKtvWindow *i = WindowList.First(); i; i = WindowList.Next(i) )
	{
		i->CleanWindow();
		CreateKtvWindow(i);
		((CBaseWindow*)i)->LoadTheme(this);
	}
}
