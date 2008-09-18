#ifndef FIFO_H
#define FIFO_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct FifoBuffer {
	uint8_t *buffer;
	uint8_t *rptr, *wptr, *end;
} FifoBuffer;
#ifdef __cplusplus
extern "C" {
#endif
	
int fifo_init(FifoBuffer *f, int size);
void fifo_free(FifoBuffer *f);
int fifo_size(FifoBuffer *f, uint8_t *rptr);
int fifo_read(FifoBuffer *f, uint8_t *buf, int buf_size, uint8_t **rptr_ptr);
void fifo_write(FifoBuffer *f, uint8_t *buf, int size, uint8_t **wptr_ptr);
void fifo_realloc(FifoBuffer *f, unsigned int size);
#ifdef __cplusplus
}
#endif
	
#endif
