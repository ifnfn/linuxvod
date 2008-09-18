#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ftw.h>

#include "memtest.h"

#include "createindex.h"
#include "osnet.h"
#include "strext.h"

static CSongIndex songindex;
static CSingerIndex singerindex;

static int sqlitecallback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for(i=0; i<argc; i++)
		printf("%s", argv[i] ? argv[i] : "");
	printf("\n");
	return 0;
}

static int SongDataCallBack(void *NotUsed, int argc, char **argv, char **azColName)
{
	CKeywordIndex *index = (CKeywordIndex *)NotUsed;
	SystemFields data;
#if 0
	int i=0;
	for (i=0;i<argc; i++)
		printf("%s|", argv[i]);
		printf("(%d)%s|", i, azColName[i]);
	printf("\n");
	return 0;
#endif
	memset(&data, 0, sizeof(SystemFields));
	if (argv[ 0]) strncpy(data.SongCode   , argv[0], SongCodeLen   - 1);
	if (argv[ 1]) strncpy(data.SongName   , argv[1], SongNameLen   - 1);
	if (argv[ 2]) strncpy(data.Class      , argv[2], ClassLen      - 1);
	if (argv[ 3]) strncpy(data.Language   , argv[3], LanguageLen   - 1);
	if (argv[ 4]) strncpy(data.SingerName1, argv[4], SingerNameLen - 1);
	if (argv[ 5]) strncpy(data.SingerName2, argv[5], SingerNameLen - 1);
	if (argv[ 6]) strncpy(data.SingerName3, argv[6], SingerNameLen - 1);
	if (argv[ 7]) strncpy(data.SingerName4, argv[7], SingerNameLen - 1);
	if (argv[ 8]) strncpy(data.PinYin     , argv[8], PinYinLen     - 1);
	if (argv[ 9]) strncpy(data.WBH        , argv[9], PinYinLen     - 1);
	if (argv[10]) data.Charset  = atoi(argv[10]);
	if (argv[11]) data.VolumeK  = atoi(argv[11]);
	if (argv[12]) data.VolumeS  = atoi(argv[12]);
	if (argv[13]) data.Num      = atoi(argv[13]);
	if (argv[14]) data.Klok     = argv[14][0];
	if (argv[15]) data.Sound    = argv[15][0];
	if (argv[16]) data.SoundMode= atoi(argv[16]);
	if (argv[17]) strncpy(data.StreamType, argv[17], StreamTypeLen - 1);
	if (argv[18]) data.IsNewSong = atoi(argv[18]);

//	printf("data.charset=%d(%s)\n", data.Charset, argv[10]);
//	printf("data.Language%s(%s)\n", data.Language, argv[3]);
//	printf("data.Klok=%c(%s)\n", data.Klok, argv[14]);
//	printf("data.Sound=%c(%s)\n", data.Sound, argv[15]);
//	printf("StreamType (%s) (%s)\n", data.StreamType, argv[17]);
	index->AddSongData(&data);
	return 0;
}

static int HotSongDataCallBack(void *NotUsed, int argc, char **argv, char **azColName)
{
	CSongIndex *index = (CSongIndex *)NotUsed;
	SystemFields data;

	memset(&data, 0, sizeof(SystemFields));
	if (argv[0]) strncpy(data.SongCode, argv[0], SongCodeLen - 1);
	if (argv[1]) strncpy(data.PinYin, argv[1], PinYinLen   - 1);
	if (argv[2]) data.PlayNum = atoi(argv[2]);
	index->AddHotSongData(&data);
	return 0;
}

static int AddSingerCallBack(void *NotUsed, int argc, char **argv, char **azColName)
{
	CKeywordIndex *index = (CKeywordIndex *)NotUsed;
	if (argv[1]) index->AddIndexNode(argv[1], F_SINGER); else return 0;

	SystemFields data;
	memset(&data, 0, sizeof(SystemFields));
	if (argv[0]) strncpy(data.SongCode  , argv[0], SongCodeLen - 1);
	if (argv[1]) strncpy(data.SingerName, argv[1], SongNameLen - 1);
	if (argv[2]) strncpy(data.Sex       , argv[2], ClassLen - 1);
	if (argv[3]) strncpy(data.PinYin    , argv[3], PinYinLen - 1);
	singerindex.AddSongData(&data, false);
	return 0;
}

