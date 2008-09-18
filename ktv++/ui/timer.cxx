#include <pthread.h>

#include "memtest.h"
#include "timer.h"

void CTimer::Tick(void)
{
	Count -= 100;
//	printf("%d = %d\n", TimerID, Count);
	if (Count <= 0)
	{
		Count = Interval;
		if (Target)
			Target->TimerWork();
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////
CTimerManager * CTimerManager::m_pTimerManager = NULL;

CTimerManager* GetTimerManager()
{
	return CTimerManager::GetTimerManager();
}

CTimerManager::CTimerManager(): TimerList(NULL), count(0), working(true)
{
//	pthread_mutex_init(&CS, NULL);
	if (pthread_create(&pthread, NULL, TimerTickThread, this) != 0)
		printf("pthread_create KeyReadThread error.\n");
}

CTimerManager * CTimerManager::GetTimerManager(void)
{
	if (! m_pTimerManager) {
		m_pTimerManager = new CTimerManager();
		atexit(DeleteTimerManager);
	}
	return m_pTimerManager;
}

void CTimerManager::DeleteTimerManager(void)
{
	if (m_pTimerManager)
		delete m_pTimerManager;
}

bool CTimerManager::StartTimer(CBaseObject* pTarget, uint8_t timerID, uint32_t dwInterval)
{
//	pthread_mutex_lock(&CS);
	for (int i=0; i<count;i++) {
		if (TimerList[i]->TimerID == timerID)
		{
			TimerList[i]->Count = dwInterval;
			TimerList[i]->Interval = dwInterval;
//			pthread_mutex_unlock(&CS);
			return true;
		}
	}

	TimerList = (CTimer **)realloc(TimerList, sizeof(CTimer*) * (count + 1));
	TimerList[count] = new CTimer(pTarget, timerID, dwInterval);
	count++;
//	pthread_mutex_unlock(&CS);
	return true;
}

bool CTimerManager::KillTimer(CBaseObject *pTarget, uint8_t timerID)
{
	bool ok = false;
//	pthread_mutex_lock(&CS);
	for (int i=0;i<count;i++)
		if ( (TimerList[i]->TimerID == timerID) && (TimerList[i]->Target == pTarget) ) {
			delete TimerList[i];
			count--;
			memcpy(TimerList+i, TimerList+i+1, sizeof(CTimer*) * (count - i));
			ok = true;
			break;
		}
//	pthread_mutex_unlock(&CS);
	return ok;
}

bool CTimerManager::KillAllTimer()
{
//	pthread_mutex_lock(&CS);
	for (int i=0; i<count;i++)
		delete TimerList[i];
	if (TimerList)
		free(TimerList);
//	pthread_mutex_unlock(&CS);
	return true;
}

static void Delay(uint32_t ms)
{
	int was_error;
	struct timespec elapsed, tv;

	elapsed.tv_sec = ms/1000;
	elapsed.tv_nsec = (ms%1000)*1000000;
	do {
		errno = 0;

		tv.tv_sec = elapsed.tv_sec;
		tv.tv_nsec = elapsed.tv_nsec;
		was_error = nanosleep(&tv, &elapsed);
	} while ( was_error && (errno == EINTR) );
}

void *TimerTickThread(void *p)
{
	CTimerManager *tm = (CTimerManager *)p;
	while (tm->working) {
//		pthread_mutex_lock(&tm->CS);
		for (int i=0; i<tm->count; i++)
			tm->TimerList[i]->Tick();
//		pthread_mutex_unlock(&tm->CS);
		Delay(100);
//		usleep(50 * 10000);
	}
	printf("Exit TimerTickThread.\n");
	return tm;
}
