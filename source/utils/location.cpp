#include "location.hpp"

namespace haste {

Location conjoin(Location a, Location b)
{
    assert(a.src == b.src);

    auto &src = *a.src;
    auto line_a = src.lines[a.line - 1];
    auto line_b = src.lines[b.line - 1];
    auto content = src.content;

    std::size_t a_byte = (line_a.data - content.data) + (a.column - 1);
    std::size_t b_byte = (line_b.data - content.data) + (b.column - 1) + b.len;

    if (b_byte < a_byte) {
        return conjoin(b, a);
    }

    Location result = a;
    result.len = static_cast<std::uint32_t>(b_byte - a_byte);
    return result;
}

StringView get_line(const Location &location)
{
	return location.src->lines[location.line - 1];
}

StringView as_string(const Location &location)
{
	StringView line = location.src->lines[location.line - 1];
	return StringView(line.data + location.column - 1, location.len);
}

};
