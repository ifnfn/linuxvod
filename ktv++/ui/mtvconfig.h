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
	CString ChineseFont;           // 中文字库文件
	CString KoreanFont;            // 韩文字库文件
	CString JapaneseFont;          // 日文字库文件
	char playerip[16];             // 播放器 IP
	int SongMaxNumber;             // 已点歌曲最大数
	int PicThemeCount;             // 可以换肤的数量
	char **ThemeList;              // 界面列表
	char **ThemePicList;           // 界面列表
	bool testkey;                  // 测试键值
	long idetime;                  // 空闲时间
	CString defaulttheme;          // 默认界面主题
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
	TiXmlDocument *doc;                 // 配置用的 XML文件
	TiXmlNode *confignode;              // <config>

	int ReadKeyDefine();                // 从配置文件中读取参数值
	int ReadThemeCount();               // 从配置文件中读取界面配置
	int ReadSoundMode();
	int ReadFontDefine();
	const char *ReadConfig(const char *conf);
	void GetPlayerIP();                 // 取播放器IP地址a

	CFontFactory *fonts;
};
//==============================================================================
#endif