static int UpdateCallBack(void *NotUsed, int argc, char **argv, char **azColName)
{
	int *update = (int *) NotUsed;
	*update = atoi(argv[0]);
//	printf("%s=%s\n", azColName[0], argv[0]);
	return 0;
}

static char *zErrMsg = 0;

int IndexFromServer(sqlite *db)
{
	if (db ==NULL) return -1;

	URLContext *ucp;

	int udpfd = CreateStreamUdp(SERVERUDPPORT);
	if (udpfd < 0) return -1;

	char *buf=(char *)malloc(1024);
	int len,size=0;

	char url[512],host[20];
	while (1)
	{
		if(GetStreamServer(udpfd, host))
		{
			sprintf(url,"http://%s/cgi-bin/list", host);
			printf("%s Indexing...\n",host);
			url_open(&ucp, url, URL_RDONLY);
			if (ucp == NULL) {
				printf("open error.\n");
				continue;
			}
			while (1)
			{
				buf = (char *)realloc(buf, size+512);
				len = url_read(ucp, (unsigned char *)buf+size, 511);
				size += len;
				buf[size] = 0;
				if (len <=0) break;
			}
			url_close(ucp);
		} else
			break;
	}
	close(udpfd);

	sqlite_exec(db, "BEGIN TRANSACTION;", NULL, 0, NULL);
	sqlite_exec(db, "UPDATE system SET havesong=0;", NULL, 0, NULL);
	char sql[1024];
	char *code = strtok(buf, "\n");
	while (code)
	{
		sprintf(sql, "UPDATE system SET havesong=1 WHERE code='%s';", code);
		sqlite_exec(db, sql, NULL, 0, NULL);
		code = strtok(NULL, "\n");
	}

	sqlite_exec(db, "COMMIT;", NULL , 0, NULL);
	return 0;
}

static sqlite *localdb = NULL;
static long filenum =0;

static int fn(const char *file, const struct stat *sb, int flag)
{
	char buf[1024], code[20], path[200], extname[10];
	static long id = 0;
	if (id % 200 == 0)
		fprintf(stderr, ".");
	id++;
	if (flag == FTW_F)
	{
		ExtractFilePath(file, path);
		ExtractFileName(file, code);
		ExtractFileExt (file, extname);
		if ((strcasecmp(extname, "mpg") == 0) ||
			(strcasecmp(extname, "dat" ) == 0) ||
			(strcasecmp(extname, "m1s" ) == 0) ||
			(strcasecmp(extname, "vob" ) == 0) ||
			(strcasecmp(extname, "divx") == 0) ||
			(strcasecmp(extname, "avi" ) == 0) ||
			(strcasecmp(extname, "div" ) == 0) )
		{
			filenum++;
			sprintf(buf, "UPDATE system SET havesong=1,filesize=%ld WHERE code='%s';", sb->st_size, code);
			sqlite_exec(localdb, buf, NULL, 0, NULL);
		}
	}
	return 0;
}

int IndexLocal(sqlite *db, const char *indexpath)
{
	sqlite_exec(db, "BEGIN TRANSACTION;", NULL , 0, NULL);
	sqlite_exec(db, "UPDATE system SET havesong=0;", NULL , 0, NULL);
	localdb = db;
	filenum =0;
	ftw(indexpath, fn, 500);
	sqlite_exec(db, "COMMIT;", NULL , 0, NULL);
	printf("\n\nFound Song Number: %ld\n", filenum);
	return filenum;
}

