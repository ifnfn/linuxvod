#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "gui.h"
#include "xmltheme.h"
#include "player.h"
#include "windowmanage.h"
#include "timer.h"


class CKeyFreeTime: public CBaseObject
{
public:
	CKeyFreeTime() {
		stack = CWindowStack::CreateStack();
	}
protected:
	bool StartTimer(const uint8_t timerID, const uint32_t dwInterval);
private:
	CWindowStack *stack;

};

class CApplication
{
public:
	CApplication(int argc, char **argv);
	virtual ~CApplication();
	void run();
	virtual void ApplicationInit() {}
	CKtvTheme *mytheme;
protected:
	virtual bool GlobalInputProcess(int KtvKeyValue);
	CBaseGui *gui        ;
	CWindowStack *stack  ;
	CMtvConfig *configure;
	bool quit;
private:
	CKeyBuffer *KeyBuffer;
//	CTimerManager *timermanager;
	void PrintKeyValue(InputEvent *event);

	pthread_t keyreadthread;
	pthread_t consolethread;

	friend void *KeyReadThread(void *p);
	friend void *ConsoleInputThread(void *p);
};

#endif

