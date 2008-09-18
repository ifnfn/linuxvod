#ifndef _MTVCONFIG_H_
#define _MTVCONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "key.h"
#include "tinyxml/tinyxml.h"
#include "avio/avio.h"
#include "utility.h"
#include "diskcache.h"
#include "font.h"

typedef struct soundmode {
	char modename[20];
	char *cmd;
} soundmode_t;

//==============================================================================
class CMtvConfig {
public:
	CMtvConfig(const char *root);
	CMtvConfig();
	~CMtvConfig();
	CString ChineseFont;           // �����ֿ��ļ�
	CString KoreanFont;            // �����ֿ��ļ�
	CString JapaneseFont;          // �����ֿ��ļ�
	char playerip[16];             // ������ IP
	int SongMaxNumber;             // �ѵ���������
	int PicThemeCount;             // ���Ի���������
	char **ThemeList;              // �����б�
	char **ThemePicList;           // �����б�
	bool testkey;                  // ���Լ�ֵ
	long idetime;                  // ����ʱ��
	CString defaulttheme;          // Ĭ�Ͻ�������
	bool haveserver;
	char urlroot[50];
	char songdata[50];
	char singerdata[50];
	void printconfig();
	char *GetSoundMode(const char *modename);
	void geturl(char *url, const char *path, const char *filename);
	void getremoteurl(char *url, const char *sub, const char *filename);
private:
	CDiskCache* DiskCache;
	char localroot[50];
	soundmode_t *soundmodes;
	int modecount;
	TiXmlDocument *doc;                 // �����õ� XML�ļ�
	TiXmlNode *confignode;              // <config>

	int ReadKeyDefine();                // �������ļ��ж�ȡ����ֵ
	int ReadThemeCount();               // �������ļ��ж�ȡ��������
	int ReadSoundMode();
	int ReadFontDefine();
	const char *ReadConfig(const char *conf);
	void GetPlayerIP();                 // ȡ������IP��ַa

	CFontFactory *fonts;
};
//==============================================================================
#endif
