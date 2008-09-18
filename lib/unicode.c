#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef WIN32
	#include <windows.h>
#else
	#include <iconv.h>
#endif

#include "unicode.h"
#include "strext.h"

int Unicode(const char* charset, const char* inbuf, char *outbuf, int MaxLen)
{
	iconv_t cd;
	size_t InLen = strlen(inbuf);
	const char *OutBuf = outbuf;
	const char *InBuf  = inbuf;
	size_t OutByteSize = MaxLen;

	if( (cd = iconv_open("unicode", charset) ) == (iconv_t) -1){
		printf("Dont execute iconv_open function in DrawToSurface function.\n");
		return 0;
	}
	if( (iconv(cd, (char **)&InBuf, &InLen, (char **)&OutBuf, &OutByteSize)) == (size_t) -1){
		printf("Dont convert %s charset to unicode (%s)\n", charset, inbuf);
		return 0;
	}
	iconv_close(cd);
	OutByteSize = MaxLen - OutByteSize;
	return OutByteSize;
}

