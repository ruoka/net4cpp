.SUFFIXES:
.SUFFIXES: .cpp .hpp .o .a .c++m .pcm
.PRECIOUS: $(PCMDIR)/%.pcm
.DEFAULT_GOAL := all

#PROJECT := $(lastword $(notdir $(CURDIR)))
PROJECT := net

ifeq ($(MAKELEVEL),0)
# Include shared compiler configuration (relative to project root)
# If used standalone, fall back to inline compiler configuration
-include ../../config/compiler.mk

# If config/compiler.mk wasn't found (standalone mode), define compiler settings inline
OS := $(shell uname -s)

ifeq ($(OS),Linux)
# Use /usr/bin/clang++-20 directly (installed by CI workflow)
# This ensures we use the correct compiler version, not g++ or system clang
# Use override to force these values even if CC/CXX are set in environment
override CC = /usr/bin/clang-20
override CXX = /usr/bin/clang++-20
CXXFLAGS = -pthread -I/usr/lib/llvm-20/include/c++/v1 -I/usr/local/include
LDFLAGS = -L/usr/local/lib
CXXFLAGS += -stdlib=libc++ -Wall -Wextra
endif

ifeq ($(OS),Darwin)
# Use /usr/local/llvm directly (preferred location)
# Use override to force these values even if CC/CXX are set in environment
LLVM_PREFIX := /usr/local/llvm
ifeq ($(wildcard $(LLVM_PREFIX)/bin/clang++),)
$(error LLVM not found at $(LLVM_PREFIX). Please install LLVM at /usr/local/llvm)
endif
override CC = $(LLVM_PREFIX)/bin/clang
override CXX = $(LLVM_PREFIX)/bin/clang++
# Check if LLVM has its own libc++ (Homebrew) or uses system libc++ (/usr/local/llvm)
LLVM_HAS_LIBCXX := $(shell test -d $(LLVM_PREFIX)/include/c++/v1 && echo yes || echo no)
ifeq ($(LLVM_HAS_LIBCXX),yes)
CXXFLAGS = -I$(LLVM_PREFIX)/include/c++/v1
LDFLAGS = -L$(LLVM_PREFIX)/lib/c++ -L$(LLVM_PREFIX)/lib -Wl,-rpath,$(LLVM_PREFIX)/lib/c++ -Wl,-rpath,$(LLVM_PREFIX)/lib -lc++
else
CXXFLAGS =
LDFLAGS =
endif
CXXFLAGS += -stdlib=libc++ -Wall -Wextra
endif

endif #($(MAKELEVEL),0)

CXXFLAGS += -std=c++23 -MMD

############

PREFIX ?= .
SRCDIR = src
TESTDIR = test
OBJDIR = $(PREFIX)/obj
BINDIR = $(PREFIX)/bin
LIBDIR = $(PREFIX)/lib
INCDIR = $(PREFIX)/include
PCMDIR = $(PREFIX)/pcm
GTESTDIR = $(PREFIX)/googletest

CXXFLAGS += -I$(SRCDIR) -I$(INCDIR)

# Set AR and ARFLAGS if not already defined
AR ?= ar
ARFLAGS ?= rcs

############

# Make does not offer a recursive wildcard function, so here's one:
rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

############

# Verify compiler exists and is clang (not g++ or system c++)
CXX_TYPE := $(shell $(CXX) --version 2>/dev/null | head -1 | cut -d' ' -f1)
ifeq ($(CXX_TYPE),g++)
    $(error Detected g++ instead of clang++. Please ensure clang++-20 is installed and use /usr/bin/clang++-20. Current CXX: $(CXX))
endif
# On Darwin, ensure we're using /usr/local/llvm compiler, not system c++
ifeq ($(OS),Darwin)
ifneq ($(findstring /usr/local/llvm,$(CXX)),/usr/local/llvm)
    $(error Wrong compiler detected on Darwin. Expected /usr/local/llvm/bin/clang++, got: $(CXX). Please unset CC and CXX environment variables: unset CC CXX)
endif
endif

