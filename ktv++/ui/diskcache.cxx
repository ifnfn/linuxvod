#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "diskcache.h"
//##include "utils.h"
#include "utility.h"

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

CDiskCache* CDiskCache::DiskCache = NULL;

CString CDiskCache::RootLocal;
CString CDiskCache::RootNet;
bool    CDiskCache::HaveNet = false;
bool    CDiskCache::HaveLocal = false;

CUrlBuffer::CUrlBuffer(CDiskCache* cache)
{
	UrlIsOPen = false;
	fstream = NULL;
	purl = &urlstream;
	Position = 0;
	Size = 0;
	Memory = NULL;
	InMemory = false;
	DiskCache = cache;
	IsOpen = false;
}

bool CUrlBuffer::Open(const char* url, const char* local, bool memory, bool usefile)
{
	Url = url;
	Position = 0;
	SaveToFile = usefile;
	UrlIsOPen = (url_fopen(purl, url, URL_RDONLY) == 0);
	if (UrlIsOPen) {
		Size = url_fsize(purl);
	}
	if (DiskCache->HaveLocal) {
		if (SaveToFile)
			fstream = fopen(local, "ab+");
		else
			fstream = fopen(local, "rb");
		if (fstream != NULL) {
			fseek(fstream, 0, SEEK_END);
			LocalSize =ftell(fstream);
//			printf("LocalSize=%ld\n", LocalSize);
			fseek(fstream, 0, SEEK_SET);
		}
		else {
//			printf("fopen error.\n");
			LocalSize = 0;
		}
	}
	if ((!UrlIsOPen) && (fstream == NULL)) return false;
	Size = MAX(Size, LocalSize);
	if (memory) {
		Memory = (uint8_t*)realloc(Memory, Size+1);
		if (fstream)
			fread(Memory, LocalSize, 1, fstream);
	}
	else if (Memory) {
		free(Memory);
		Memory = NULL;
	}
	IsOpen = true;
	return true;
}


int CUrlBuffer::Read(unsigned char *buf, int size)
{
	int ReadMemSize = 0;

	if (Position < LocalSize) {
		ReadMemSize = MIN(size, LocalSize - Position);
//		printf("MIN(%d, %ld)=%d,Position=%ld\n", size, LocalSize - Position, ReadMemSize, Position);
		if (Memory) {
			if (ReadMemSize > 0)
				memcpy(buf, Memory+Position, ReadMemSize);
		}
		else if (fstream)
			ReadMemSize = fread(buf, 1, ReadMemSize, fstream);
	}
	int ReadUrlSize = MIN((size - ReadMemSize), Size);
	int ReadSize = 0;
	if ((ReadUrlSize > 0) && (Position + ReadMemSize < Size)&& UrlIsOPen) {
//		printf("ReadMemSize=%d, ReadSize=%d\n", ReadMemSize, ReadSize);
//		printf("%ld Need Read From Net Size %d\n", Size - Position - ReadMemSize, ReadUrlSize);
		ReadSize = url_fread(purl, buf + ReadMemSize, ReadUrlSize);
		if (Memory) {
//			printf("Read From Memory\n");
			memmove(Memory + Position, buf + ReadMemSize, ReadSize);
		}
		if (SaveToFile)
			fwrite(buf + ReadMemSize, ReadSize, 1, fstream);
	}
	Position += ReadSize + ReadMemSize;
//	printf("ReadMemSize=%d, ReadSize=%d\n", ReadMemSize, ReadSize);
	return ReadMemSize + ReadSize;
}

int CUrlBuffer::Close()
{
	DiskCache->RemoveChild(this);
	if (IsOpen==false) {
		return false;
	}
	if (Memory){
		free(Memory);
		Memory = NULL;
	}
	if (fstream)
		fclose(fstream);
	if (UrlIsOPen)
		url_fclose(purl);
	IsOpen = false;
	return true;
}

CUrlBuffer* CDiskCache::OpenFile(const char* filename, bool memory, bool savefile)
{
	CUrlBuffer* buffer = FindFile(filename);
	if (!buffer)
		buffer = new CUrlBuffer(this);
	if (!buffer) return NULL;
	char URL[512], LOCAL[512];
	pathcat(URL, RootNet.data(), filename);
	pathcat(LOCAL, RootLocal.data(), filename);

	buffer->Open(URL, LOCAL, memory, savefile);
	UrlBufferList.Add(buffer);
	return buffer;
}

CUrlBuffer* CDiskCache::FindFile(const char* filename)
{
	for (CUrlBuffer *buffer = UrlBufferList.First(); buffer; buffer = UrlBufferList.Next(buffer) )
	{
		if (strcasecmp(buffer->Url.data(), filename) == 0) {
			buffer->Seek(0, SEEK_SET);
			return buffer;
		}
	}
	return NULL;
}

void* CDiskCache::ReadBuffer(const char* filename, size_t& size, bool savefile)
{
	CUrlBuffer* buffer = OpenFile(filename, false, savefile);
	if (!buffer) return NULL;
	size= buffer->GetSize();
	uint8_t* p = (uint8_t*)malloc(size + 1);
	p[size] = 0;
	int pos = 0;
	while (1) {
		int len = buffer->Read(p + pos, 1023);
		if (len <=0)
			break;
		pos+=len;
	}
	delete buffer;
	return p;
}

bool CDiskCache::Download(const char* filename)
{
//	printf("%s(%s)\n", __FUNCTION__, filename);
	if (LocalExists(filename)) return true;
	CUrlBuffer* buffer = OpenFile(filename, false, true);
	if (!buffer) return false;
	uint8_t buf[1024];
	while (1) {
		int len = buffer->Read(buf, 1023);
		if (len <=0)
			break;
	}
	delete buffer;
	return true;
}

void CDiskCache::SetPath(const char* root, const char* localroot)
{
	if (root) {
		RootNet = root;
		HaveNet = RootNet.length();
	}
	if (localroot) {
		RootLocal = localroot;
		HaveLocal = RootLocal.length();
	}
}

