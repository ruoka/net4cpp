.SUFFIXES:
.SUFFIXES: .cpp .hpp .o .a .c++m
.DEFAULT_GOAL := all

PROJECT := $(lastword $(notdir $(CURDIR)))

ifeq ($(MAKELEVEL),0)

OS := $(shell uname -s)

ifeq ($(OS),Linux)
CC := /usr/lib/llvm-18/bin/clang
CXX := /usr/lib/llvm-18/bin/clang++
CXXFLAGS = -pthread -I/usr/local/include
LDFLAGS = -L/usr/local/lib
CXXFLAGS += -std=c++23
endif

ifeq ($(OS),Darwin)
CC := /Library/Developer/CommandLineTools/usr/bin/clang
CXX := /Library/Developer/CommandLineTools/usr/bin/clang++
CXXFLAGS = -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
CXXFLAGS += -std=c++2b
endif

CXXFLAGS += -stdlib=libc++ -Wall -Wextra

endif #($(MAKELEVEL),0)

CXXFLAGS += -MMD -I$(SRCDIR)
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

ifeq ($(basename $(basename $(shell $(CXX) -dumpversion))),18) # FIXME as we need greater than equal to 17 etc. 

MODULES = $(SRCDIR)/net.c++m

PCMS = $(MODULES:$(SRCDIR)/%.c++m=$(PCMDIR)/%.pcm)

OBJECTS += $(MODULES:$(SRCDIR)/%.c++m=$(OBJDIR)/%.o)

$(PCMDIR)/%.pcm: $(SRCDIR)/%.c++m
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $< --precompile -c -o $@

$(OBJDIR)/%.o: $(PCMDIR)/%.pcm
	@mkdir -p $(@D)
	$(CXX) $< -c -o $@

endif # Clang 15 and above

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
