//==============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "createindex.h"
#include "songstream.h"

#ifdef USESQLITEDB

static int compar(const void *p1, const void *p2)
{
	IndexNode *k1= (IndexNode*)p1, *k2 = (IndexNode*)p2;
	return strcmp(k1->IndexName, k2->IndexName);
}

CKeywordIndex::CKeywordIndex()
{
	SongCount  = 0;
	ClassCount = 0;
	IndexCount = 0;
	PinyinCount= 0;

	ClassList  = NULL;
#ifdef SAVE_FILE
	tmpfp = tmpfile();
#else
	SongLists  = NULL;
#endif
	PinyinList = NULL;
}

CKeywordIndex::~CKeywordIndex()
{
#ifdef SAVE_FILE
	fclose(tmpfp);
#else
	free(SongLists);
#endif
	for (int i=0;i<ClassCount;i++)
		free(ClassList[i].node);
	free(ClassList);
}

FieldClass* CKeywordIndex::AddIndexNode(const char *IndexName, FieldID id)
{
	int i;
	for (i=0;i < ClassCount;i++){
		if (ClassList[i].fieldid == id)
			break;
	}
	if (i >= ClassCount){
		i=ClassCount++;
		ClassList = (FieldClass *) realloc(ClassList, sizeof(FieldClass)*ClassCount);
		memset(ClassList + i, 0, sizeof(FieldClass));
		ClassList[i].fieldid = id;
	}
	int x = ClassList[i].count++;
	ClassList[i].node =(IndexNode *)realloc(ClassList[i].node, sizeof(IndexNode) * ClassList[i].count);
	memset(ClassList[i].node + x, 0, sizeof(IndexNode));
	strncpy(ClassList[i].node[x].IndexName, IndexName, FilterLen - 1);

	ClassList[i].node[x].count   = 0;
	ClassList[i].node[x].Filters = NULL;
	ClassList[i].sort = false;
	IndexCount++;
	return ClassList + i;
}

void CKeywordIndex::AddIndexNodeFilter(IndexNode *indexnode, FilterNode node)
{
	int id = indexnode->count++;
	indexnode->Filters = (FilterNode *) realloc(indexnode->Filters, sizeof(FilterNode) * indexnode->count);
	indexnode->Filters[id] = node;
}

void CKeywordIndex::FieldClassSort()
{
	for (int i=0;i<ClassCount;i++){
		qsort(ClassList[i].node, ClassList[i].count, sizeof(IndexNode), compar);
		ClassList[i].sort = true;
	}
}

IndexNode *CKeywordIndex::FindIndexNode(FieldClass *Class, const char *IndexName)
{
	if (IndexName[0] == '\0') return NULL;
	if (Class->sort == false){
		qsort(Class->node, Class->count, sizeof(IndexNode), compar);
		Class->sort = true;
	}
	int Low = 0, High = Class->count - 1, Mid, comp;
	while(Low <= High)
	{
		Mid = (Low + High) / 2;
		comp = strcmp(Class->node[Mid].IndexName, IndexName);
		if (comp < 0)
			Low = Mid + 1;
		else if (comp > 0)
			High = Mid - 1;
		else
			return Class->node + Mid;
	}
	return NULL;
}


void CKeywordIndex::Data2Data(SongListNode* To, SystemFields* From)
{
	strcpy(To->SongCode  , From->SongCode);
	strcpy(To->SongName  , From->SongName);
	strcpy(To->Language  , From->Language);
	strcpy(To->SingerName, From->SingerName1);
	strcpy(To->StreamType, From->StreamType);
	To->Charset   = From->Charset;
	To->VolumeK   = From->VolumeK;
	To->VolumeS   = From->VolumeS;
	To->Num       = From->Num;
	To->Klok      = From->Klok;
	To->Sound     = From->Sound;
	To->SoundMode = From->SoundMode;
//	To->Password  = From->Password;
#if 0
	printf("Code %s = %s\n", To->SongCode, From->SongCode);
	printf("\tName %s = %s\n", To->SongName, From->SongName);
	printf("\tLanguage %s = %s\n", To->Language, From->Language);
	printf("\tSingerName %s = %s\n", To->SingerName, From->SingerName1);
	printf("\tStreamType %s = %s\n", To->StreamType, From->StreamType);
	printf("\tCharset %d = %d\n", To->Charset, From->Charset);
	printf("\tVolumeK %d = %d\n\tVolumeS %d = %d\n", To->VolumeK, From->VolumeK, To->VolumeS, From->VolumeS);
	printf("\tNum %d = %d\n\tSoundMode %d = %d\n", To->Num, From->Num, To->SoundMode, From->SoundMode);
	printf("\tKlok %c = %c\n\tSound %c = %c\n", To->Klok, From->Klok, To->Sound, From->Sound);
#endif
}

