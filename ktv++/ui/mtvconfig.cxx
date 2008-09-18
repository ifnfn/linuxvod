#include <stdio.h>
#include <stdlib.h>

#include "memtest.h"
#include "player.h"
#include "config.h"

CMtvConfig::CMtvConfig(const char *root): \
	SongMaxNumber(50),                  // 已点歌曲最大数
	PicThemeCount(0),                   // 可以换肤的数量
	ThemeList(NULL),                    // 界面列表
	ThemePicList(NULL),                 // 界面列表
	testkey(false),                     // 测试键值
	idetime(0),                         // 空闲时间
	haveserver(false),
	soundmodes(NULL),
	modecount(0)
{
	av_register_all();
	const char *keyname = NULL;

	if (strlen(root) > 0)
		sprintf(urlroot, "http://%s/", root);
	else
		urlroot[0] = 0;
	strcpy(localroot, DATAPATH);
	DiskCache = CDiskCache::GetDiskCache(urlroot, localroot);

	size_t size = 0;
	char *data = (char*) DiskCache->ReadBuffer("/mtv.xml", size);
	if (!data) {
		printf("Download mtv.xml error\n");
		exit(1);
	}
	doc = new TiXmlDocument();
	doc->Parse((char *)data);
	delete data;
	fonts = CFontFactory::CreateFontFactory();
	confignode = doc->RootElement();

	if ((keyname = ReadConfig("ChineseFont")))
		ChineseFont = keyname;
	else
		ChineseFont = DEFAULT_FONT;    // 中文字库
	if ((keyname = ReadConfig("KoreanFont")))
		KoreanFont = keyname;
	else
		KoreanFont = DEFAULT_FONT;    // 中文字库
	if ((keyname = ReadConfig("JapaneseFont")))
		JapaneseFont = keyname;
	else
		JapaneseFont = DEFAULT_FONT;    // 中文字库

	if ((keyname = ReadConfig("SongMaxNumber"))) // 已点歌曲最大数
		SongMaxNumber = atoi(keyname);
	else
		SongMaxNumber = MAX_SONG_NUMBER;
	if ((keyname = ReadConfig("testkey")))       // 测试遥控器键值
		testkey = atoi(keyname);

	if ((keyname = ReadConfig("idetimesecond"))) // 空闲时间
		idetime = atoi(keyname);

	ReadKeyDefine();
//	PrintKeyMaps();
	GetPlayerIP();
	ReadThemeCount();
	ReadFontDefine();

	DiskCache->GetUrl("/songdata", songdata);
	DiskCache->GetUrl("/singerdata", singerdata);
//	ReadSoundMode();
	delete doc;
}

CMtvConfig::~CMtvConfig()
{
	if (ThemeList)
	{
		for (int i=0;i<PicThemeCount;i++){
			if (ThemeList[i])
				free(ThemeList[i]);
			if (ThemePicList[i])
				free(ThemePicList[i]);
		}
		free(ThemeList);
	}
	if (ThemePicList)
		free(ThemePicList);
}

#ifdef DEBUG
void CMtvConfig::printconfig()
{
	DEBUG_OUT("ChineseFont  = %s\n", ChineseFont.data());
	DEBUG_OUT("PlayerIP     = %s\n", playerip);
	DEBUG_OUT("SongMaxNumber= %d\n", SongMaxNumber);
	DEBUG_OUT("ThemeCount   = %d\n", PicThemeCount);
}
#endif

char *CMtvConfig::GetSoundMode(const char *modename)
{
	for (int i=0; i<modecount;i++)
		if (strcasecmp(modename, soundmodes[i].modename) == 0)
			return soundmodes[i].cmd;
	return NULL;
}

int CMtvConfig::ReadSoundMode()
{
	TiXmlNode *node = doc->RootElement();
	modecount =0;
	if (!node) return  0;
	node = node->FirstChildElement("soundmode");
	if (!node) return  0;
	const char *modename = NULL;
	TiXmlElement *mode = node->FirstChildElement("mode");
	while (mode)
	{
		modename = mode->Attribute("name");
		if (modename)
		{
			soundmodes = (soundmode_t *)realloc(soundmodes, (modecount+1)*sizeof(soundmode_t));
			strncpy(soundmodes[modecount].modename, modename, 20);
			soundmodes[modecount++].cmd = strdup (mode->FirstChild()->ToText()->Value());
			DEBUG_OUT("mode=%s, cmd=%s\n", modename, soundmodes[modecount-1].cmd);
		}
		mode = mode->NextSiblingElement();
	}
	return modecount;
}

