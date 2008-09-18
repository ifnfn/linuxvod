#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//#include "memtest.h"

#include "application.h"
#include "player.h"

void *KeyReadThread(void *p) // ��Ա�����������̣߳����ܣ�������������
{
	InputEvent event;
	if (p == NULL) return NULL;
	CApplication *cp = (CApplication *)p;
	int timeout = 800;
	while( !cp->quit ){
		memset(&event, 0, sizeof(InputEvent));
		if (cp->gui->WaitInputEvent(&event, timeout))
			cp->KeyBuffer->KeyIn(event);
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
		haveserver = FindServerHost(ServerIP, argv[0]);
		if (!haveserver)
			DEBUG_OUT("No Found Server.\n");
		else
			printf("Found Server: %s\n", ServerIP);
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
			if (event.type == IT_QUIT) {
				quit = 1;
				continue;
			}
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
					case IT_QUIT:
						quit = 1;
						break;
				}
				if ( !CurWindow->InputProcess(&event) ) {
					int key = 0;
					if (event.option != NULL)
						key = event.option->mtv_value;
					else if (event.type == IT_KEY_DOWN)
						key = FindMtvKey(event.k);
					GlobalInputProcess(key);
				}
			}
			else {
				printf("No WindowTop\n");
				break;
			}
		}
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

bool CApplication::GlobalInputProcess(int KtvKeyValue)  /* ȫ�ְ����봦���¼� */
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
		sprintf(buf, "(δ֪��)=%d", event->k);
	ShowMsgBox(buf, 0);
}

