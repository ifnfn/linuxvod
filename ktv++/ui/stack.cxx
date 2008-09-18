#include <stdio.h>
#include <stdlib.h>

#include "memtest.h"
#include "windowmanage.h"

CWindowStack *CWindowStack::pWinStack = NULL;

CWindowStack::~CWindowStack(void)
{
}

CBaseWindow *CWindowStack::WindowPop()
{
	tos--;
	if (tos < 0) {
		DEBUG_OUT("Stack Underflow.\n");
		return NULL;
	}
	return stack[tos];
}

CBaseWindow *CWindowStack::WindowTop()
{
	if (tos > 0) {
		return stack[tos-1];
	}
	else
		return NULL;
}

void CWindowStack::WindowPush(CBaseWindow *window)
{
	if (tos < maxnum)
	{
		stack[tos] = window;
		tos++;
	}
}

void CWindowStack::StackClean(int num)
{
	if (num < tos)
		tos = num;
}

CWindowStack *CWindowStack::CreateStack()
{
	if (!pWinStack) {
		pWinStack = new CWindowStack();
		atexit(CWindowStack::FreeStack);
	}
	return pWinStack;
}

void CWindowStack::FreeStack()
{
	if (pWinStack) delete pWinStack;
}

