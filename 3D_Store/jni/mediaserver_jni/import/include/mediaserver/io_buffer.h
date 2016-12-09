#ifndef IO_BUFFER_H_
#define IO_BUFFER_H_

#include <sys/types.h>
#include "utils.h"
#include <stdio.h>
typedef struct io_buffer
{
  void    *data;
  uint64  total_size;

  uint64  pos;
  uint64  size;

} io_buffer;

io_buffer *io_buffer_alloc(uint64 size);
void io_buffer_free(io_buffer *b);
void io_buffer_reset(io_buffer *b);
uint64 io_buffer_write(io_buffer *b, void *data, uint64 size);
uint64 io_buffer_read(io_buffer *b, void *data, uint64 size);

uint64 io_buffer_read_from(io_buffer *b, int fd, uint64 size);
uint64 io_buffer_read_from_vod(io_buffer *b, int task_id, uint64 offset, uint64 size);
uint64 io_buffer_read_from_xv(io_buffer *b, void *xv, uint64 offset, uint64 size);
uint64 io_buffer_write_to(io_buffer *b, int fd, uint64 size);

#define io_buffer_is_empty(b)     ((b)->size == 0)
#define io_buffer_is_full(b)     ((b)->size == (b)->total_size)
#define io_buffer_remain_size(b) ((b)->total_size - (b)->size)

#endif  /* IO_BUFFER_H_ */
