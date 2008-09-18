#include <stdio.h>
#include <stdlib.h>

#include "SDL_rwnet.h"
#include "../avio/avio.h"


static int urlio_seek(SDL_RWops *context, int offset, int whence)
{
	if ( url_fseek((ByteIOContext *)context->hidden.unknown.data1, offset, whence) == 0 ) {
		return(url_ftell((ByteIOContext*)context->hidden.unknown.data1));
	} else {
		SDL_Error(SDL_EFSEEK);
		return(-1);
	}
}
static int urlio_read(SDL_RWops *context, void *ptr, int size, int maxnum)
{
	size_t nread;

	nread = url_fread((ByteIOContext*)context->hidden.unknown.data1, (unsigned char *)ptr, size * maxnum);
	if ( nread == 0 && url_ferror((ByteIOContext*)context->hidden.unknown.data1) ) {
		SDL_Error(SDL_EFREAD);
	}
	return(nread);
}
static int urlio_write(SDL_RWops *context, const void *ptr, int size, int num)
{
	size_t sizenum = size*num;
	const unsigned char *p = (uint8_t*)ptr;
	ByteIOContext* io = (ByteIOContext*)context->hidden.unknown.data1;

	put_buffer(io, (const unsigned char *)p, sizenum);
	return 0;
//	if ( nwrote == 0 && url_ferror((ByteIOContext*)context->hidden.unknown.data1) ) {
//	SDL_Error(SDL_EFWRITE);
//	}
//	return(nwrote);
}
static int urlio_close(SDL_RWops *context)
{
	if ( context ) {
		url_fclose((ByteIOContext*)context->hidden.unknown.data1);
		free((ByteIOContext*)context->hidden.unknown.data1);
		SDL_FreeRW(context);
	}
	return 0;
}
SDL_RWops * SDLCALL SDL_RWFromUrl(const char *url, int mode)
{
	av_register_all();
	SDL_RWops *rwops = NULL;
	ByteIOContext *io = (ByteIOContext*)malloc(sizeof(ByteIOContext));
	if (url_fopen(io, url, mode) != 0) return NULL;
	rwops = SDL_AllocRW();

	if ( rwops != NULL ) {
		rwops->seek  = urlio_seek;
		rwops->read  = urlio_read;
		rwops->write = urlio_write;
		rwops->close = urlio_close;
		rwops->hidden.unknown.data1 = io;
	}
	return(rwops);
}
