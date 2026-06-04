// #include "haste.h"
#include "dynamic_memory_stream.h"
#include "my_stream.h"
#include "my_allocator.h"

#define DYN_DEFAULT_LEN 1024

struct dynamic_memory_stream {
	struct Allocator allocator;
	char **out;
	size_t len, pos;
};

static int dyn_mem_close(void *data);
static int dyn_mem_read(void *data, unsigned char *out_buf, size_t amount);
static int dyn_mem_write(void *data, const unsigned char *in, size_t m);
static int dyn_mem_seek(void *data, long int offset, int whence);
static int dyn_mem_flush(void *data);

static stream_interface_t dyn_mem_vtable_ = {
	.close = dyn_mem_close,
	.read = dyn_mem_read,
	.write = dyn_mem_write,
	.seek = dyn_mem_seek,
	.flush = dyn_mem_flush,
};

stream_t sdynmemopen(struct Allocator allocator, char **out)
{
	struct dynamic_memory_stream *data = calloc(sizeof(struct dynamic_memory_stream), 1);
	data->out = out;
	*data->out = alloc(allocator, sizeof(char) * DYN_DEFAULT_LEN);
	(*data->out)[0] = '\0';
	data->allocator = allocator;
	data->len = DYN_DEFAULT_LEN;
	data->pos = 0;
	return (stream_t) {
		.data = data,
		.vtable = &dyn_mem_vtable_,
	};
}

static int dyn_mem_close(void *data)
{
	struct dynamic_memory_stream *self = data;
	xdestroy(self->allocator, self->len * sizeof(char), *self->out);
	return 0;
}

static int dyn_mem_read(void *data, unsigned char *out_buf, size_t amount)
{
	(void)data;
	(void)out_buf;
	(void)amount;
	exit(1);
}

static void grow(struct dynamic_memory_stream *self)
{
	const size_t newlen = self->len * 2;
	*self->out = xrecreate(
		self->allocator,
		self->len * sizeof(char),
		newlen * sizeof(char),
		*self->out);
	self->len = newlen;
}

static int dyn_mem_write(void *data, const unsigned char *in, size_t m)
{
	struct dynamic_memory_stream *self = data;

	const size_t remaining = self->len - self->pos;
	if (remaining + 1 < m) {
		grow(self);
	}

	const int write_amount = m;
	memcpy(*self->out + self->pos, in, sizeof(char) * write_amount);
	self->pos += write_amount;
	(*self->out)[self->pos] = '\0';

	return write_amount;
}

static int dyn_mem_seek(void *data, long int offset, int whence)
{
	(void)data;
	(void)offset;
	(void)whence;
	exit(1);
}

static int dyn_mem_flush(void *data)
{
	(void)data;
	return 0;
}