# Detect compiler version
CXX_VERSION_RAW := $(shell $(CXX) -dumpversion 2>/dev/null)
CXX_VERSION_MAJOR := $(shell echo $(CXX_VERSION_RAW) | cut -d. -f1)
$(info Using compiler: $(CXX))
$(info CXX version is $(CXX_VERSION_MAJOR))
$(info CXXFLAGS=$(CXXFLAGS))
ifeq ($(shell test -n "$(CXX_VERSION_MAJOR)" && test $(CXX_VERSION_MAJOR) -ge 19 2>/dev/null && echo yes),)
    $(error CXX version is less than 19 (detected: $(CXX_VERSION_MAJOR)). Please use a C++ compiler version >= 19 for C++23 modules support. Compiler: $(CXX))
else

MODULES = $(SRCDIR)/$(PROJECT).c++m

PCMS = $(MODULES:$(SRCDIR)/%.c++m=$(PCMDIR)/%.pcm)

OBJECTS += $(MODULES:$(SRCDIR)/%.c++m=$(OBJDIR)/%.o)

$(PCMDIR)/%.pcm: $(SRCDIR)/%.c++m
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -I$(INCDIR) $< --precompile -o $@

$(OBJDIR)/%.o: $(PCMDIR)/%.pcm
	@mkdir -p $(@D)
	$(CXX) -c $< -o $@

endif

############

SOURCES = $(call rwildcard,$(SRCDIR)/,*.cpp)

OBJECTS += $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

LIBRARY = $(addprefix $(LIBDIR)/, lib$(PROJECT).a)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -c $< -o $@

$(LIBRARY) : $(OBJECTS)
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^

############

HEADERS = $(call rwildcard,$(SRCDIR)/,*.hpp)

INCLUDES = $(HEADERS:$(SRCDIR)/%.hpp=$(INCDIR)/%.hpp)

$(INCDIR)/%.hpp: $(SRCDIR)/%.hpp
	@mkdir -p $(@D)
	cp $< $@

############

GTESTLIBS = $(addprefix $(LIBDIR)/, libgtest.a libgtest_main.a)

$(GTESTLIBS):
	git submodule update --init --recursive --depth 1
	cd $(GTESTDIR) && cmake  -DCMAKE_INSTALL_PREFIX=.. -DCMAKE_C_COMPILER="$(CC)" -DCMAKE_CXX_COMPILER="$(CXX)" -DCMAKE_CXX_FLAGS="$(CXXFLAGS)" . && make install

############

TEST_SOURCES = $(call rwildcard,$(TESTDIR)/,*.cpp)

TEST_OBJECTS = $(TEST_SOURCES:$(TESTDIR)/%.cpp=$(OBJDIR)/$(TESTDIR)/%.o)

TEST_TARGET = $(BINDIR)/test_$(PROJECT)

$(OBJDIR)/$(TESTDIR)/%.o: $(TESTDIR)/%.cpp $(GTESTLIBS) $(INCLUDES)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

$(TEST_TARGET): $(TEST_OBJECTS) $(LIBRARY)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(TEST_OBJECTS) $(LIBRARY) $(GTESTLIBS) -o $@

############

DEPENDENCIES = $(MAINS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.d) $(OBJECTS:%.o=%.d) $(TEST_OBJECTS:%.o=%.d)

############

.PHONY: all
all: lib

.PHONY: lib
lib: $(INCLUDES) $(LIBRARY)

.PHONY: module
module: $(PCMS) $(LIBRARY)

.PHONY: test
test: $(TEST_TARGET)
	$(TEST_TARGET) --gtest_filter=-*CommandLine:HttpServerTest*:NetReceiverAndSenderTest*

.PHONY: clean
clean:
	@rm -rf $(OBJDIR) $(BINDIR) $(LIBDIR) $(INCDIR) $(PCMDIR)

.PHONY: dump
dump:
	$(foreach v, $(sort $(.VARIABLES)), $(if $(filter file,$(origin $(v))), $(info $(v)=$($(v)))))
	@echo ''

-include $(DEPENDENCIES)
