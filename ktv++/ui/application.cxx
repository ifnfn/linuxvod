#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "memtest.h"

#include "application.h"
#include "player.h"

void *KeyReadThread(void *p) // 友员函数，用于线程，功能：按键操作缓冲
{
	InputEvent event;
	if (p == NULL) return NULL;
	CApplication *cp = (CApplication *)p;
	int timeout = 800;
	while( !cp->quit ){
		memset(&event, 0, sizeof(InputEvent));
		if (cp->gui->WaitInputEvent(&event, timeout)){
//			DEBUG_OUT("%c, %d,%d\n", event.k, event.x, event.y);
			ftime(&cp->gui->opttime);
			cp->KeyBuffer->KeyIn(event);
		}
	}
	return NULL;
}

CApplication::CApplication(int argc, char **argv): quit(false)
{
	GetTimerManager();
	gui          = CScreen::CreateScreen()->GetGui();
	stack        = CWindowStack::CreateStack();
	KeyBuffer    = CKeyBuffer::CreateKeyBuf();

	char ServerIP[20]="";
	bool haveserver =false;
	if ((argc == 2) && (strcmp(argv[1], "--noserver") == 0)){
	}
	else {
//		haveserver = FindServerHost(ServerIP, argv[0]);
//		if (!haveserver)
//			DEBUG_OUT("No Found Server.\n");
	}
	configure = new CMtvConfig(ServerIP);
	configure->haveserver = haveserver;

	player     = new CPlayer(configure->playerip);
	mytheme    = new CKtvTheme(configure);
	player->theme = mytheme;

	if (mytheme == NULL) {
		printf("mytheme = NULL\n");
		exit(1);
	}
	CMsgWindow::Create(mytheme);
	gui->GraphicInit(argc, argv);
	CBaseWindow* main = (CBaseWindow*) mytheme->FindWindow(MainFormStr);
	if (main) main->Show();
	player->CheckVideoCard();
}

CApplication::~CApplication()
{
	quit = true;
	delete player;
	delete mytheme;
	delete configure;
}

void CApplication::run()
{
	if (pthread_create(&keyreadthread , NULL, KeyReadThread, this) != 0)
		DEBUG_OUT("pthread_create KeyReadThread error.\n");

#ifdef CONSOLEINPUT
	if (pthread_create(&consolethread, NULL, ConsoleInputThread, this) != 0)
		DEBUG_OUT("pthread_create ConsoleInputThread error.\n");
#endif

	InputEvent event;
	while( !quit ){
		memset(&event, 0, sizeof(InputEvent));
		if (KeyBuffer->KeyOut(&event)) {
			ftime(&gui->opttime);
			if ((configure->testkey) && (event.type == IT_KEY_DOWN)) {
				PrintKeyValue(&event);
				continue;
			}

			CBaseWindow *CurWindow = stack->WindowTop();
			if (CurWindow){
				switch (event.type){
					case IT_KEY_DOWN:
						event.option = CurWindow->FindOption(event.k);
						break;
					case IT_MOUSELEFT_DOWN:
					case IT_MOUSEMIDDLE_DOWN:
						event.option = CurWindow->FindOption(event.x, event.y);
						break;
					case IT_MOUSERIGHT_DOWN:
						CurWindow->Close();
						break;
					case IT_MOUSEMOVE:
					case IT_KEY_UP:
					case IT_MOUSELEFT_UP:
					case IT_MOUSERIGHT_UP:
					case IT_MOUSEMIDDLE_UP:
						break;
				}
				if ( !CurWindow->InputProcess(&event) ) {
					int key = 0;
					if (event.option != NULL)
						key = event.option->mtv_value;
					else if (event.type == IT_KEY_DOWN)
						key = FindMtvKey(event.k);
					GlobalInputProcess(key);
//					GlobalInputProcess(FindMtvKey(event.k));
//					GlobalInputProcess(event.option->mtv_value);
				}
			}
			else {
				printf("No WindowTop\n");
				break;
			}
		} else
			DEBUG_OUT("BACK\n");
	}
}

#ifdef CONSOLEINPUT
void *ConsoleInputThread(void *p)
{
	InputEvent event;
	if (p == NULL) return NULL;
	CApplication *cp = (CApplication *)p;
	while (!cp->quit) {
		memset(&event, 0, sizeof(InputEvent));
		event.k= getchar();
		printf("getchar=%c\n", event.k);
//		cp->quit = true;
		event.type = IT_KEY_DOWN;
		cp->KeyBuffer->KeyIn(event);
		event.type = IT_KEY_UP;
		cp->KeyBuffer->KeyIn(event);
	}
//	printf("exit ConsoleInuptThread\n");
	return NULL;
}
#endif

bool CApplication::GlobalInputProcess(int KtvKeyValue)  /* 全局按输入处理事件 */
{
	return false;
}

void CApplication::PrintKeyValue(InputEvent *event)
{
	char buf[50];
	key_struct *k =	FindMtvKeyByKeyValue(event->k);
	if (k)
		sprintf(buf, "(%s)=%d", k->title, event->k);
	else
		sprintf(buf, "(未知键)=%d", event->k);
	ShowMsgBox(buf, 0);
}

