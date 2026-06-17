# cfg library: C++98 configuration parser, no dependencies.

# `=` (not `?=`): Make predefines CXX=g++ with origin `default`, and `?=` does
# not override an already-defined variable -> `c++` would be ignored. `=`
# overrides that default while still letting the command line win
# (e.g. `make CXX=clang++`).
CXX      = c++
# -MMD -MP: emit a .d per object listing the headers it includes (-MP adds
# phony targets for those headers so removing one doesn't break the build).
# Without this, editing a header (lexer.hpp, cfg_helpers.hpp, ...) leaves the
# .o untouched and ships a stale libcfg.a.
CXXFLAGS = -std=c++98 -Wall -Wextra -pedantic -MMD -MP
INCLUDES  = -Iinc -Isrc

LIB       = libcfg.a
SRCDIR    = src
OBJDIR    = obj

SRCS      = $(SRCDIR)/lexer.cpp $(SRCDIR)/parser.cpp $(SRCDIR)/errors.cpp $(SRCDIR)/helpers.cpp
OBJS      = $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

TEST_SRC  = tests/test_cfg.cpp
TEST_BIN  = test_cfg
# g++ 13+ emits a false-positive -Wdangling-reference on the idiom
#   const Statement& b = getBlock(root, "name");
# (the reference points into `root`, never into the temporary literal). We
# silence it for the test target only; the library compiles without any warning.
TEST_FLAGS = -Wno-dangling-reference

.PHONY: all test clean

all: $(LIB)

$(LIB): $(OBJS)
	ar rcs $@ $(OBJS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

test: $(LIB) $(TEST_SRC)
	$(CXX) $(CXXFLAGS) $(TEST_FLAGS) $(INCLUDES) $(TEST_SRC) $(LIB) -o $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(OBJDIR) $(LIB) $(TEST_BIN)

# Pull in the generated header-dependency rules (absent on the first build, so
# the leading '-' silences the "no such file" warning). The .d files live in
# $(OBJDIR), so `clean` already removes them.
-include $(OBJS:.o=.d)
