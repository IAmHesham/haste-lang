#ifndef HASTE_H_
#define HASTE_H_

#include "containers/arena_allocator.hpp"
#include "utils/intern.hpp"
#include "utils/source_manager.hpp"

namespace haste {

struct CompilationException : std::exception {};

struct Compiler {
	ArenaAllocator arena;
	InternPool pool {};
	SourceFileManager source_manager {};

	Compiler(Allocator &gpa_allocator);

	Allocator &gpa_allocator() const;
	Allocator &arena_allocator() const;
};

void deinit(Compiler &self);

void add_source_file(Compiler &self, StringView path);
void compile(Compiler &self);

}; // namespace haste

#endif
