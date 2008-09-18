#ifndef _DLL_H_
#define _DLL_H_

#include "headerdetection.h"

#if BUILDING_DLL
# define DLLIMPORT __declspec (dllexport)
#else /* Not BUILDING_DLL */
# define DLLIMPORT __declspec (dllimport)
#endif /* Not BUILDING_DLL */

DLLIMPORT DWORD getStreamType(char * filename, FileStreamType *streamType);

#endif /* _DLL_H_ */
