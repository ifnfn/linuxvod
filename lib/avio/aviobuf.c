/*
 * Buffered I/O for ffmpeg system
 * Copyright (c) 2000,2001 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "avio.h"
#include <stdarg.h>

#define IO_BUFFER_SIZE 32768

int init_put_byte(ByteIOContext *s,
	unsigned char *buffer,
	int buffer_size,
	int write_flag,
	void *opaque,
	int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
	int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
	offset_t (*seek)(void *opaque, offset_t offset, int whence))
{
	s->buffer = buffer;
	s->buffer_size = buffer_size;
	s->buf_ptr = buffer;
	s->write_flag = write_flag;
	if (!s->write_flag)
		s->buf_end = buffer;
	else
		s->buf_end = buffer + buffer_size;
	s->opaque = opaque;
	s->write_packet = write_packet;
	s->read_packet = read_packet;
	s->seek = seek;
	s->pos = 0;
	s->must_flush = 0;
	s->eof_reached = 0;
	s->error = 0;
	s->is_streamed = 0;
	s->max_packet_size = 0;
	s->update_checksum= NULL;
	return 0;
}

static void flush_buffer(ByteIOContext *s)
{
	if (s->buf_ptr > s->buffer)
	{
		if (s->write_packet && !s->error)
		{
			int ret= s->write_packet(s->opaque, s->buffer, s->buf_ptr - s->buffer);
			if(ret < 0)
				s->error = ret;
		}
		if(s->update_checksum)
		{
			s->checksum= s->update_checksum(s->checksum, s->checksum_ptr, s->buf_ptr - s->checksum_ptr);
			s->checksum_ptr= s->buffer;
		}
		s->pos += s->buf_ptr - s->buffer;
	}
	s->buf_ptr = s->buffer;
}

void put_byte(ByteIOContext *s, int b)
{
	*(s->buf_ptr)++ = b;
	if (s->buf_ptr >= s->buf_end)
		flush_buffer(s);
}

void put_buffer(ByteIOContext *s, const unsigned char *buf, int size)
{
	int len;

	while (size > 0)
	{
		len = (s->buf_end - s->buf_ptr);
		if (len > size)
			len = size;
		memcpy(s->buf_ptr, buf, len);
		s->buf_ptr += len;

		if (s->buf_ptr >= s->buf_end)
			flush_buffer(s);

		buf += len;
		size -= len;
	}
}

void put_flush_packet(ByteIOContext *s)
{
	flush_buffer(s);
	s->must_flush = 0;
}

offset_t url_fseek(ByteIOContext *s, offset_t offset, int whence)
{
	offset_t offset1;

	if (whence != SEEK_CUR && whence != SEEK_SET)
		return -EINVAL;

	if (s->write_flag)
	{
		if (whence == SEEK_CUR)
		{
			offset1 = s->pos + (s->buf_ptr - s->buffer);
			if (offset == 0)
				return offset1;
			offset += offset1;
		}
		offset1 = offset - s->pos;
		if (!s->must_flush && offset1 >= 0 && offset1 < (s->buf_end - s->buffer)) {
			/* can do the seek inside the buffer */
			s->buf_ptr = s->buffer + offset1;
		}
		else {
			if (!s->seek)
				return -EPIPE;
			flush_buffer(s);
			s->must_flush = 1;
			s->buf_ptr = s->buffer;
			s->seek(s->opaque, offset, SEEK_SET);
			s->pos = offset;
		}
	}
	else {
		if (whence == SEEK_CUR)
		{
			offset1 = s->pos - (s->buf_end - s->buffer) + (s->buf_ptr - s->buffer);
			if (offset == 0)
				return offset1;
			offset += offset1;
		}
		offset1 = offset - (s->pos - (s->buf_end - s->buffer));
		if (offset1 >= 0 && offset1 <= (s->buf_end - s->buffer)) {
			/* can do the seek inside the buffer */
			s->buf_ptr = s->buffer + offset1;
		}
		else {
			if (!s->seek)
				return -EPIPE;
			s->buf_ptr = s->buffer;
			s->buf_end = s->buffer;
			if (s->seek(s->opaque, offset, SEEK_SET) == (offset_t)-EPIPE)
				return -EPIPE;
			s->pos = offset;
		}
		s->eof_reached = 0;
	}
	return offset;
}

