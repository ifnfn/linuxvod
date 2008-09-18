#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ustat.h>

#include "sqlite.h"
#include "osnet.h"
#include "ini.h"
#include "strext.h"

int CopyFile(const char *, const char *);
static int UpdateCallBack(void *, int, char **, char **);

typedef long long offset_t;
typedef struct DistPath {
	offset_t freesize;
	char path[512];
	struct DistPath *next;
}DistPath;

static DistPath *DistPathList = NULL;

static int CreateDistPathList(const char *filename)
{
	FILE *fp = fopen(filename, "r+");
	int count = 0;
	if (fp)
	{
		char buf[1024];
		while (!feof(fp))
		{
			if (fgets(buf, 1023, fp) && buf[0] ) {
				DistPath *tmp = (DistPath*) malloc( sizeof(DistPath) );
				if (tmp == NULL) return -1;
				sscanf(buf, "%s %lld",  tmp->path, &tmp->freesize);
				tmp->freesize *= 1024;
				count++;
				tmp->next = DistPathList;
				DistPathList = tmp;
			}
		}
		fclose(fp);
#if 0
		DistPath *head = DistPathList;
		printf("\n\nStart: %d\n", count);
		while (head)
		{
			printf("asdf:%s %lld\n", head->path, head->freesize);
			head = head->next;
		}
#endif
		return count;
	}
	return -1;
}

const char *GetMaxSizeDisk(const char *copyfile, const char *distfile)
{
	struct stat stfile;
	static char dist[1024] = "";
	memset(&stfile, 0, sizeof(struct stat));
	if (stat(copyfile, &stfile)==-1) return NULL;

	offset_t needsize = stfile.st_size *2;

	DistPath *head = DistPathList, *work;
	offset_t MaxFreeSizePath = 0;
	while (head)
	{
		if (head->freesize > MaxFreeSizePath) {
			MaxFreeSizePath = head->freesize;
			work = head;
		}
		head = head->next;
	}

	if ( (needsize > MaxFreeSizePath) || ( work == NULL ) )
		return NULL;
	else {
		work->freesize -= stfile.st_size;
		strcpy(dist, work->path);
		strcat(dist, "/");
		strcat(dist, distfile);
		return dist;
	}
}

static char *ReadStrSection(struct ENTRY *pSection, char *pKey, char *value)
{
	struct ENTRY *key = FindpKey(pSection, pKey);
	if (key) {
		strcpy (value, key->Node.ValText);
		return value;
	}
	strcpy(value, "");
	return NULL;
}

