/** Created in: 19/41/2026 12:06
  *
  */
#ifndef SOURCE_MANAGER_H_
#define SOURCE_MANAGER_H_

#include "containers/chunked_dynamic_array.hpp"
#include "containers/dynamic_array.hpp"
#include "containers/string_view.hpp"

namespace haste {

struct InternPool;

enum class SourceFileType {
	Haste,
	Unknown,
};

struct SourceFile {
	StringView filepath;
	StringView content;
	SourceFileType type = SourceFileType::Unknown;
	DynamicArray<StringView> lines;
};

struct SourceFileManager {
	ChunkedDynamicArray<SourceFile> sources;
};

void deinit(SourceFileManager &self, InternPool &pool);
SourceFile *load_file(SourceFileManager &self, InternPool &pool, StringView path);

}; // namespace haste

#endif /* !SOURCE_MANAGER_H_ */
