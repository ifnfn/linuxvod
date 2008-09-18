#include <stdio.h>
#include <stdlib.h>

#include "memtest.h"
#include "keybuf.h"

CKeyBuffer *CKeyBuffer::pKeyBuf = NULL;
CKeyBuffer::CKeyBuffer(void): MaxQueueLen(4),Front(0),Rear(0)
{
	if (sem_init(&EmptySemId, 0, 0) < 0)
		perror("semget EmptySemId error.\n");
	pthread_mutex_init(&CS, NULL);
}

CKeyBuffer::~CKeyBuffer()
{
	sem_post(&EmptySemId);
	sem_destroy(&EmptySemId);
	pthread_mutex_destroy(&CS);
}

void CKeyBuffer::FreeKeyBuf()
{
	if (pKeyBuf) delete pKeyBuf;
}

CKeyBuffer *CKeyBuffer::CreateKeyBuf()
{
	if (!pKeyBuf) {
		pKeyBuf = new CKeyBuffer();
		atexit(CKeyBuffer::FreeKeyBuf);
	}
	return pKeyBuf;
}

bool CKeyBuffer::KeyIn(InputEvent key)
{
	if (!IsFull()) {
		pthread_mutex_lock(&CS);
		if (key.type == IT_KEY_DOWN) {
			if (keylist.length() >= 4)
				keylist.assign(keylist.data() + 1, keylist.length()-1);
			if (key.k < 255) {
				if (isalpha(key.k)) {
					keylist += key.k;
//					printf("keylist=%s\n",keylist.data());
					if (keylist == "exit")
						exit(0);
				}
			}
		}
		Rear = (Rear + 1) % MaxQueueLen;
		keys[Rear] = key;
		pthread_mutex_unlock(&CS);
		sem_post(&EmptySemId);
		return true;
	}
	return false;
}

bool CKeyBuffer::KeyOut(InputEvent *key)
{
	sem_wait(&EmptySemId);
	pthread_mutex_lock(&CS);
	if (Front == Rear) {
		pthread_mutex_unlock(&CS);
		return 0;
	}
	Front = (Front+1) % MaxQueueLen;
	*key = keys[Front];
	pthread_mutex_unlock(&CS);
	return true;
}

bool CKeyBuffer::IsEmpty()
{
	pthread_mutex_lock(&CS);
	bool k = (Front == Rear);
	pthread_mutex_unlock(&CS);
	return k;
}

bool CKeyBuffer::IsFull()
{
	pthread_mutex_lock(&CS);
	bool k = ((Rear + 1) % MaxQueueLen == Front);
	pthread_mutex_unlock(&CS);
	return k;
}

