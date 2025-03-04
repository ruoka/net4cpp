.SUFFIXES:
.SUFFIXES: .cpp .hpp .o .a .c++m .pcm
.DEFAULT_GOAL := all

#PROJECT := $(lastword $(notdir $(CURDIR)))
PROJECT := net

ifeq ($(MAKELEVEL),0)

OS := $(shell uname -s)

ifeq ($(OS),Linux)
CC := /usr/lib/llvm-19/bin/clang
CXX := /usr/lib/llvm-19/bin/clang++
CXXFLAGS = -pthread -I/usr/local/include
LDFLAGS = -L/usr/local/lib
endif

ifeq ($(OS),Darwin)
CC = /opt/homebrew/opt/llvm/bin/clang
CXX = /opt/homebrew/opt/llvm/bin/clang++
CXXFLAGS =-I/opt/homebrew/opt/llvm/include/c++/v1
LDFLAGS = -L/opt/homebrew/opt/llvm/lib/c++
endif

ifeq ($(OS),Github)
CC = /usr/local/opt/llvm/bin/clang
CXX = /usr/local/opt/llvm/bin/clang++
CXXFLAGS = -I/usr/local/opt/llvm/include/ -I/usr/local/opt/llvm/include/c++/v1
LDFLAGS = -L/usr/local/opt/llvm/lib/c++ -Wl,-rpath,/usr/local/opt/llvm/lib/c++
endif

CXXFLAGS += -stdlib=libc++ -Wall -Wextra

endif #($(MAKELEVEL),0)

CXXFLAGS += -std=c++23 -MMD -I$(SRCDIR)
LDFLAGS +=

############

PREFIX = .
SRCDIR = src
OBJDIR = obj
TESTDIR = test
BINDIR =$(PREFIX)/bin
LIBDIR = $(PREFIX)/lib
INCDIR = $(PREFIX)/include
PCMDIR = $(PREFIX)/pcm
GTESTDIR = $(PREFIX)/googletest

############

# Make does not offer a recursive wildcard function, so here's one:
rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

############

CXX_VERSION := $(basename $(basename $(shell $(CXX) -dumpversion)))
$(info CXX version is $(CXX_VERSION))
ifeq ($(filter 17 18 19 20 21,$(CXX_VERSION)),)
    $(error CXX version is less than 17. Please use a CXX version >= 17)
else

MODULES = $(SRCDIR)/$(PROJECT).c++m

PCMS = $(MODULES:$(SRCDIR)/%.c++m=$(PCMDIR)/%.pcm)

OBJECTS += $(MODULES:$(SRCDIR)/%.c++m=$(OBJDIR)/%.o)

$(PCMDIR)/%.pcm: $(SRCDIR)/%.c++m
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $< --precompile -o $@

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
