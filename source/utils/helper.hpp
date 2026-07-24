/** Created in: 23/50/2026 15:06
  *
  */
#ifndef HELPER_H_
#define HELPER_H_

#include "containers/string_view.hpp"
#include <string>

std::string quoted(const StringView input);
std::string quoted(const std::string input);

std::string shorten(const StringView input);
std::string shorten(const std::string input);

bool has_decimal(double num);

#endif /* !HELPER_H_ */
