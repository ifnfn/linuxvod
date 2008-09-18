#include "memtest.h"

#include "font.h"
#include "strext.h"
#include "config.h"

CFontFactory *CFontFactory::pFont  = NULL;

const char *CKtvFont::GetFontFile()
{
	if (font){
		if (font->LongFile == "") {
			char fonturl[512];
			fonts->DiskCache->Download(font->File.data());
			if (fonts->DiskCache->GetUrl(font->File.data(), fonturl))
				font->LongFile = fonturl;
//			printf("fontfile = %s\n", font->LongFile.data());
		}
		return font->LongFile.data();
	}
	else
		return NULL;
}

void CKtvFont::SetFont(int value, int size)
{
	if (size == -1)
		size = font->Size;
	CXmlFont *xmlfont = fonts->GetXmlFontByValue(value);
	const char *fontfile = NULL;
	if (xmlfont) {
		fontfile = xmlfont->FontFile.data();
		SetCharset(xmlfont->Charset.data());
	}
	if (fontfile) {
//		printf("%s(%d,%d): fontfile=%s\n", __FUNCTION__, value, size, fontfile);
		CFontFile *tmp = fonts->FindOrCreateFont(fontfile, size);
		if (tmp)
			font = tmp;
	}
}

CFontFactory::~CFontFactory()
{
}

CFontFactory *CFontFactory::CreateFontFactory()
{
	if (!pFont) {
		pFont = new CFontFactory();
		atexit(CFontFactory::DeleteFonts);
	}
	return pFont;
}

CKtvFont* CFontFactory::CreateFont(int size, const char *charset, TColor color, const char *aliasname)
{
	const char *fontfile = NULL;
	CXmlFont *xmlfont = GetXmlFontByCharset(charset);
	if (xmlfont)
		fontfile = xmlfont->FontFile.data();
	if (!fontfile)
	{
//		printf("CreateFont %s(%s)[%d] Error\n", fontfile, aliasname, size);
		return NULL;
	}

	CKtvFont* basefont = NULL;
	CFontFile *tmpfont = FindOrCreateFont(fontfile, size);
	if (tmpfont) {
		basefont = new CKtvFont(this, color, tmpfont);
		basefont->SetCharset(charset);
		if (aliasname)
			AliasList.Add(new CFontAlias(aliasname, basefont));
	}
	return basefont;
}

static bool StrToFont(const char *buf, char *name, int& size, TColor& color, char *charset)
{
	int i=0;
	char *tmpbuf = strdup(buf);
	char *tmp = strtok(tmpbuf, ",");
	while (tmp)
	{
		switch(i)
		{
			case 0:
				strcpy(name, tmp);
				break;
			case 1:
				size = atoi(tmp);
				break;
			case 2:
				color = StrToColor(tmp);
				break;
			case 3:
				strcpy(charset, tmp);
				break;
		};
		i++;
		tmp = strtok(NULL, ",");
	}
	free(tmpbuf);
	return true;
}

CKtvFont* CFontFactory::CreateFont(const char *aliasname, const char *fontstr)
{
	if (aliasname == NULL && fontstr == NULL)
		return NULL;

	if (aliasname)
	{
		for ( CFontAlias *p = AliasList.First(); p; p = AliasList.Next(p) )
			if (p->AliasName == aliasname)
				if (p->Font) return p->Font;
	}

	if (fontstr)
	{
		char name[100], charset[100];
		int size = 0;
		TColor color={0,0,0,0};
		if (StrToFont(fontstr, name, size, color, charset))
			return CreateFont(size, charset, color, aliasname);
	}
	return NULL;
}

CFontFile *CFontFactory::FindOrCreateFont(const char *name, int size)
{
	for ( CFontFile *p = FontList.First(); p; p = FontList.Next(p) )
		if (p->Size == size && p->File == name)
			return p;
	CFontFile *tmpfont = new CFontFile(name, size);
	FontList.Add(tmpfont);
	return tmpfont;
}

CXmlFont *CFontFactory::GetXmlFontByValue(int value)
{
	CXmlFont *DefaultFont = XmlFontList.First();
//	printf("DefaultFont->FontFile=%s\n", DefaultFont->FontFile.data());
	for (CXmlFont *p = DefaultFont; p; p = XmlFontList.Next(p)){
//		printf("p->FontFile=%s\n", p->FontFile.data());
		if (p->Value == value)
			return p;
	}
	return DefaultFont;
}

CXmlFont *CFontFactory::GetXmlFontByCharset(const char *charset)
{
	CXmlFont *DefaultFont = XmlFontList.First();
	for (CXmlFont *p = DefaultFont; p; p = XmlFontList.Next(p)){
		if (p->Charset == charset)
			return p;
	}
	return DefaultFont;
}

