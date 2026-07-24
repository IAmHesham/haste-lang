#include "helper.hpp"
#include <cmath>

std::string quoted(const StringView input)
{
	std::string result {};
	result.append("\"");
	for (auto c : input) {
		switch (c) {
		case '"':  result.append("\\\"");   break;
		case '\\': result.append("\\\\"); break;
		case '\t': result.append("\\t");  break;
		case '\n': result.append("\\n");  break;
		case '\v': result.append("\\v");  break;
		default:
			result.push_back(c);
			break;
		}
	}
	result.append("\"");
	return result;
}

std::string quoted(const std::string input)
{
	return quoted(StringView(input.data(), input.size()));
}

std::string shorten(const StringView input)
{
    if (input.len <= 40) {
        return as_std_string(input);
    }
    auto first = slice(input, 0, 15);
    auto last = slice(input, input.len - 15, input.len);
    return as_std_string(first) + "..." + as_std_string(last);
}

std::string shorten(const std::string input)
{
    return shorten(StringView(input.data(), input.size()));
}

bool has_decimal(double num) {
    double int_part;
    // std::modf returns the fractional part and stores the integer part in int_part
    double frac_part = std::modf(num, &int_part);
    
    // Use an epsilon threshold to avoid tiny precision rounding issues
    double epsilon = 1e-9; 
    return std::abs(frac_part) > epsilon;
}
