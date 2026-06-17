#ifndef PARSE_H_
#define PARSE_H_

#include "common.h"
#include "utils/intern.h"

Error parse(struct intern_pool *pool, const source_file_id src);

#endif
