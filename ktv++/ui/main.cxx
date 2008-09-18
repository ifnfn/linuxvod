#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "memtest.h"
#include "gui.h"
#include "win.h"
#include "player.h"
#include "application.h"
#include "main.h"
#include "diskcache.h"

using namespace std;

bool CKtvApplication::QuickKeyWindow(const char *winname, int stacknum)
{
	CBaseWindow *tmpwindow = (CBaseWindow*) mytheme->FindWindow(winname);
	if (tmpwindow) {
		if (stacknum >= 0) {
			stack->StackClean(stacknum);
			tmpwindow->Show();
		}
		return true;
	}
	return false;
}

bool CKtvApplication::GlobalInputProcess(int KtvKeyValue)
{
	switch(KtvKeyValue)
	{
		case DIKC_KLOK   : player->SendPlayerCmdAndRecv("audioswitch")  ; return true;
		case DIKC_NEXT   : player->SendPlayerCmdNoRecv ("playnext")     ; return true;
		case DIKC_REPLAY : player->SendPlayerCmdNoRecv ("replay")       ; return true;
		case DIKC_PAUSE  : player->SendPlayerCmdAndRecv("PauseContinue"); return true;
		case DIKC_HIFI   : player->SendPlayerCmdNoRecv ("HiSong")       ; return true;
		case DIKC_DJ1    : player->SendPlayerCmdNoRecv ("runscript=dj1"); return true;
		case DIKC_DJ2    : player->SendPlayerCmdNoRecv ("runscript=dj2"); return true;
		case DIKC_DJ3    : player->SendPlayerCmdNoRecv ("runscript=dj3"); return true;
		case DIKC_DJ4    : player->SendPlayerCmdNoRecv ("runscript=dj4"); return true;
		case DIKC_MUTE   : player->SendPlayerCmdNoRecv ("setmute")      ; return true;
		case DIKC_MENU   : return QuickKeyWindow(MainFormStr      , 0);
		case DIKC_PINYIN : return QuickKeyWindow(PinYinFormStr    , 1);
		case DIKC_SINGER : return QuickKeyWindow(SingerClassFrmStr, 1);
		case DIKC_LANG   : return QuickKeyWindow(YuYanFormStr     , 1);
		case DIKC_CODE   : return QuickKeyWindow(InputCodeFormStr , 1);
		case DIKC_CLASS  : return QuickKeyWindow(ClassFormStr     , 1);
		case DIKC_WORDNUM: return QuickKeyWindow(WordNumFormStr   , 1);
		case DIKC_WBH    : return QuickKeyWindow(WBHFormStr       , 1);
		case DIKC_SWITCHLANG: return QuickKeyWindow(SwitchLangFormStr, -1);
		case DIKC_SORT   : {
			CMainWindow *tmpwindow = (CMainWindow*) mytheme->FindWindow(MainFormStr);
			if (tmpwindow) {
				stack->StackClean(1);
				tmpwindow->HotSong();
			}
			return true;
		}
		case DIKC_VOLUME_UP:
			player->SendPlayerCmdNoRecv("addvolume");
			ShowVolumeBox(player->ReadIntFromPlayer());
			return true;
		case DIKC_VOLUME_DOWN:
			player->SendPlayerCmdNoRecv("delvolume");
			ShowVolumeBox(player->ReadIntFromPlayer());
			return true;
		case DIKC_ESCAPE: {
			CBaseWindow *CurWindow = stack->WindowTop();
			if (CurWindow) CurWindow->Close();
			break;
		}
		case DIKC_NEWS    : return true;
		case DIKC_PLUNGINS: return true;
		case DIKC_QUIT    : quit = true; return true;
	}
	return false;
}

int main(int argc, char **argv)
{
#ifdef MEMWATCH
	mwStatistics( 2 );
	mwAutoCheck( 1 );
	TRACE("Hello world!\n");
#endif
	CKtvApplication app(argc, argv);
	app.run();
	return 0;
}