void url_fskip(ByteIOContext *s, offset_t offset)
{
	url_fseek(s, offset, SEEK_CUR);
}

offset_t url_ftell(ByteIOContext *s)
{
	return url_fseek(s, 0, SEEK_CUR);
}

offset_t url_fsize(ByteIOContext *s)
{
	offset_t size;

	if (!s->seek)
		return -EPIPE;
//	size = s->seek(s->opaque, -1, SEEK_END) + 1;
	size = s->seek(s->opaque, -1, SEEK_END);
	s->seek(s->opaque, s->pos, SEEK_SET);
	return size;
}

int url_feof(ByteIOContext *s)
{
	return s->eof_reached;
}

int url_ferror(ByteIOContext *s)
{
	return s->error;
}

void put_le32(ByteIOContext *s, unsigned int val)
{
	put_byte(s, val);
	put_byte(s, val >> 8);
	put_byte(s, val >> 16);
	put_byte(s, val >> 24);
}

void put_be32(ByteIOContext *s, unsigned int val)
{
	put_byte(s, val >> 24);
	put_byte(s, val >> 16);
	put_byte(s, val >> 8);
	put_byte(s, val);
}

void put_strz(ByteIOContext *s, const char *str)
{
	if (str)
		put_buffer(s, (const unsigned char *) str, strlen(str) + 1);
	else
		put_byte(s, 0);
}

void put_le64(ByteIOContext *s, uint64_t val)
{
	put_le32(s, (uint32_t)(val & 0xffffffff));
	put_le32(s, (uint32_t)(val >> 32));
}

void put_be64(ByteIOContext *s, uint64_t val)
{
	put_be32(s, (uint32_t)(val >> 32));
	put_be32(s, (uint32_t)(val & 0xffffffff));
}

void put_le16(ByteIOContext *s, unsigned int val)
{
	put_byte(s, val);
	put_byte(s, val >> 8);
}

void put_be16(ByteIOContext *s, unsigned int val)
{
	put_byte(s, val >> 8);
	put_byte(s, val);
}

void put_be24(ByteIOContext *s, unsigned int val)
{
	put_be16(s, val >> 8);
	put_byte(s, val);
}

void put_tag(ByteIOContext *s, const char *tag)
{
	while (*tag)
		put_byte(s, *tag++);
}

/* Input stream */

static void fill_buffer(ByteIOContext *s)
{
	int len;

	/* no need to do anything if EOF already reached */
	if (s->eof_reached)
		return;

	if(s->update_checksum)
	{
		if(s->buf_end > s->checksum_ptr)
			s->checksum= s->update_checksum(s->checksum, s->checksum_ptr, s->buf_end - s->checksum_ptr);
		s->checksum_ptr= s->buffer;
	}

	len = s->read_packet(s->opaque, s->buffer, s->buffer_size);
	if (len <= 0)
	{
		/* do not modify buffer if EOF reached so that a seek back can be done without rereading data */
		s->eof_reached = 1;
		if(len<0)
			s->error= len;
		}
		else {
			s->pos += len;
			s->buf_ptr = s->buffer;
			s->buf_end = s->buffer + len;
		}
	}

unsigned long get_checksum(ByteIOContext *s)
{
	s->checksum= s->update_checksum(s->checksum, s->checksum_ptr, s->buf_ptr - s->checksum_ptr);
	s->update_checksum= NULL;
	return s->checksum;
}

void init_checksum(ByteIOContext *s,
	unsigned long (*update_checksum)(unsigned long c, const uint8_t *p, unsigned int len),
	unsigned long checksum)
{
	s->update_checksum= update_checksum;
	if(s->update_checksum){
		s->checksum= s->update_checksum(checksum, NULL, 0);
		s->checksum_ptr= s->buf_ptr;
	}
}

/* NOTE: return 0 if EOF, so you cannot use it if EOF handling is
   necessary */
