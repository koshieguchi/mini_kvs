# CC = /Library/Developer/CommandLineTools/usr/bin/c++
# CC = /Library/Developer/CommandLineTools/usr/bin/clang++
CC = g++
C_COMPILER = gcc
INCLUDE_DIRS = -Iinclude -Iexternal/xxHash
CPPFLAGS = -std=c++17 -Wall $(INCLUDE_DIRS)
CFLAGS = -Wall $(INCLUDE_DIRS)
OBJDIR = obj
SRCDIR = src
TESTDIR = tests
TESTUTILSDIR = $(TESTDIR)/utils
EXPDIR = exps
BINDIR = bin

CPP_SOURCES := $(wildcard $(SRCDIR)/*.cpp)
C_SOURCES := external/xxHash/xxhash.c
OBJECTS := $(CPP_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o) $(C_SOURCES:external/xxHash/%.c=$(OBJDIR)/%.o)

TESTUTILS := $(wildcard $(TESTUTILSDIR)/*.cpp)
TESTUTILSOBJS := $(TESTUTILS:$(TESTUTILSDIR)/%.cpp=$(OBJDIR)/test_utils/%.o)

TESTS := $(wildcard $(TESTDIR)/*_test.cpp)
TESTBINS := $(TESTS:$(TESTDIR)/%_test.cpp=$(BINDIR)/%_test)

EXPS := $(wildcard $(EXPDIR)/*_exp.cpp)
EXPBINS := $(EXPS:$(EXPDIR)/%_exp.cpp=$(BINDIR)/%_exp)

all: $(TESTBINS)

# compile C source files into object files
$(OBJDIR)/%.o: external/xxHash/%.c
	@mkdir -p $(@D)
	$(C_COMPILER) $(CFLAGS) -c $< -o $@

# compile C++ source files into object files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c $< -o $@

# compile test utils into object files
$(OBJDIR)/test_utils/%.o: $(TESTUTILSDIR)/%.cpp
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -c $< -o $@

# compile test files into binaries
$(BINDIR)/%_test: $(TESTDIR)/%_test.cpp $(OBJECTS) $(TESTUTILSOBJS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $^ -o $@

# compile exp files into binaries
$(BINDIR)/%_exp: $(EXPDIR)/%_exp.cpp $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $^ -o $@

test: $(TESTBINS)
	@for test in $^ ; do \
		echo "=== Running $$test ==="; \
		./$$test; \
	done

exp: $(EXPBINS)
	@for exp in $^ ; do \
		echo "=== Running $$exp ==="; \
		./$$exp; \
	done

clean:
	rm -rf $(OBJDIR) $(BINDIR)
