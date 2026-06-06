# Bibliotheque cfg : parser de configuration C++98, zero dependance.

# `=` (et non `?=`) : Make predefinit CXX=g++ avec une origine `default`, et `?=`
# n'ecrase pas une variable deja definie -> `c++` serait ignore. `=` ecrase ce
# defaut tout en laissant la ligne de commande gagner (ex. `make CXX=clang++`).
CXX      = c++
CXXFLAGS = -std=c++98 -Wall -Wextra -pedantic
INCLUDES  = -Iinc -Isrc

LIB       = libcfg.a
SRCDIR    = src
OBJDIR    = obj

SRCS      = $(SRCDIR)/lexer.cpp $(SRCDIR)/parser.cpp $(SRCDIR)/errors.cpp $(SRCDIR)/helpers.cpp
OBJS      = $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

TEST_SRC  = tests/test_cfg.cpp
TEST_BIN  = test_cfg
# g++ 13+ produit un faux positif -Wdangling-reference sur l'idiome
#   const Statement& b = getBlock(root, "name");
# (la reference vise `root`, jamais le litteral temporaire). On le neutralise
# pour la cible de test uniquement ; la bibliotheque compile sans aucun warning.
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