/* XXX: put an inline version */
int get_byte(ByteIOContext *s)
{
	if (s->buf_ptr < s->buf_end) {
		return *s->buf_ptr++;
	}
	else {
		fill_buffer(s);
		if (s->buf_ptr < s->buf_end)
			return *s->buf_ptr++;
		else
			return 0;
	}
}

/* NOTE: return URL_EOF (-1) if EOF */
int url_fgetc(ByteIOContext *s)
{
	if (s->buf_ptr < s->buf_end)
		return *s->buf_ptr++;
	else {
		fill_buffer(s);
		if (s->buf_ptr < s->buf_end)
			return *s->buf_ptr++;
		else
			return EOF; //TODO
	}
}

int get_buffer(ByteIOContext *s, unsigned char *buf, int size)
{
	int len, size1;

	size1 = size;
	while (size > 0)
	{
		len = s->buf_end - s->buf_ptr;
		if (len > size)
			len = size;
		if (len == 0)
		{
			if(size > s->buffer_size && !s->update_checksum)
			{
				len = s->read_packet(s->opaque, buf, size);
				if (len <= 0)
				{
					/* do not modify buffer if EOF reached so that a seek back can be done without rereading data */
					s->eof_reached = 1;
					if(len<0)
						s->error= len;
					break;
				}
				else {
					s->pos += len;
					size -= len;
					buf += len;
					s->buf_ptr = s->buffer;
					s->buf_end = s->buffer/* + len*/;
				}
			}
			else {
				fill_buffer(s);
				len = s->buf_end - s->buf_ptr;
				if (len == 0)
					break;
			}
		}
		else {
			memcpy(buf, s->buf_ptr, len);
			buf += len;
			s->buf_ptr += len;
			size -= len;
		}
	}
	return size1 - size;
}

int get_partial_buffer(ByteIOContext *s, unsigned char *buf, int size)
{
	int len;

	if(size<0)
		return -1;

	len = s->buf_end - s->buf_ptr;
	if (len == 0)
	{
		fill_buffer(s);
		len = s->buf_end - s->buf_ptr;
	}
	if (len > size)
		len = size;
	memcpy(buf, s->buf_ptr, len);
	s->buf_ptr += len;
	return len;
}

unsigned int get_le16(ByteIOContext *s)
{
	unsigned int val;
	val = get_byte(s);
	val |= get_byte(s) << 8;
	return val;
}

unsigned int get_le32(ByteIOContext *s)
{
	unsigned int val;
	val = get_le16(s);
	val |= get_le16(s) << 16;
	return val;
}

uint64_t get_le64(ByteIOContext *s)
{
	uint64_t val;
	val = (uint64_t)get_le32(s);
	val |= (uint64_t)get_le32(s) << 32;
	return val;
}

unsigned int get_be16(ByteIOContext *s)
{
	unsigned int val;
	val = get_byte(s) << 8;
	val |= get_byte(s);
	return val;
}

unsigned int get_be24(ByteIOContext *s)
{
	unsigned int val;
	val = get_be16(s) << 8;
	val |= get_byte(s);
	return val;
}
unsigned int get_be32(ByteIOContext *s)
{
	unsigned int val;
	val = get_be16(s) << 16;
	val |= get_be16(s);
	return val;
}

char *get_strz(ByteIOContext *s, char *buf, int maxlen)
{
	int i = 0;
	char c;

	while ((c = get_byte(s))) {
		if (i < maxlen-1)
			buf[i++] = c;
	}

	buf[i] = 0; /* Ensure null terminated, but may be truncated */
	return buf;
}

uint64_t get_be64(ByteIOContext *s)
{
	uint64_t val;
	val = (uint64_t)get_be32(s) << 32;
	val |= (uint64_t)get_be32(s);
	return val;
}

/* link with avio functions */

static int url_write_packet(void *opaque, uint8_t *buf, int buf_size)
{
	URLContext *h = (URLContext *)opaque;
	return url_write(h, buf, buf_size);
}

