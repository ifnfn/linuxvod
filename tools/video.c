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

#define AES
//#define DEBUG
static char find_code[256] = "";
static int find_count;

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

static int hello_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
#ifdef DEBUG
	FILE *fp = fopen("/tmp/abc", "a+");
	fprintf(fp, "path=%s\n", path);
#endif
	memset(stbuf, 0, sizeof(struct stat));
	if(strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else if (path){
		char   *filename = GetFileByName("/tmp/video", path +1);
		if (filename != NULL) {	
			stbuf->st_mode = S_IFREG | 0444;
			stbuf->st_nlink = 1;
#ifdef AES
			stbuf->st_size = GetSizeByFileName(filename);
#else
			res = lstat(filename, stbuf);
			if (res == -1)
				res = -ENOENT;
#endif
//			fprintf(fp, "stbuf->st_size =%ld\n", stbuf->st_size);
		}
		else 
			res = -ENOENT;
#ifdef DEBUG
		fclose(fp);
#endif
	}
	else 
		res = -ENOENT;

	return res;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;
	
	if(strcmp(path, "/") != 0)
		return -ENOENT;
	
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
//	filler(buf, hello_path + 1, NULL, 0);
	
	return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{
	char  *filename;

	filename = GetFileByName("/tmp/video", path+1);
#ifdef AES
#ifdef DEBUG
	FILE *fp = fopen("/tmp/abc", "a+");
	fprintf(fp, "hello_open %s\n", filename);
	fclose(fp);
#endif
	struct AesFile *aes = AesOpenFile(filename, "cnsczd", 1);

	if (aes == NULL)
		return -ENOENT;

	fi->fh = (unsigned long)aes;

	return 0;
#else
	if (filename == NULL)
		return -ENOENT;
#ifdef DEBUG
	FILE *fp = fopen("/tmp/abc", "a+");
	if (fp) {
		fprintf(fp, "filename=%s\n", filename);
		fclose(fp);
	}
#endif

	int res = open(filename, fi->flags);
	if (res == -1)
		return -errno;

	close(res);

	return 0;
#endif
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
#ifdef AES
	struct AesFile *aes = (struct AesFile *)(uintptr_t)fi->fh;

	if (aes == NULL)
		return -ENOENT;
	int len = AesReadFile(aes, buf, size, offset);

#ifdef DEBUG
	FILE *fp = fopen("/tmp/abc", "a+");
	fprintf(fp, "start read %d offset=%ld\n", size, offset);
	if (fp) {
		fprintf(fp, "hello_read (%s, size:%ld, offset:%ld\n", path, size, offset);
		fprintf(fp, "read size:%d\n", len);
		fclose(fp);
	}
#endif
	return len;
#else	
	char *filename = GetFileByName("/tmp/video", path+1);

	int fd;
	int res;

	(void) fi;

	fd = open(filename, O_RDONLY);
	if (fd == -1)
		return -errno;
	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;
	close(fd);
#ifdef
	FILE *fp = fopen("/tmp/abc", "a+");
	if (fp){
		fprintf(fp, "hello_read(offset=%ld, size=%d, read=%d\n", offset, size, res);
		fclose(fp);
	}
#endif
	return res;
#endif
}

static int hello_release(const char *path, struct fuse_file_info *fi)
{
#ifdef AES
	struct AesFile *aes = (struct AesFile *)(uintptr_t)fi->fh;
	FILE *fp = fopen("/tmp/abc", "a+");
	if (aes) {
		AesCloseFile(aes);
		fprintf(fp, "hello_release: %s\n", path);
		fi->fh = 0;
	}
	else
		fprintf(fp, "aes=NULL\n");
	fclose(fp);
#else
	FILE *fp = fopen("/tmp/abc", "a+");
	if (fp) {
		fprintf(fp, "hello_release: %s\n", path);
		fclose(fp);
	}
#endif
	return 0;
}

static int hello_statfs(const char *path, struct statvfs *stbuf) 
{
	int res;
	(void) path;
	FILE *fp = fopen("/tmp/abc", "a+");
	fprintf(fp, "%s(%s)\n", __FUNCTION__, path);
	fclose(fp);

	return 0;
}


static struct fuse_operations hello_oper = {
	.getattr  = hello_getattr,
	.readdir  = hello_readdir,
	.open     = hello_open,
	.read     = hello_read,
	.release  = hello_release,
//	.statfs   = hello_statfs,
};

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &hello_oper);
}
