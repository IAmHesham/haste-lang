#ifndef UNICODE_H_
#define UNICODE_H_

#include "common.h"

int encode_utf8(char *out, uint32_t c);
uint32_t decode_utf8(const char **new_pos, const char *p, source_file_id src);
bool is_ident1(uint32_t c);
bool is_ident2(uint32_t c);
int display_width(const char *p, int len, source_file_id src);

#endif
