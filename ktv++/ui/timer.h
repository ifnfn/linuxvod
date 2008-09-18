#ifndef TIMER_H
#define TIMER_H

#include "windowmanage.h"
#include "list.h"

class CTimer
{
public:
	CTimer(CBaseObject* pTarget, uint8_t timerID, uint32_t dwInterval)
		: TimerID(timerID), Interval(dwInterval), Count(dwInterval), Target(pTarget){}
	void Tick(void);
	uint8_t TimerID;
	uint32_t Interval;
	uint32_t Count;
	CBaseObject* Target;
protected:

};

class CTimerManager
{
public:
	static CTimerManager * GetTimerManager(void);

	static void DeleteTimerManager(void);

	bool StartTimer(CBaseObject* pTarget, uint8_t timerID, uint32_t dwInterval);
	bool KillTimer(CBaseObject*pTarget, uint8_t timerID);
	bool KillAllTimer();
protected:
	CTimerManager();
	~CTimerManager() {
		working = false;
		KillAllTimer();
	}
	pthread_mutex_t CS;
private:
	static CTimerManager * m_pTimerManager;
	CTimer **TimerList;
	int count;
	bool working;
	pthread_t pthread;
	friend void *TimerTickThread(void *p);
};

CTimerManager* GetTimerManager();
#endif
