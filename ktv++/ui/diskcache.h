#ifndef DISKCACHE
#define DISKCACHE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "avio/avio.h"
#include "string.h"
#include "utility.h"
#include "list.h"
#include "strext.h"

class CDiskCache;

class CUrlBuffer: public CListObject {
public:
	CString Url;
	CUrlBuffer(CDiskCache* cache);
	virtual ~CUrlBuffer() {
		Close();
	}
	bool Open(const char* url, const char* local, bool memory, bool usefile);
	int Read(unsigned char *buf, int size);
	long GetSize() { return Size; }
	int Seek(offset_t pos, int whence) {
		if (whence == SEEK_SET)
			Position = 0;
		else if (whence == SEEK_END)
			;
		else if (whence == SEEK_CUR)
			Position+=pos;
		return 0;
	}
	int Close();
private:
	bool IsOpen;
	FILE* fstream;
	ByteIOContext urlstream;
	ByteIOContext *purl;
	long Position;
	long Size;
	uint8_t* Memory;
	bool InMemory;
	bool SaveToFile;
	CDiskCache* DiskCache;
	long LocalSize;
	bool UrlIsOPen;
};

class CDiskCache {
public:
	~CDiskCache() {}
	friend class CUrlBuffer;
	static CDiskCache* GetDiskCache(const char* root, const char* localroot) {
		if (!DiskCache) {
			DiskCache = new CDiskCache(root, localroot);
		}
		SetPath(root, localroot);
		return DiskCache;
	}
	static void SetPath(const char* root, const char* localroot);
	CUrlBuffer* OpenFile(const char* filename, bool memory, bool savefile);
	void* ReadBuffer(char* filename, size_t& size, bool savefile=false);

	bool Download(const char* filename);
	void RemoveChild(CUrlBuffer* pChild) {
		UrlBufferList.Remove(pChild, false);
	}
	char* GetUrl(const char* filename, char* url)   {
		pathcat(url, RootLocal.data(), filename);
		if (FileExists(url))
			return url;

		pathcat(url, RootNet.data(), filename);
		if (url_exist(url))
			return url;
		return NULL;
	}
	bool LocalExists(const char* filename) {
		char Buffer[512];
		pathcat(Buffer, RootLocal.data(), filename);
		return FileExists(Buffer);
	}
protected:
	CDiskCache(const char*root, const char* localroot) {
		av_register_all();
		SetPath(root, localroot);
	}
	static bool HaveNet;
	static bool HaveLocal;
private:
	CUrlBuffer* FindFile(const char* filename);
	static CString RootLocal;
	static CString RootNet;

	CList<CUrlBuffer> UrlBufferList;
	static CDiskCache* DiskCache;
};

#endif

