# Linux configuration — auto-detect compiler
CXX := $(or $(shell command -v clang++ 2>/dev/null),$(shell command -v g++ 2>/dev/null),$(shell command -v c++ 2>/dev/null),clang++)
CC  := $(CXX)

# Detect NixOS glibc-dev path (needed for iostreams/<format> with clang on NixOS)
# GLIBC_DEV := $(shell find /nix/store -maxdepth 1 -name '*-glibc-*-dev' -type d 2>/dev/null | head -1)
ifneq ($(GLIBC_DEV),)
  CXXFLAGS  := -std=c++20 -Iinclude/ -Isource/ # -idirafter $(GLIBC_DEV)/include
else
  CXXFLAGS  := -std=c++20 -Iinclude/ -Isource/
endif
LDFLAGS   :=

DEBUG_FLAGS    := -ggdb -fsanitize=undefined,address -Og -DDEBUG -Wall -Wextra -Wpedantic -Werror -Wno-unused-function
RELEASE_FLAGS  := -O3
EXE            := haste
