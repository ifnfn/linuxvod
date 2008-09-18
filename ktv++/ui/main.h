#ifndef MAIN_H
#define MAIN_H

#include "application.h"

class CKtvApplication: public CApplication 
{
public:
	CKtvApplication(int argc, char **argv): CApplication(argc, argv) {}
	virtual bool GlobalInputProcess(int KtvKeyValue);
private:
	bool QuickKeyWindow(const char *winname, int stacknum);
};
#endif