void CKeywordIndex::AddSongData(SystemFields *data, bool check)
{
//	printf("check=%d, data->SongCode=%s\n", check, data->SongCode);
	if (check && data->SongCode[0] != '-'){
		if (codehash.HaveCode(data->SongCode, SongCount))
			return;
	}
	long id = SongCount++;
#ifdef SAVE_FILE
	SongListNode songnode, *p = &songnode;
#else
	SongLists = (SongListNode *) realloc(SongLists, sizeof(SongListNode) * SongCount);
	SongListNode *p = SongLists + id;
	memset(p, 0, sizeof(SongListNode));
#endif
	Data2Data(p, data);
#ifdef SAVE_FILE
	fwrite(&songnode, sizeof(SongListNode), 1, tmpfp);
#endif
	FilterNode node;
	node.SongID = id;
	strncpy(node.subfilter, data->PinYin, 4);

	if (data->PinYin[0] != '\0' && (unsigned char)data->PinYin[0] <= (unsigned char)0x80)
	{
		PinyinCount++;
		PinyinList = (Pinyin_t*)realloc(PinyinList, sizeof(Pinyin_t) * PinyinCount);
		Pinyin_t *p = PinyinList + PinyinCount - 1;
		strncpy(p->PinYin, data->PinYin, FilterLen - 1);
		strncpy(p->WBH, data->WBH, FilterLen - 1);
		p->Num = data->Num;
		p->SongID = id;
	}

	FieldClass *f = NULL;
	for (int i=0;i<ClassCount;i++)
	{
		IndexNode *p = NULL;
		f = ClassList + i;
		switch (f->fieldid)
		{
			case F_CLASS:
				p = FindIndexNode(f, data->Class);
				break;
			case F_LANGUAGE:
				p = FindIndexNode(f, data->Language);
				break;
			case F_SINGER:
				p = FindIndexNode(f, data->SingerName1);
				if (p) break;
				if (!p)	p = FindIndexNode(f, data->SingerName2); else break;
				if (!p)	p = FindIndexNode(f, data->SingerName3); else break;
				if (!p)	p = FindIndexNode(f, data->SingerName4); else break;
				break;
			case F_SEX:
				p = FindIndexNode(f, data->Sex);
				break;
			case F_NUM:{
				char tmp[20];
				if (data->Num > 9)
					strcpy(tmp, "0×Ö¸è");
				else
					sprintf(tmp, "%d×Ö¸è", data->Num);

				p = FindIndexNode(f, tmp);
				break;
			}
			case F_HOT:
				break;
			case F_NEWSONG:
				if (data->IsNewSong)
					p = FindIndexNode(f, "ÐÂ¸èÍÆ¼ö");
				break;
			case F_UNKOWN:
				break;
		}
		if (p)
			AddIndexNodeFilter(p, node);
	}
}

void CKeywordIndex::printIndex()
{
	for (int i=0;i<ClassCount;i++){
		for (int j=0;j<ClassList[i].count;j++)
			printf("IndexName(%d): %s(%d)\n", i, ClassList[i].node[j].IndexName, ClassList[i].node[j].count);
	}
}

static int pos = 0;
static void myurl_write(URLContext* ucp, void *data, int len)
{
	char *buf = (char *)malloc(len);
	memcpy(buf, data, len);
	for (int i=0;i<len;i++)
		buf[i] = buf[i] ^(char)(pos++);
	url_write(ucp, (unsigned char *)buf, len);
	free(buf);
}

void CKeywordIndex::SaveFile(const char *url)
{
	long IndexSize = 0;
	for (int i=0;i<ClassCount;i++)
		for (int j=0;j<ClassList[i].count;j++)
			IndexSize += FilterLen + sizeof(int) + ClassList[i].node[j].count * sizeof(struct tagFilterNode);
	long PySize = PinyinCount * sizeof(Pinyin_t);

	FHead.IndexCount   = IndexCount;

	FHead.PYCount      = PinyinCount;
	FHead.PyPosition   = sizeof(FHead) + IndexSize;
	FHead.DataPosition = sizeof(FHead) + IndexSize + PySize;

	FHead.SongCount    = SongCount;
	av_register_all();
	URLContext *ucp = NULL;
	url_open(&ucp, url, URL_WRONLY);
	pos = 0;
	if (ucp == NULL) return;
	myurl_write(ucp, (unsigned char *)&FHead, sizeof(FHead));
	for (int i=0;i<ClassCount;i++)
		for (int j=0;j<ClassList[i].count;j++) {
			myurl_write(ucp, ClassList[i].node[j].IndexName, FilterLen);
			myurl_write(ucp, &ClassList[i].node[j].count, sizeof(int));
			myurl_write(ucp, ClassList[i].node[j].Filters, sizeof(struct tagFilterNode)*ClassList[i].node[j].count);
		}
	for (int i=0;i<PinyinCount;i++)
		myurl_write(ucp, (unsigned char *)(PinyinList+i), sizeof(Pinyin_t));
	SongListNode songnode;
	fseek(tmpfp, 0, SEEK_SET);

	for (int i=0;i<SongCount;i++) {
#ifdef SAVE_FILE
		fread(&songnode, sizeof(SongListNode), 1, tmpfp);
		myurl_write(ucp, (unsigned char *)&songnode, sizeof(SongListNode));
#else
		myurl_write(ucp, (unsigned char *)(SongLists+i), sizeof(SongListNode));
#endif
	}
	url_close(ucp);
}

void CKeywordIndex::SaveTo(int fd)
{
	for (int i=0;i<ClassCount;i++)
		for (int j=0;j<ClassList[i].count;j++) {
			write(fd, ClassList[i].node[j].IndexName, FilterLen);
		}
}

void CSongIndex::AddHotSongData(SystemFields *data)
{
	int id = codehash.FindCode(data->SongCode, 1);
	if (id >= 0)
	{
		FilterNode node;
		node.SongID = id;
		strncpy(node.subfilter, data->PinYin, 4);
//		printf("SongID=%d,sub=%s\t %d, %s\n", node.SongID, data->PinYin, id, data->PinYin);

		AddIndexNodeFilter(HotIndexNode, node);
	}
}
#endif

