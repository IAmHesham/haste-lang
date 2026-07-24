#ifndef LOCATION_HPP_
#define LOCATION_HPP_

#include "utils/source_manager.hpp"
#include <cstdint>

namespace haste {

struct Location {
	SourceFile *src;
	std::uint32_t line, column;
	std::uint32_t len;
};

Location conjoin(Location a, Location b);

StringView get_line(const Location &location);
StringView as_string(const Location &location);

}; // namespace haste

#endif // !LOCATION_HPP_
