#pragma once

#include <iostream>

#define LOG(msg) std::cout << "LOG: " << msg << '\n';
#define UNIMPLEMENTED std::cout << "Unimplemented" << std::endl; exit(1);

