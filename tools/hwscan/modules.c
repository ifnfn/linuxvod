/*
 * Note: we don't use any kind of indexes (btrees, etc) here, because
 * that would add more overhead than it would improve performance.
 */

#include "hwscan2.h"
#include <sys/utsname.h>

struct modmap_t *modmap = NULL;
char modules_dir[100];

FILE *fopen_md(char *relative_filename)
{
	char full_filename[4096];
	snprintf(full_filename, 4096, "%s/%s", modules_dir, relative_filename);
	return fopen(full_filename, "r");
}

void read_modmap()
{
	struct utsname un;
	char line[4096];
	char *tmppos;

	uname(&un);
	snprintf(modules_dir, 100, "/lib/modules/%s", un.release);

	FILE *f = fopen_md("modules.dep");
	while ( fgets(line, 4095, f) != NULL ) {
		line[4095] = 0;
		if ( line[0] == ' ' || line[0] == '\t' ||
		     line[0] == '\n' || !line[0] ) continue;
		if ( (tmppos = strchr(line, ':')) != NULL ) *tmppos = 0;
		if ( (tmppos = strrchr(line, '.')) != NULL ) *tmppos = 0;
		if ( (tmppos = strrchr(line, '/')) != NULL ) {
			struct modmap_t *newmod =
				malloc(sizeof(struct modmap_t));
			newmod->basename = malloc(strlen(tmppos));
			strcpy(newmod->basename, tmppos+1);
			*(tmppos+1) = 0;
			newmod->dirname = malloc(strlen(line)+1);
			strcpy(newmod->dirname, line);
			newmod->next = modmap;
			modmap = newmod;
		}
	}
	fclose(f);
}

struct modmap_t * find_module(char * line)
{
	struct modmap_t *p = modmap;
	int thatsit, c;

	while ( p ) {
		thatsit = 1;
		for (c=0; line[c] && line[c]!=' ' &&
		     line[c]!='\t' && line[c]!='\n'; c++)
			if ( line[c] != p->basename[c] ) {
				thatsit = 0;
				break;
			}
		if ( thatsit && !p->basename[c] )
			return p;
		p = p->next;
	}

	return NULL;
}