int main(int argc, char **argv)
{
	av_register_all();

	sqlite *db = NULL;

	char filename[512] = "/ktvdata/song2000.db";
	char sql[512] = "";
	char indexpath[512] ="/tmp/video";

	char singerfile[200]="/ktvdata/singerdata", songfile[200]="/ktvdata/songdata";
	int fromnet = 2;
	int print = 0;
	int update = 0;
	char sqlfile[512]="";
	int setupdate=0;
	char ch;
	while ((ch = getopt(argc, argv, "q:f:s:a:b:d:hnlpu"))!= -1)
	{
		switch (ch)
		{
			case 'u':
				update = 1;
				break;
			case 'q':
				strcpy(sqlfile, optarg);
				break;
			case 'f':
				strcpy(filename, optarg);
				break;
			case 's':
				strcpy(sql, optarg);
				break;
			case 'a':
				strcpy(songfile, optarg);
				break;
			case 'b':
				strcpy(singerfile, optarg);
				break;
			case 'd':
				strcpy(indexpath, optarg);
				break;
			case 'n':
				fromnet = 1;
				break;
			case 'l':
				fromnet = 0;
				break;
			case 'p':
				print = 1;
				break;
			case 'm':
				setupdate=1;
				break;
			case 'h':
				printf("Usage: %s [-u] [-q <sqlfile>] [-f <songfile>] [-s <sql command>] [-a <songdatafile>] [-b <singerdatafile>] [-d <video>] [-l/-n] [-h] [-m]\n", argv[0]);
				exit(0);
		}
	}

	db = sqlite_open(filename, 0, &zErrMsg);
	if (db == NULL)
	{
		printf("error: %s\n", zErrMsg);
		return -1;
	}

	if (setupdate) {
		sqlite_exec(db, "UPDATE UpdateIndex SET IndexTag=1;", NULL, NULL, NULL);
		exit(1);
	}

	if (strcasecmp(sql, ""))
	{
		sqlite_exec(db, sql, sqlitecallback, 0, &zErrMsg);
		sqlite_exec(db, "UPDATE UpdateIndex SET IndexTag=1;", NULL, NULL, NULL);
		return 1;
	}
	if (strcasecmp(sqlfile, ""))
	{
		FILE* fp = fopen(sqlfile, "r");
		if (fp == NULL) return -1;
		char sql[1024];
		while (!feof(fp))
		{
			if ( fgets(sql, 1024, fp) != NULL)
				sqlite_exec(db, sql, NULL, 0, NULL);
		}
		fclose(fp);
		sqlite_exec(db, "UPDATE UpdateIndex SET IndexTag=1;", NULL, NULL, NULL);
		return 2;
	}

	if (!update) {
		sqlite_exec(db, "SELECT IndexTag FROM UpdateIndex;", UpdateCallBack, &update, NULL);
		sqlite_exec(db, "UPDATE UpdateIndex SET IndexTag=0;", NULL, NULL, NULL);
	}

	if (update)
	{
		if (fromnet)
			IndexFromServer(db);
		else
			IndexLocal(db, indexpath);
		char *sql_1 = "SELECT id,name,sex,pinyin FROM singer WHERE name in (\
				SELECT singer1 FROM system WHERE havesong=1) AND visible=1 ORDER by num DESC;";
		sqlite_exec(db, sql_1, AddSingerCallBack, &songindex, &zErrMsg);

		char *sql_2 = "SELECT code,name,class,language,singer1,singer2,singer3,singer4,\
				pinyin,wbh,charset,volumek,volumes,num,klok,sound,soundmode,\
				videotype, isnewsong FROM system WHERE havesong=1 ORDER BY Num, PinYin, Name;";

		sqlite_exec(db, sql_2, SongDataCallBack, &songindex, &zErrMsg);
		songindex.CodeHashSort();

		char *sql_3 = "SELECT code, PinYin, PlayNum FROM system WHERE havesong=1 ORDER BY playnum DESC LIMIT 1000";
		sqlite_exec(db, sql_3, HotSongDataCallBack, &songindex, &zErrMsg);
		sqlite_close(db);
		songindex.SaveFile(songfile);
		singerindex.SaveFile(singerfile);
		if (print)
		{
			songindex.printIndex();
			singerindex.printIndex();
		}
	}
	return 0;
}
