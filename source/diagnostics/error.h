#ifndef ERROR_H_
#define ERROR_H_

#include "common.h"
#include "utils/location.h"
#include "lexer/token.h"

void f_report_at(const source_file_id src, const char *kind, const char *start, const char *fmt, ...);
void f_vreport_at(const source_file_id src, const char *kind, const char *start, const char *fmt, va_list args);
void f_report_at_token(const char *kind, struct token token, const char *fmt, ...);
void f_vreport_at_token(const char *kind, struct token token, const char *fmt, va_list args);
void f_report_at_location(const char *kind, struct location location, const char *fmt, ...);
void f_vreport_at_location(const char *kind, struct location location, const char *fmt, va_list args);

#endif
