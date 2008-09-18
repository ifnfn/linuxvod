#ifndef OSDEF_H
#define OSDEF_H
typedef unsigned char   BYTE;
typedef unsigned long   DWORD;
typedef unsigned short  WORD;
typedef long		LONG;
typedef void		*PVOID;

typedef int PORTHANDLE,HRESULT;

// 8bit types
typedef char *PCHAR,*LPCTSTR;
typedef unsigned char *PBYTE;
typedef char CHAR;

// 16bit types
typedef unsigned short USHORT;

// 32bit types
typedef unsigned long ULONG;
typedef unsigned long UINT;
typedef long INT;

// 64bit types
// #ifdef _VOD_
// typedef unsigned long long ULONGLONG;
// #endif

// pointer types
typedef void *LPVOID;
typedef int BOOL;

#ifndef TRUE
#define TRUE 1
#endif // TRUE

#ifndef FALSE
#define FALSE 0
#endif // FALSE

#ifdef _UNICODE_
typedef unsigned short TCHAR;
#else
typedef char TCHAR;
#endif // _UNICODE_

#ifdef _BARBADOS_
typedef unsigned long long ULONGLONG;
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#endif
