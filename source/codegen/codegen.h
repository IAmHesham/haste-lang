#ifndef CODEGEN_H_
#define CODEGEN_H_

#include "common.h"
#include "my_allocator.h"

Error codegen(
    struct Allocator allocator,
    const source_file_id src,
    const char *output_path,
    bool dump_to_stderr,
    FILE *output_file);

#endif
