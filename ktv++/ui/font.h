#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "color.h"
#include "diskcache.h"

class CFontFile: public CListObject
{
public:
	CFontFile(const char *file, int size)
	{
		Size = size;
		File = file;
		LongFile = "";
		handle = NULL;
	}
	bool operator == (const CFontFile& font) const
	{
		if (&font == this) return true;
		return (font.File == File && font.Size == font.Size);
	}
	int Size;
	CString File;
	CString LongFile;
	void *handle;
};

class CKtvFont;
class CFontAlias;

class CXmlFont: public CListObject {
public:
	CXmlFont(const char* name, const char* charset, const char* file, int value) {
		FontName = name;
		Charset  = charset;
		FontFile = file;
		Value    = value;
	}
	CString FontName;
	CString Charset;
	CString FontFile;
	int Value;
};

class CFontFactory
{
public:
	static void DeleteFonts(void) {
		if (pFont) delete pFont;
	}
	static CFontFactory *CreateFontFactory();
	CKtvFont* CreateFont(const char *aliasname, const char *fontstr=NULL);
	CFontFile *FindOrCreateFont(const char *fontfile, int size);
	void AppendXmlFont(const char *name, const char* charset, const char* file, int value)
	{
		CXmlFont *font = new CXmlFont(name, charset, file, value);
		if (font)
			XmlFontList.Add(font);
	}
	CXmlFont *GetXmlFontByValue(int value);
	CXmlFont *GetXmlFontByCharset(const char *charset);
	bool DownloadFontFile(const char *fontfile);
	CDiskCache* DiskCache;
protected:
	CFontFactory(){
		DiskCache = CDiskCache::GetDiskCache(NULL, NULL);
	}
	~CFontFactory();
private:
	CKtvFont* CreateFont(int size, const char *charset, TColor color, const char *aliasname=NULL);
	static CFontFactory *pFont;

	CList<CFontFile>  FontList;
	CList<CFontAlias> AliasList;
	CList<CXmlFont>   XmlFontList;
};

class CKtvFont: public CListObject {
public:
	CKtvFont():font(NULL){}
	CKtvFont(CFontFactory* parent, TColor Color, CFontFile *Font)
	{
		fonts = parent;
		font  = Font;
		color = Color;
	}
	const int   size()     { if (font) return font->Size; else return 0; }
	const char *charset()  { return Charset.data();}
	const char *GetFontFile();
	void SetCharset (const char *charset) { Charset = charset;}
	void SetFont(int value, int size = -1);
	TColor color;

	void *GetHandle() { if(font) return font->handle; else return NULL; }
	void SetHandle(void *handle) { if (font) font->handle = handle; }
private:
	CFontFile *font;
	CString Charset;
	CFontFactory *fonts;
};


class CFontAlias: public CListObject{
public:
	CFontAlias():Font(NULL) {}
	CFontAlias(const char *aliasname, CKtvFont *font):Font(font) {AliasName = aliasname;}
	CString AliasName;
	CKtvFont *Font;
};

#endif

