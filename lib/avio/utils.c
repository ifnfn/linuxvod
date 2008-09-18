#include "utils.h"

/**
 * Return TRUE if val is a prefix of str. If it returns TRUE, ptr is
 * set to the next character in 'str' after the prefix.
 *
 * @param str input string
 * @param val prefix to test
 * @param ptr updated after the prefix in str in there is a match
 * @return TRUE if there is a match
 */
int strstart(const char *str, const char *val, const char **ptr)
{
	const char *p, *q;
	p = str;
	q = val;
	while (*q != '\0')
	{
		if (*p != *q)
			return 0;
		p++;
		q++;
	}
	if (ptr)
		*ptr = p;
	return 1;
}

/**
 * Return TRUE if val is a prefix of str (case independent). If it
 * returns TRUE, ptr is set to the next character in 'str' after the
 * prefix.
 *
 * @param str input string
 * @param val prefix to test
 * @param ptr updated after the prefix in str in there is a match
 * @return TRUE if there is a match */
int stristart(const char *str, const char *val, const char **ptr)
{
	const char *p, *q;
	p = str;
	q = val;
	while (*q != '\0')
	{
		if (toupper(*(const unsigned char *)p) != toupper(*(const unsigned char *)q))
			return 0;
		p++;
		q++;
	}
	if (ptr)
		*ptr = p;
	return 1;
}

/**
 * Copy the string str to buf. If str length is bigger than buf_size -
 * 1 then it is clamped to buf_size - 1.
 * NOTE: this function does what strncpy should have done to be
 * useful. NEVER use strncpy.
 *
 * @param buf destination buffer
 * @param buf_size size of destination buffer
 * @param str source string
 */
void pstrcpy(char *buf, int buf_size, const char *str)
{
	int c;
	char *q = buf;

	if (buf_size <= 0)
		return;

	for(;;)
	{
		c = *str++;
		if (c == 0 || q >= buf + buf_size - 1)
			break;
		*q++ = c;
	}
	*q = '\0';
}

/* strcat and truncate. */
char *pstrcat(char *buf, int buf_size, const char *s)
{
	int len;
	len = strlen(buf);
	if (len < buf_size)
		pstrcpy(buf + len, buf_size - len, s);
	return buf;
}

void url_split(char *proto, int proto_size,
				char *authorization, int authorization_size,
				char *hostname, int hostname_size,
				int *port_ptr,
				char *path, int path_size,
				const char *url)
{
	const char *p;
	char *q;
	int port, id;

	port = -1;

	p = url;
	q = proto;
	while (*p != ':' && *p != '\0')
	{
		if ((q - proto) < proto_size - 1)
			*q++ = *p;
		p++;
	}
	if (proto_size > 0)
		*q = '\0';
	if (authorization_size > 0)
		authorization[0] = '\0';
	if (*p == '\0')
	{
		if (proto_size > 0)
			proto[0] = '\0';
		if (hostname_size > 0)
			hostname[0] = '\0';
		p = url;
	} else {
		char *at,*slash; // PETR: position of '@' character and '/' character

		p++;
		if (*p == '/')
			p++;
		if (*p == '/')
			p++;
		at = strchr(p,'@'); // PETR: get the position of '@'
		slash = strchr(p,'/');  // PETR: get position of '/' - end of hostname
		if (at && slash && at > slash) at = NULL; // PETR: not interested in '@' behind '/'

		q = at ? authorization : hostname;  // PETR: if '@' exists starting with auth.

		while ((at || *p != ':') && *p != '/' && *p != '?' && *p != '\0')// PETR:
		{
			if (*p == '@') // PETR: passed '@'
			{
				if (authorization_size > 0)
					*q = '\0';
				q = hostname;
				at = NULL;
			} else if (!at) {   // PETR: hostname
				if ((q - hostname) < hostname_size - 1)
					*q++ = *p;
			}
			else {
				if ((q - authorization) < authorization_size - 1)
					*q++ = *p;
			}
			p++;
		}
		if (hostname_size > 0)
			*q = '\0';
		if (*p == ':')
		{
			p++;
			port = strtoul(p, (char **)&p, 10);
		}
	}
	if (port_ptr)
		*port_ptr = port;

	id = 0;
	for (;;) {
		int c = *p++;
		if (c == 0 || id >= path_size - 1)
			break;

		if (c == ' '){
			path[id++] = '%';
			path[id++] = '2'; 
			path[id++] = '0';
		}
		else
			path[id++]  = c;
	}
	path[id] = 0;
//pstrcpy(path, path_size, p);
}

/**
 * Attempts to find a specific tag in a URL.
 *
 * syntax: '?tag1=val1&tag2=val2...'. Little URL decoding is done.
 * Return 1 if found.
 */
int find_info_tag(char *arg, int arg_size, const char *tag1, const char *info)
{
	const char *p;
	char tag[128], *q;

	p = info;
	if (*p == '?')
		p++;
	for(;;)
	{
		q = tag;
		while (*p != '\0' && *p != '=' && *p != '&')
		{
			if ((q - tag) < sizeof(tag) - 1)
				*q++ = *p;
			p++;
		}
		*q = '\0';
		q = arg;
		if (*p == '=')
		{
			p++;
			while (*p != '&' && *p != '\0')
			{
				if ((q - arg) < arg_size - 1)
				{
					if (*p == '+')
						*q++ = ' ';
					else
						*q++ = *p;
				}
				p++;
			}
			*q = '\0';
		}
		if (!strcmp(tag, tag1))
			return 1;
		if (*p != '&')
			break;
		p++;
	}
	return 0;
}
