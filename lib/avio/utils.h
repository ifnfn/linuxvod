#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int strstart(const char *str, const char *val, const char **ptr);
int stristart(const char *str, const char *val, const char **ptr);
void pstrcpy(char *buf, int buf_size, const char *str);
char *pstrcat(char *buf, int buf_size, const char *s);
void url_split(char *proto, int proto_size,
				char *authorization, int authorization_size,
				char *hostname, int hostname_size,
				int *port_ptr,
				char *path, int path_size,
				const char *url);
int find_info_tag(char *arg, int arg_size, const char *tag1, const char *info);

#ifdef __cplusplus
}
#endif

#endif
