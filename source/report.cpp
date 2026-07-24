#include "report.hpp"
#include "termcolor.hpp"
#include <iomanip>

namespace haste {

Reporter _begin_report(Token token)
{
	return _begin_report(token.location);
}

Reporter _begin_report(Location location)
{
	auto result = Reporter {};
	result.location = location;
	return result;
}

static std::size_t utf8_char_len(unsigned char c)
{
	if (c < 0x80) return 1;
	if (c < 0xC0) return 1;
	if (c < 0xE0) return 2;
	if (c < 0xF0) return 3;
	if (c < 0xF8) return 4;
	return 1;
}

static std::size_t column_to_byte_offset(StringView line, std::uint32_t col)
{
	std::size_t off = 0;
	for (std::uint32_t i = 1; i < col && off < line.len; i++) {
		off += utf8_char_len(static_cast<unsigned char>(line.data[off]));
	}
	return off;
}

Reporter &operator>>(Reporter &self, std::ostream &os)
{
	auto location = self.location;
	if (location.line == 0) location.line = 1;
	auto &src = *location.src;
	if (location.line > src.lines.len) location.line = src.lines.len;
	if (location.column == 0) location.column = 1;
	os << ANSI_CODE_UNDERLINE << src.filepath;
	os << ":" << location.line
	   << ":" << location.column << ANSI_CODE_RESET
	   << ": ";

	os << self.kind.str();
	os << ": " << ANSI_CODE_BOLD << self.brief.str();
	os << "\n";

	auto line = get_line(location);
	auto byte_off = column_to_byte_offset(line, location.column);

	auto end = byte_off + location.len;
	if (end > line.len) end = line.len;
	while (end < line.len && (static_cast<unsigned char>(line.data[end]) & 0xC0) == 0x80) {
		end++;
	}

	std::size_t caret_count = 0;
	for (auto i = byte_off; i < end; i++) {
		if ((static_cast<unsigned char>(line.data[i]) & 0xC0) != 0x80) {
			caret_count++;
		}
	}

	auto line_w = std::max<std::size_t>(4, std::to_string(location.line).size());
	os << ANSI_CODE_YELLOW << ANSI_CODE_BOLD << std::setw(line_w) 
	   << location.line << ANSI_CODE_RESET << " | "
	   << slice(line, 0, byte_off);
	os << ANSI_CODE_RED << ANSI_CODE_BOLD << slice(line, byte_off, end) << ANSI_CODE_RESET;
	os << slice(line, end, line.len) << "\n";
	os << std::setw(line_w) << "" << " | "
	   << std::string(location.column - 1, ' ')
	   << ANSI_CODE_RED << std::string(caret_count, '^') << ANSI_CODE_RESET
	   << " " << self.description.str();

	os << "\n";

	return self;
}

Reporter &operator<<(Reporter &self, ReportKind k)
{
	if (self.current_stream) {
		*self.current_stream << ANSI_CODE_RESET;
	}
	self.current_stream = &self.kind;
	*self.current_stream << k.s;
	return self;
}

Reporter &operator<<(Reporter &self, ReportBrief)
{
	if (self.current_stream) {
		*self.current_stream << ANSI_CODE_RESET;
	}
	self.current_stream = &self.brief;
	return self;
}

Reporter &operator<<(Reporter &self, ReportDescription)
{
	if (self.current_stream) {
		*self.current_stream << ANSI_CODE_RESET;
	}
	self.current_stream = &self.description;
	return self;
}

Reporter &Reporter::ref()
{
	return *this;
}

ReportKind::ReportKind(std::string s): s(s) {}

}; // namespace haste