int CMtvConfig::ReadKeyDefine()
{
	TiXmlNode *node = doc->RootElement();
	int count = 0;
	if (node)
	{
		node = node->FirstChildElement("keys");
		if (node)
		{
			const char *keyname = NULL;
			TiXmlElement *KeyNode = node->FirstChildElement("key");
			while (KeyNode)
			{
				keyname = KeyNode->Attribute("name");
				if (keyname)
				{
					for (int i=0;i<MAX_KEY_STRUCT;i++) {
						if (strcasecmp(keyname, KeyMaps[i].name) == 0)
						{
							KeyMaps[i].sys_value = atoi(KeyNode->Attribute("use"));
							strncpy(KeyMaps[i].title, KeyNode->Attribute("title"), sizeof(KeyMaps[i].title)-1);
							count++;
							break;
						}
					}
				}
				KeyNode = KeyNode->NextSiblingElement();
			}
		}
	}
	return count;
}

int CMtvConfig::ReadFontDefine()
{
	TiXmlNode *node = doc->RootElement();
	int count = 0;
	if (node)
	{
		node = node->FirstChildElement("fonts");
		if (node)
		{
			const char *keyname = NULL;
			const char *charset = NULL;
			const char *filename= NULL;
			int value = 0;
			TiXmlElement *element= node->FirstChildElement();
			while (element)
			{
				keyname  = element->Attribute("name");
				charset  = element->Attribute("charset");
				filename = element->Attribute("filename");
				element->QueryIntAttribute("value", &value);
//				printf("name: %s\tcharset: %s\tfilename: %s\tvalue=%d\n", keyname, charset, filename, value);
				if (strlen(keyname) && strlen(charset) && strlen(filename)) {
					fonts->AppendXmlFont(keyname, charset, filename, value);
					count++;
				}
				element= element->NextSiblingElement();
			}
		}
	}
	return count;
}

int CMtvConfig::ReadThemeCount()
{
	PicThemeCount = 0;
	if (confignode)
	{
		const char *themename = NULL;
		TiXmlElement *KeyNode = confignode->FirstChildElement("interface");
		if (KeyNode)
		{
			const char *p = KeyNode->Attribute("default");
			if (p)
				defaulttheme = p;
			else
				defaulttheme = "theme.pug";
			KeyNode = KeyNode->FirstChildElement("plungin");
			while (KeyNode)
			{
				themename = KeyNode->Attribute("name");
				if (themename)
				{
					ThemeList = (char **)realloc(ThemeList, (PicThemeCount + 1) * sizeof(char *));
					ThemeList[PicThemeCount] = strdup(themename);
					ThemePicList = (char **)realloc(ThemePicList, (PicThemeCount + 1) * sizeof(char *));
					ThemePicList[PicThemeCount] = strdup(KeyNode->Attribute("mainpic"));
					PicThemeCount++;
				}
				KeyNode = KeyNode->NextSiblingElement();
			}
		}
	}
	return PicThemeCount;
}

const char *CMtvConfig::ReadConfig(const char *conf)
{
	if (confignode)
	{
		TiXmlElement *KeyNode = confignode->FirstChildElement(conf);
		if (KeyNode)
			return KeyNode->Attribute("use");
		else
			return NULL;
	} else
		return NULL;
}

void CMtvConfig::GetPlayerIP()
{
	strcpy(playerip, "127.0.0.1");
	if (confignode)
	{
		const char *keyname = NULL;
		TiXmlElement *KeyNode = confignode->FirstChildElement("playergroup");

		if (KeyNode)
		{
			char localip[16];
			GetLocalIP(localip);
			KeyNode = KeyNode->FirstChildElement("ip");
			while (KeyNode)
			{
				keyname = KeyNode->Attribute("name");
				if (keyname) {
					if (strcmp(keyname, localip) == 0) {
						strcpy(playerip, KeyNode->Attribute("player"));
						break;
					}
				}
				KeyNode = KeyNode->NextSiblingElement();
			}
		}
	}
}

void CMtvConfig::geturl(char *url, const char *path, const char *filename)
{
	char Path[512];
	pathcat(Path, path, filename);
	DiskCache->GetUrl(Path, url);
}

void CMtvConfig::getremoteurl(char *url, const char *path, const char *filename)
{
	pathcat(url, urlroot, path);
	pathcat(url, url, filename);
}