static int GetMaxSongCode(sqlite *db)
{
	int maxcode = -1;
	sqlite_exec(db, "SELECT MAX(code) from system", UpdateCallBack, &maxcode, NULL);
	return maxcode;
}

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
		if ((strcasecmp(extname, "jpg") == 0) ||
			(strcasecmp(extname, "jpeg" ) == 0) )
		{
			char Dist[100] = "/ktvdata/photos/";
			strcat(Dist, code);
			strcat(Dist, ".jpg");
			CopyFile(file, Dist);
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if(argc != 4)
	{
		printf("Usage:addsong <SourcePath> <DataBase> <DiskSizeFile>\n", argv[0]);
		return -1;
	}
	char SourcePath[1024];
	sprintf(SourcePath, "%s/install.ini", argv[1]);
	struct ENTRY *headini = OpenIniFile(SourcePath);

	CreateDistPathList(argv[3]);

	const char * database = argv[2];
	sqlite *db = NULL;
	char *zErrMsg;
	db = sqlite_open(database, 0, &zErrMsg);
	if(db == NULL)
	{
		printf("Error: %s\n", zErrMsg);
		return -1;
	}
	ftw(SourcePath, fn, 500);
	int maxcode = GetMaxSongCode(db);
	if (maxcode = -1) return -1;
	printf("maxcode = %d\n", maxcode);
	char code     [20];
	char name     [50];
	char charset  [10];
	char language [50];
	char classes  [50];
	char singer1  [50];
	char singer2  [50];
	char singer3  [50];
	char singer4  [50];
	char num      [50];
	char pinyin   [50];
	char wbh      [50];
	char playnum  [10];
	char klok     [10];
	char sound    [10];
	char filesize [10];
	char volumek  [10];
	char volumes  [10];
	char videotype[10];
	char filename [99];
	char isnewsong[10];

	struct ENTRY *pEntry = headini->pChild;
	while (pEntry != NULL){
		if (pEntry->Node.Type == tpSECTION){
			strcpy(code, pEntry->Node.KeyText);
			ReadStrSection(pEntry, "code"     , code     );
			if (strlen(code) == 0)
				sprintf(code, "%d", ++maxcode);
			ReadStrSection(pEntry, "name"     , name     );
			ReadStrSection(pEntry, "charset"  , charset  );
			ReadStrSection(pEntry, "language" , language );
			ReadStrSection(pEntry, "class"    , classes  );
			ReadStrSection(pEntry, "singer1"  , singer1  );
			ReadStrSection(pEntry, "singer2"  , singer2  );
			ReadStrSection(pEntry, "singer3"  , singer3  );
			ReadStrSection(pEntry, "singer4"  , singer4  );
			ReadStrSection(pEntry, "num"      , num      );
			ReadStrSection(pEntry, "pinyin"   , pinyin   );
			ReadStrSection(pEntry, "wbh"      , wbh      );
			ReadStrSection(pEntry, "playnum"  , playnum  );
			ReadStrSection(pEntry, "klok"     , klok     );
			ReadStrSection(pEntry, "sound"    , sound    );
			ReadStrSection(pEntry, "filesize" , filesize );
			ReadStrSection(pEntry, "volumek"  , volumek  );
			ReadStrSection(pEntry, "volumes"  , volumes  );
			ReadStrSection(pEntry, "videotype", videotype);
			ReadStrSection(pEntry, "filename" , filename );
			ReadStrSection(pEntry, "isnewsong", isnewsong);
			char sendmsg[1024];
			sprintf(sendmsg, "MsgBox=�Ӹ裺%s,0", name);
			printf("%s\n", sendmsg);
			UdpSendStr("*", 31016, sendmsg);

			char sql[512];
			sprintf(sql, "SELECT COUNT(*) FROM system WHERE code='%s';", code);
			int update = 0;
			sqlite_exec(db, sql, UpdateCallBack, &update, NULL);
			if (update == 1)
			{
				sprintf(sql, "UPDATE System SET Name='%s', Charset='%s', \
Language='%s', Class='%s', Singer1='%s', Singer2='%s', Singer3='%s', \
Singer4='%s', Num='%s', Pinyin='%s', WBH='%s', PlayNum='%s', Klok='%s', \
Sound='%s', FileSize='%s', isNewSong='%s', VolumeK='%s', \
VolumeS='%s', VideoType='%s' WHERE Code='%s'",
					name, charset, language, classes, singer1, singer2, singer3, \
					singer4, num, pinyin, wbh, playnum, klok, sound, filesize, \
					isnewsong, volumek, volumes, videotype, code);
//				printf("%s\n", sql);
				sqlite_exec(db, sql, NULL, NULL, NULL);
			}
			else{
				sprintf(sql, "INSERT INTO System(Code,Name,Charset,Language,Class,Singer1, \
Singer2,Singer3,Singer4,Num,Pinyin,WBH,PlayNum,Klok,Sound,FileSize, \
isNewSong,VolumeK,VolumeS,VideoType)  \
VALUES('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s', \
'%s','%s','%s','%s','%s','%s');",
					code,name,charset,language,classes,singer1,singer2,singer3,singer4,
					num,pinyin,wbh,playnum,klok,sound,filesize,isnewsong,volumek,
					volumes,videotype);
//				printf("%s\n", sql);
				sqlite_exec(db, sql, NULL, NULL, NULL);
			}
			char SourceFileName[512], Dist[512];
			sprintf(SourceFileName, "%s/%s", argv[1], filename);
			sprintf(Dist, "%s.%s", code, videotype);
			strlow(Dist);
			const char *DestFileName = GetMaxSizeDisk(SourceFileName, Dist);
			printf("CopyFile %s to %s\n", SourceFileName, DestFileName);
			if (DestFileName)
				CopyFile( SourceFileName, DestFileName );
		}
		pEntry = pEntry->pChild;
	}
	UdpSendStr("*", 31016, "");
	sqlite_close(db);
 	return 0;
}


static int UpdateCallBack(void *NotUsed, int argc, char **argv, char **azColName)
{
	int* update = (int *) NotUsed;
	*update = atoi(argv[0]);
	return 0;
}

int CopyFile(const char *oldfile, const char *newfile)
{
	FILE *fpin, *fpout;
	fpin = fopen(oldfile, "rb");
	if (fpin == NULL) return -1;
	fpout = fopen(newfile, "wb");
	if (fpout == NULL) return -1;
	char buffer[2049];
	int size;
	while (!feof(fpin)) {
		size = fread(buffer, 1, 2048, fpin);
		fwrite(buffer, 1, size, fpout);
	}
	fclose(fpin);
	fclose(fpout);

	return 0;
}
