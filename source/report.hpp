/** Created in: 20/52/2026 14:06
  *
  */
#ifndef REPORT_H_
#define REPORT_H_

#include "token/token.hpp"
#include "utils/location.hpp"
#include "termcolor.hpp"
#include <iostream>
#include <sstream>
#include <string>

namespace haste {

constexpr const char *const Error   = ANSI_CODE_RED    ANSI_CODE_BOLD "Error";
constexpr const char *const Warning = ANSI_CODE_YELLOW ANSI_CODE_BOLD "Warning";
constexpr const char *const Info    = ANSI_CODE_BLUE   ANSI_CODE_BOLD "Info";

struct Reporter {
	Location location;
	std::stringstream kind;
	std::stringstream brief;
	std::stringstream description;

	std::stringstream *current_stream = nullptr;

	Reporter &ref();
};

#define begin_report(...) \
	_begin_report(__VA_ARGS__).ref()

#define empty_report() _begin_report(Location{}).ref()

Reporter _begin_report(Token token);
Reporter _begin_report(Location location);

Reporter &operator>>(Reporter &self, std::ostream &os);

struct ReportKind {
	std::string s;

	ReportKind(std::string s = "");
};

struct ReportBrief {};
struct ReportDescription {};

Reporter &operator<<(Reporter &self, ReportKind);
Reporter &operator<<(Reporter &self, ReportBrief);
Reporter &operator<<(Reporter &self, ReportDescription);

template <typename T>
Reporter &operator<<(Reporter &self, T anything)
{
	if (self.current_stream == nullptr) {
		throw;
	}
	*self.current_stream << anything;
	return self;
}

}; // namespace haste

#endif /* !REPORT_H_ */
