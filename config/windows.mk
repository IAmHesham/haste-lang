# Windows configuration — auto-detect compiler
# Prefer MSVC (cl.exe), fall back to MinGW GCC
CC  := $(or $(shell command -v cl 2>/dev/null),$(shell command -v gcc 2>/dev/null),$(shell command -v cc 2>/dev/null),gcc)
CXX := $(or $(shell command -v cl 2>/dev/null),$(shell command -v g++ 2>/dev/null),$(shell command -v c++ 2>/dev/null),g++)

ifneq (,$(findstring cl,$(notdir $(CC))))
  # MSVC
  CFLAGS   := /std:c17 /Iinclude/ /I$(shell llvm-config --includedir 2>nul)
  LDFLAGS  := /link libLLVM.lib
  DEBUG_FLAGS    := /Zi /Od /DDEBUG /W4 /WX
  RELEASE_FLAGS  := /O2
  EXE      := haste.exe
else
  # MinGW GCC
  STD := $(shell echo 'int main(){}' | $(CC) -std=c23 -x c - -o nul 2>/dev/null && echo c23 || echo c17)
  CFLAGS   := -std=$(STD) -Iinclude/
  LDFLAGS  := -lstdc++
  DEBUG_FLAGS    := -g -Og -DDEBUG -Wall -Wextra -Wpedantic -Werror -Wno-unused-function
  RELEASE_FLAGS  := -O3
  EXE      := haste.exe
endif
