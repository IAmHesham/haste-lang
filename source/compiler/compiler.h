#ifndef HASTE_COMPILER_H_
#define HASTE_COMPILER_H_

#include "common.h"
#include "my_allocator.h"
#include "my_arena_allocator.h"
#include "utils/intern.h"
#include "utils/source.h"
#include "my_timing.h"
#include <stdio.h>

struct haste_compiler {
    struct Allocator allocator;
    struct Arena arena;
    struct intern_pool pool;
    source_file_id src;
    struct timer_list timers;
    bool enable_fun;
};

void haste_compiler_set_fun(struct haste_compiler *compiler, bool enable);
Error haste_compiler_add_source(struct haste_compiler *compiler, const char *path);
Error haste_compiler_dump_tokens(struct haste_compiler *compiler, FILE *f);
Error haste_compiler_dump_ast(struct haste_compiler *compiler, FILE *f);
Error haste_compiler_dump_sema(struct haste_compiler *compiler, FILE *f);
Error haste_compiler_dump_c(struct haste_compiler *compiler, FILE *f);
void haste_compiler_dump_measure(struct haste_compiler *compiler, FILE *f);
void haste_compiler_deinit(struct haste_compiler *compiler);

#endif
