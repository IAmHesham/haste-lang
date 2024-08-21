#include "Scanner.hpp"
#include "tokens.hpp"
#include <iostream>
#include <ostream>

int main (int argc, char *argv[]) {
	Scanner::setup_keywords();

	std::string code = ". ; : & && | || ~ ~= < <= <<  > >= >> $hhhh";
	Scanner scanner(code);
	TokenList tokens = scanner.scan();
	for (auto& token : tokens) {
		std::cout << token.to_string() << std::endl;
	}
	return 0;
}
