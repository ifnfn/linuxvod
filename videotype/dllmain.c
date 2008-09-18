/* Replace "dll.h" with the name of your header */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "dll.h"

DLLIMPORT DWORD getStreamType(char * filename, FileStreamType *streamType)
{
  if (streamType == NULL) {
    FileStreamType mm;
    GetFileProperties(filename, &mm);
    return mm.fileType;
  } 
  else {
    GetFileProperties(filename, streamType);
    return streamType->fileType;
  }
}

BOOL APIENTRY DllMain (HINSTANCE hInst     /* Library instance handle. */ ,
                       DWORD reason        /* Reason this function is being called. */ ,
                       LPVOID reserved     /* Not used. */ )
{
    switch (reason)
    {
      case DLL_PROCESS_ATTACH:
        break;

      case DLL_PROCESS_DETACH:
        break;

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }

    /* Returns TRUE on success, FALSE on failure */
    return TRUE;
}
