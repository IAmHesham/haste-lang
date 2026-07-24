#include "common.hpp"
#include "containers/allocator.hpp"
#include "containers/cpp_allocator.hpp"
#include "haste.hpp"

int main(int, char *[])
{
	auto c_allocator = Mallocator::get_instance();
	set_default_allocator(&c_allocator);

	auto compiler = haste::Compiler(c_allocator);
	defer { deinit(compiler); };

	haste::add_source_file(compiler, "./main.haste");
	try {
		haste::compile(compiler);
	} catch (...) {
		return 1;
	}

	return 0;
}
