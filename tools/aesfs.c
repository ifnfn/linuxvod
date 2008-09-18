#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include <fuse.h>
#include <fcntl.h>
#include "aes.h"

//#define DEBUG
static char find_code[256] = "";
static int find_count;

static char *getfilename_passwd(const char *path, char *base, char *passwd)
{
	char *pwd = rindex(path, '@');
	if (pwd) {
		strcpy(passwd, pwd+1);
		memcpy(base, path, pwd - path);
		base[pwd-path]= 0;
	}
	else {
		passwd[0] = '\0';
		strcpy(base, path);
	}

	return base;
}

static int find_fn(const char *file, const struct stat *sb, int flag)
{
	if (flag == FTW_F) {
		char code[100];
		char *p = rindex(file, '/');
		if (p) 
			p++; 
		else 
			p = (char *)file;

		if (strncmp(p, find_code, strlen(find_code)) == 0) {
			strncpy(find_code, file, 255);
			find_count ++;

			return 1;
		}
	}
	return 0;
}

static char *GetFileByName(const char *path, const char *code)
{
	find_count = 0;
	strncpy(find_code, code, 255);
	ftw(path, find_fn, 50);

	if (find_count > 0)
		return find_code;

	return NULL;
}

static int video_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if(strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else if (path){
		char *filename;
#ifdef DEBUG
		FILE *fp = fopen("/tmp/abc", "a+");
		fprintf(fp, "%s: path=%s\n", __FUNCTION__, path+1);
#endif
//		filename = AesDecryptStringDefault(path+1);
//		if (filename == NULL)
//			return -ENOENT;
		filename = strdup(path+1);

#ifdef DEBUG
		fprintf(fp, "\tAesDecryptString filename:%s\n", filename);
#endif
		
		char base[512], passwd[20];
		if (getfilename_passwd(filename, base, passwd) == NULL){
			free(filename);
			return -ENOENT;
		}

		free(filename);

		filename = GetFileByName("/tmp/video", base);
		if (filename != NULL) {	
			stbuf->st_mode = S_IFREG | 0444;
			stbuf->st_nlink = 1;
			stbuf->st_size = GetAesFileSizeByName(filename);
		}
		else 
			res = -ENOENT;
#ifdef DEBUG
		fprintf(fp, "\tbase=%s, passwd=%s, size=%ld\n", base, passwd, stbuf->st_size);
		fclose(fp);
#endif	
	}
	else 
		res = -ENOENT;

	return res;
}

static int video_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;
	
	if(strcmp(path, "/") != 0)
		return -ENOENT;
	
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
//	filler(buf, video_path + 1, NULL, 0);
	
	return 0;
}

static int video_open(const char *path, struct fuse_file_info *fi)
{
	char  *filename;
	char  base[512], passwd[20];
#ifdef DEBUG
	FILE *fp = fopen("/tmp/abc", "a+");
	fprintf(fp, "%s: path=%s\n", __FUNCTION__, path+1);
#endif
//	filename = AesDecryptStringDefault(path+1);
//	if (filename == NULL)
//		return -ENOENT;

	filename = strdup(path + 1);
#ifdef DEBUG
	fprintf(fp, "\tAesDecryptString filename:%s\n", filename);
#endif
	if (getfilename_passwd(filename, base, passwd) == NULL) {
		free(filename);
		return -ENOENT;
	}
	free(filename);

#ifdef DEBUG
	fprintf(fp, "\tbase=%s, passwd=%s\n", base, passwd);
	fclose(fp);
#endif	
	filename = GetFileByName("/tmp/video", base);
	struct AesFile *aes = AesOpenFile(filename, passwd, 1);

	if (aes == NULL)
		return -ENOENT;

	fi->fh = (unsigned long)aes;

	return 0;
}
int video_write(const char *path, const char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{

	return 0;
}

static int video_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
	int len;
	struct AesFile *aes = (struct AesFile *)(uintptr_t)fi->fh;

	if (aes == NULL)
		return -ENOENT;

	len = AesReadFile(aes, buf, size, offset);
#ifdef DEBUG
	FILE *fp = fopen("/tmp/abc", "a+");
	fprintf(fp, "%s: %s, size=%ld, offset=%lld\n", __FUNCTION__, path+1, size, offset);
	fprintf(fp, "len=%d\n", len);
	fclose(fp);
#endif

	return len;
}

static int video_release(const char *path, struct fuse_file_info *fi)
{
	struct AesFile *aes = (struct AesFile *)(uintptr_t)fi->fh;
	if (aes)
		AesCloseFile(aes);
#ifdef DEBUG
	FILE *fp = fopen("/tmp/abc", "a+");
	fprintf(fp, "%s: %s\n", __FUNCTION__, path+1);
	fclose(fp);
#endif

	return 0;
}

static int video_statfs(const char *path, struct statvfs *stbuf) 
{
	int res;
	(void) path;
#ifdef DEBUG
	FILE *fp = fopen("/tmp/abc", "a+");
	fprintf(fp, "%s(%s)\n", __FUNCTION__, path);
	fclose(fp);
#endif
	return 0;
}


static struct fuse_operations video_oper = {
	.getattr  = video_getattr,
	.readdir  = video_readdir,
	.open     = video_open,
	.read     = video_read,
	.write    = video_write,
	.release  = video_release,
	.statfs   = video_statfs,
};

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &video_oper);
}
