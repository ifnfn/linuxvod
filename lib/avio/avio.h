#ifndef AVIO_H
#define AVIO_H

/* output byte stream handling */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#ifdef LINUX
#include <error.h>
#include <assert.h>
#endif

#include "network.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AVERROR_IO          (-2)  /* i/o error */

#define LIBAVFORMAT_IDENT       "Silicon"

#define av_malloc malloc
#define av_free free
#define av_realloc realloc

typedef int64_t offset_t;

struct URLContext {
	struct URLProtocol *prot;
	int flags;
	int is_streamed;  /* true if streamed (no seek possible), default = false */
	int max_packet_size;  /* if non zero, the stream is packetized with this max packet size */
	void *priv_data;
	char filename[1]; /* specified filename */
};

typedef struct URLContext URLContext;

#define URL_RDONLY 0
#define URL_WRONLY 1
#define URL_RDWR   2

int resolve_host(struct in_addr *sin_addr, const char *hostname);

int url_open     (URLContext **h, const char *filename, int flags);
int url_read     (URLContext *h, unsigned char *buf, int size);
int url_write    (URLContext *h, unsigned char *buf, int size);
offset_t url_seek(URLContext *h, offset_t pos, int whence);
int url_close    (URLContext *h);
int url_exist    (const char *filename);
offset_t url_size(URLContext *h);
void *url_readbuf(const char *url, offset_t *size);
inline int url_get_max_packet_size(URLContext *h);

typedef struct {
	unsigned char *buffer;
	int buffer_size;
	unsigned char *buf_ptr, *buf_end;
	void *opaque;
	int (*read_packet)(void *opaque, uint8_t *buf, int buf_size);
	int (*write_packet)(void *opaque, uint8_t *buf, int buf_size);
	offset_t (*seek)(void *opaque, offset_t offset, int whence);
	offset_t pos; /* position in the file of the current buffer */
	int must_flush; /* true if the next seek should flush */
	int eof_reached; /* true if eof reached */
	int write_flag;  /* true if open for writing */
	int is_streamed;
	int max_packet_size;
	unsigned long checksum;
	unsigned char *checksum_ptr;
	unsigned long (*update_checksum)(unsigned long checksum, const uint8_t *buf, unsigned int size);
	int error;         ///< contains the error code or 0 if no error happened
} ByteIOContext;

void put_byte  (ByteIOContext *s, int b);
void put_buffer(ByteIOContext *s, const unsigned char *buf, int size);
void put_le64  (ByteIOContext *s, uint64_t val);
void put_be64  (ByteIOContext *s, uint64_t val);
void put_le32  (ByteIOContext *s, unsigned int val);
void put_be32  (ByteIOContext *s, unsigned int val);
void put_be24  (ByteIOContext *s, unsigned int val);
void put_le16  (ByteIOContext *s, unsigned int val);
void put_be16  (ByteIOContext *s, unsigned int val);
void put_tag   (ByteIOContext *s, const char *tag);
void put_strz  (ByteIOContext *s, const char *buf);

int          get_buffer         (ByteIOContext *s, unsigned char *buf, int size);
int          get_partial_buffer (ByteIOContext *s, unsigned char *buf, int size);
int          get_byte           (ByteIOContext *s);
unsigned int get_le32           (ByteIOContext *s);
uint64_t     get_le64           (ByteIOContext *s);
unsigned int get_le16           (ByteIOContext *s);
char        *get_strz           (ByteIOContext *s, char *buf, int maxlen);
unsigned int get_be16           (ByteIOContext *s);
unsigned int get_be24           (ByteIOContext *s);
unsigned int get_be32           (ByteIOContext *s);
uint64_t     get_be64           (ByteIOContext *s);

offset_t url_fseek     (ByteIOContext *s, offset_t offset, int whence);
void     url_fskip     (ByteIOContext *s, offset_t offset);
offset_t url_ftell     (ByteIOContext *s);
offset_t url_fsize     (ByteIOContext *s);
int      url_feof      (ByteIOContext *s);
int      url_ferror    (ByteIOContext *s);

#define URL_EOF (-1)
int      url_fgetc     (ByteIOContext *s);
#ifdef __GNUC__
int      url_fprintf   (ByteIOContext *s, const char *fmt, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
#else
int      url_fprintf   (ByteIOContext *s, const char *fmt, ...);
#endif
char    *url_fgets     (ByteIOContext *s, char *buf, int buf_size);
int      url_fdopen    (ByteIOContext *s, URLContext *h);
int      url_setbufsize(ByteIOContext *s, int buf_size);
int      url_fopen     (ByteIOContext *s, const char *filename, int flags);
int      url_fclose    (ByteIOContext *s);
int      url_open_buf  (ByteIOContext *s, uint8_t *buf, int buf_size, int flags);
int      url_close_buf (ByteIOContext *s);
#define  url_fread get_buffer
#define  url_fwrite put_buffer
URLContext *url_fileno (ByteIOContext *s);

typedef struct URLProtocol {
	const char *name;
	int (*url_open)     (URLContext *h, const char *filename, int flags);
	int (*url_read)     (URLContext *h, unsigned char *buf, int size);
	int (*url_write)    (URLContext *h, unsigned char *buf, int size);
	offset_t (*url_seek)(URLContext *h, offset_t pos, int whence);
	int (*url_close)    (URLContext *h);
	offset_t (*url_size)(URLContext *h);
	struct URLProtocol *next;
} URLProtocol;

extern URLProtocol *first_protocol;

int register_protocol(URLProtocol *protocol);
void av_register_all(void);

#ifdef FILE_PROTO
/* file.c */
extern URLProtocol file_protocol;
extern URLProtocol pipe_protocol;
#endif

#ifdef UDP_PROTO
/* udp.c */
extern URLProtocol udp_protocol;
int udp_set_remote_url(URLContext *h, const char *uri);
int udp_get_local_port(URLContext *h);
int udp_get_file_handle(URLContext *h);
#endif

#ifdef TCP_PROTO
/* tcp.c  */
extern URLProtocol tcp_protocol;
#endif

#ifdef HTTP_PROTO
/* http.c */
extern URLProtocol http_protocol;
#endif

#ifdef RTP_PROTO
extern URLProtocol rtp_protocol;
#endif

#ifdef __cplusplus
}
#endif

#endif

