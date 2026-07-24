#include "utils/source_manager.hpp"
#include "utils/intern.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>

namespace haste {

void deinit(SourceFileManager &self, InternPool &pool)
{
	for (SourceFile &src : self.sources) {
		deinit(src.lines, *pool.allocator);
	}
	deinit(self.sources, *pool.allocator);
}

static DynamicArray<StringView> obtain_lines(InternPool &pool, StringView content);

SourceFile *load_file(SourceFileManager &self, InternPool &pool, StringView path)
{
	std::ifstream file = std::ifstream(as_std_string(path), std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		throw;
	}

	const std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	if (size < 0) {
		throw;
	}

	const String buffer = alloc<char>(pool.arena, static_cast<std::size_t>(size));
	if (!file.read(buffer.data, size)) {
		throw;
	}

	auto type = SourceFileType::Unknown;
	auto pt = std::filesystem::path(as_std_string(path));
	if (pt.has_extension()) {
		auto extension = pt.extension();
		if (extension == ".haste") {
			type = SourceFileType::Haste;
		}
	}

	const StringView content = intern_no_alloc(pool, buffer);
	const DynamicArray<StringView> lines = obtain_lines(pool, content);
	SourceFile source_file = {
		.filepath = path,
		.content = content,
		.type = type,
		.lines = lines,
	};

	append(self.sources, *pool.allocator, source_file);

	return &self.sources[self.sources.len - 1];
}

static DynamicArray<StringView> obtain_lines(InternPool &pool, StringView content)
{
	DynamicArray<StringView> lines = {};
	StringView current = content;

	while (true) {
		const std::size_t offset = find(current, '\n');
		if (offset == current.len) {
			break;
		}

		append(lines, *pool.allocator, slice(current, 0, offset));
		current = slice(current, offset + 1, current.len);
	}

	if (current.len != 0 and current[0] != '\n') {
		append(lines, *pool.allocator, current);
	}

	return lines;
}

}; // namsespace haste
