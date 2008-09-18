#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//#include "memtest.h"
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
		case DIKC_KLOK       : return player->SendPlayerCmdAndRecv("audioswitch"  );
		case DIKC_NEXT       : return player->SendPlayerCmdNoRecv ("playnext"     );
		case DIKC_REPLAY     : return player->SendPlayerCmdNoRecv ("replay"       );
		case DIKC_PAUSE      : return player->SendPlayerCmdAndRecv("PauseContinue");
		case DIKC_HIFI       : return player->SendPlayerCmdNoRecv ("HiSong"       );
		case DIKC_DJ1        : return player->SendPlayerCmdNoRecv ("runscript=dj1");
		case DIKC_DJ2        : return player->SendPlayerCmdNoRecv ("runscript=dj2");
		case DIKC_DJ3        : return player->SendPlayerCmdNoRecv ("runscript=dj3");
		case DIKC_DJ4        : return player->SendPlayerCmdNoRecv ("runscript=dj4");
		case DIKC_MUTE       : return player->SendPlayerCmdAndRecv("setmute"      );
		case DIKC_MENU       : return QuickKeyWindow(MainFormStr      , 0);
		case DIKC_PINYIN     : return QuickKeyWindow(PinYinFormStr    , 1);
		case DIKC_SINGER     : return QuickKeyWindow(SingerClassFrmStr, 1);
		case DIKC_LANG       : return QuickKeyWindow(YuYanFormStr     , 1);
		case DIKC_CODE       : return QuickKeyWindow(InputCodeFormStr , 1);
		case DIKC_CLASS      : return QuickKeyWindow(ClassFormStr     , 1);
		case DIKC_WORDNUM    : return QuickKeyWindow(WordNumFormStr   , 1);
		case DIKC_WBH        : return QuickKeyWindow(WBHFormStr       , 1);
		case DIKC_SWITCHLANG : return QuickKeyWindow(SwitchLangFormStr, -1);
		case DIKC_VOLUME_UP  : ShowVolumeBox(player->AddVolume()); return true;
		case DIKC_VOLUME_DOWN: ShowVolumeBox(player->DecVolume()); return true;
		case DIKC_SORT       : {
			CMainWindow *tmpwindow = (CMainWindow*) mytheme->FindWindow(MainFormStr);
			if (tmpwindow) {
				stack->StackClean(1);
				tmpwindow->HotSong();
			}
			return true;
		}
		case DIKC_ESCAPE: {
			CBaseWindow *CurWindow = stack->WindowTop();
			if (CurWindow) CurWindow->Close();
			break;
		}
		case DIKC_NEWS    : 
		case DIKC_PLUNGINS: return true;
		case DIKC_QUIT    : quit = true; return true;
	}
	return false;
}

int main(int argc, char **argv)
{
//#ifdef MEMWATCH
//	mwStatistics( 2 );
//	mwAutoCheck( 1 );
//	TRACE("Hello world!\n");
//#endif
	CKtvApplication app(argc, argv);
	app.run();
	return 0;
}
