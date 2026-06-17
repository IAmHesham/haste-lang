#ifndef COMMON_H_
#define COMMON_H_

#include <stddef.h>
#include <stdint.h>

#define SAFE_COUNT(n) ((n) > 0 ? (n) : (size_t)1)
#include <stdarg.h>
#include <stdnoreturn.h>

#ifdef _MSC_VER
#  define Break() __debugbreak()
#elif defined(DEBUG)
#  define Break() __asm__("int3")
#else
#  define Break() do {} while (0)
#endif
#define Exit(...) \
	do { \
		Break(); \
		exit(__VA_ARGS__); \
	} while (0)

#define crash() \
	do { \
		fprintf(stderr, "%s:%d: Error: program crashed.\n", __FILE__, __LINE__);\
		Break(); \
		exit(1); \
	} while (0)

#define frand() ((float)rand() / (float)RAND_MAX)
#define when_fun if (not g_options.disable_fun)
#define run_at_percent(...) if ((not g_options.disable_fun) and (frand() * 100.0) <= ((__VA_ARGS__)))

typedef enum Error {
	OK = 0,
	ERROR = 1,
} Error;

typedef int16_t source_file_id;

#endif
