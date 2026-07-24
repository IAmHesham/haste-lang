# OS detection
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)

ifeq ($(UNAME_S),Linux)
  include config/linux.mk
else ifeq ($(UNAME_S),FreeBSD)
  include config/freebsd.mk
else ifneq (,$(filter MSYS_NT% MINGW64_NT% MINGW32_NT% CYGWIN_NT%,$(UNAME_S)))
  include config/windows.mk
else
  include config/linux.mk
endif

BUILD_DIR := .build/
SRC_DIR   := source/

ALL_DIRS  := $(SRC_DIR) $(shell find $(SRC_DIR) -mindepth 1 -type d | sort)
VPATH     := $(ALL_DIRS)

SRCS      := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS      := $(addprefix $(BUILD_DIR),$(notdir $(SRCS:.cpp=.o)))

.PHONY: all gen_compile_flags run clean debug release test test-clean

all: debug

debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: CXXFLAGS += -MMD -MP
debug: gen_compile_flags $(EXE)

gen_compile_flags:
	@echo $(CXXFLAGS) $(LDFLAGS) | tr ' ' '\n' > compile_flags.txt

release: CXXFLAGS += $(RELEASE_FLAGS)
release: $(EXE)

$(EXE): $(OBJS)
	@echo "$(CXX) -o $@ $^ (link flags...)"
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

HEADERS := $(shell find $(SRC_DIR) -name '*.hpp') $(shell find include/ -name '*.hpp')
$(OBJS): $(HEADERS)
$(BUILD_DIR)%.o: %.cpp | $(BUILD_DIR)
	@echo "$(CXX) -o $@ $<"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

-include $(OBJS:.o=.d)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(all)
	./$(EXE)

test: $(EXE)
	@cd test && python3 ./run_tests.py

test-brief: $(EXE)
	@cd test && python3 ./run_tests.py --brief

clean-test:
	rm -f test/**/*.got test/**/*.tokens test/**/*.ll test/**/*.json

clean: clean-test
	rm -rdf $(OBJS) $(EXE) $(BUILD_DIR) TAGS
