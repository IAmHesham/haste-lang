/** Created in: 04/00/2026 16:06
  *
  */
#ifndef DYNAMIC_MEMORY_STREAM_H_
#define DYNAMIC_MEMORY_STREAM_H_

#include "my_stream.h"
#include "my_allocator.h"

stream_t sdynmemopen(struct Allocator allocator, char **out);

#endif /* !DYNAMIC_MEMORY_STREAM_H_ */