static int url_read_packet(void *opaque, uint8_t *buf, int buf_size)
{
	URLContext *h = (URLContext *)opaque;
	return url_read(h, buf, buf_size);
}

static offset_t url_seek_packet(void *opaque, offset_t offset, int whence)
{
	URLContext *h = (URLContext *)opaque;
	return url_seek(h, offset, whence);
	//return 0;
}

int url_fdopen(ByteIOContext *s, URLContext *h)
{
	uint8_t *buffer;
	int buffer_size, max_packet_size;


	max_packet_size = url_get_max_packet_size(h);
	if (max_packet_size)
		buffer_size = max_packet_size; /* no need to bufferize more than one packet */
	else
		buffer_size = IO_BUFFER_SIZE;
	buffer = (uint8_t*)av_malloc(buffer_size);
	if (!buffer)
		return -ENOMEM;

	if (init_put_byte(s, buffer, buffer_size,
				(h->flags & URL_WRONLY || h->flags & URL_RDWR), h,
				url_read_packet, url_write_packet, url_seek_packet) < 0)
	{
		av_free(buffer);
		return AVERROR_IO;
	}
	s->is_streamed = h->is_streamed;
	s->max_packet_size = max_packet_size;
	return 0;
}

/* XXX: must be called before any I/O */
int url_setbufsize(ByteIOContext *s, int buf_size)
{
	uint8_t *buffer;
	buffer = (uint8_t *)av_malloc(buf_size);
	if (!buffer)
		return -ENOMEM;

	av_free(s->buffer);
	s->buffer = buffer;
	s->buffer_size = buf_size;
	s->buf_ptr = buffer;
	if (!s->write_flag)
		s->buf_end = buffer;
	else
		s->buf_end = buffer + buf_size;
	return 0;
}

/* NOTE: when opened as read/write, the buffers are only used for
   reading */
int url_fopen(ByteIOContext *s, const char *filename, int flags)
{
	URLContext *h;
	int err;

	err = url_open(&h, filename, flags);
	if (err < 0)
		return err;
	err = url_fdopen(s, h);
	if (err < 0) {
		url_close(h);
		return err;
	}
	return 0;
}

int url_fclose(ByteIOContext *s)
{
	URLContext *h = (URLContext *)s->opaque;

	av_free(s->buffer);
	memset(s, 0, sizeof(ByteIOContext));
	return url_close(h);
}

URLContext *url_fileno(ByteIOContext *s)
{
	return (URLContext *)s->opaque;
}

/* XXX: currently size is limited */
int url_fprintf(ByteIOContext *s, const char *fmt, ...)
{
	va_list ap;
	char buf[4096];
	int ret;

	va_start(ap, fmt);
	ret = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	put_buffer(s, (const unsigned char *)buf, strlen(buf));
	return ret;
}

/* note: unlike fgets, the EOL character is not returned and a whole
   line is parsed. return NULL if first char read was EOF */
char *url_fgets(ByteIOContext *s, char *buf, int buf_size)
{
	int c;
	char *q;

	c = url_fgetc(s);
	if (c == EOF)
		return NULL;
	q = buf;
	for(;;)
	{
		if (c == EOF || c == '\n')
			break;
		if ((q - buf) < buf_size - 1)
			*q++ = c;
		c = url_fgetc(s);
	}
	if (buf_size > 0)
		*q = '\0';
	return buf;
}

/*
 * Return the maximum packet size associated to packetized buffered file
 * handle. If the file is not packetized (stream like http or file on
 * disk), then 0 is returned.
 *
 * @param h buffered file handle
 * @return maximum packet size in bytes
 */
int url_fget_max_packet_size(ByteIOContext *s)
{
	return s->max_packet_size;
}

/* buffer handling */
int url_open_buf(ByteIOContext *s, uint8_t *buf, int buf_size, int flags)
{
	return init_put_byte(s, buf, buf_size, (flags & URL_WRONLY || flags & URL_RDWR), NULL, NULL, NULL, NULL);
}

/* return the written or read size */
int url_close_buf(ByteIOContext *s)
{
	put_flush_packet(s);
	return s->buf_ptr - s->buffer;
}
