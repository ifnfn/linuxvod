#ifndef KEYBUF_H
#define KEYBUF_H

#include <pthread.h>

#include "config.h"

typedef enum tagInputType{
	IT_KEY_DOWN,
	IT_KEY_UP,
	IT_MOUSELEFT_DOWN,
	IT_MOUSELEFT_UP,
	IT_MOUSERIGHT_DOWN,
	IT_MOUSERIGHT_UP,
	IT_MOUSEMIDDLE_DOWN,
	IT_MOUSEMIDDLE_UP,
	IT_MOUSEMOVE, 
	IT_QUIT
}InputType;

typedef struct tagInputEvent{
	InputType type;
	int k, x, y;
	CKtvOption *option;
} InputEvent;

class CKeyBuffer
{
public:
	static void FreeKeyBuf();
	static CKeyBuffer *CreateKeyBuf();
	bool KeyIn(InputEvent key);
	bool KeyOut(InputEvent *key);
	bool IsEmpty();
	bool IsFull();
protected:
	InputEvent keys[20];
	CKeyBuffer(void);
	~CKeyBuffer();
private:
	int MaxQueueLen;
	int Front;
	int Rear;
	pthread_mutex_t CS;
	sem_t EmptySemId;
	static CKeyBuffer *pKeyBuf;
	CString keylist;
};

#endif

