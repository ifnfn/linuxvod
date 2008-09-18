#include "fifo.h"
#include "avio.h"

/* fifo handling */

int fifo_init(FifoBuffer *f, int size)
{
	f->buffer = (uint8_t *)av_malloc(size);
	if (!f->buffer)
		return -1;
	f->end = f->buffer + size;
	f->wptr = f->rptr = f->buffer;
	return 0;
}

void fifo_free(FifoBuffer *f)
{
	av_free(f->buffer);
}

int fifo_size(FifoBuffer *f, uint8_t *rptr)
{
	int size;

	if(!rptr)
		rptr= f->rptr;

	if (f->wptr >= rptr) {
		size = f->wptr - rptr;
	} else {
		size = (f->end - rptr) + (f->wptr - f->buffer);
	}
	return size;
}

/**
 * Get data from the fifo (returns -1 if not enough data).
 */
int fifo_read(FifoBuffer *f, uint8_t *buf, int buf_size, uint8_t **rptr_ptr)
{
	uint8_t *rptr;
	int size, len;

	if(!rptr_ptr)
		rptr_ptr= &f->rptr;
	rptr = *rptr_ptr;

	if (f->wptr >= rptr) {
		size = f->wptr - rptr;
	} else {
		size = (f->end - rptr) + (f->wptr - f->buffer);
	}

	if (size < buf_size)
		return -1;
	while (buf_size > 0)
	{
		len = f->end - rptr;
		if (len > buf_size)
			len = buf_size;
		memcpy(buf, rptr, len);
		buf += len;
		rptr += len;
		if (rptr >= f->end)
			rptr = f->buffer;
		buf_size -= len;
	}
	*rptr_ptr = rptr;
	return 0;
}

/**
 * Resizes a FIFO.
 */
void fifo_realloc(FifoBuffer *f, unsigned int new_size)
{
	unsigned int old_size= f->end - f->buffer;

	if(old_size < new_size){
		uint8_t *old= f->buffer;

		f->buffer= (uint8_t *)av_realloc(f->buffer, new_size);

		f->rptr += f->buffer - old;
		f->wptr += f->buffer - old;

		if(f->wptr < f->rptr){
			memmove(f->rptr + new_size - old_size, f->rptr, f->buffer + old_size - f->rptr);
			f->rptr += new_size - old_size;
		}
		f->end= f->buffer + new_size;
	}
}

void fifo_write(FifoBuffer *f, uint8_t *buf, int size, uint8_t **wptr_ptr)
{
	int len;
	uint8_t *wptr;

	if(!wptr_ptr)
		wptr_ptr= &f->wptr;
	wptr = *wptr_ptr;

	while (size > 0)
	{
		len = f->end - wptr;
		if (len > size)
			len = size;
		memcpy(wptr, buf, len);
		wptr += len;
		if (wptr >= f->end)
			wptr = f->buffer;
		buf += len;
		size -= len;
	}
	*wptr_ptr = wptr;
}

