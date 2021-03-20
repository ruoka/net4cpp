CXX = /Library/Developer/CommandLineTools/usr/bin/clang++
#CXX = clang++

CXXFLAGS = -std=c++2a -MMD -Wall -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk # -D DEBUG=1

LDFLAGS =

############

SRCDIR = src

TESTDIR = test

OBJDIR = obj

BINDIR = bin

LIBDIR = lib

INCDIR = include

GTESTDIR = googletest

############

# Make does not offer a recursive wildcard function, so here's one:
rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

############

SOURCES = $(call rwildcard,$(SRCDIR)/,*.cpp)

OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

LIBRARY = $(addprefix $(LIBDIR)/, libnet4cpp.a)

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
	cd $(GTESTDIR) && cmake -DCMAKE_CXX_COMPILER="$(CXX)" -DCMAKE_CXX_FLAGS="$(CXXFLAGS) $(LDFLAGS)" -DCMAKE_INSTALL_PREFIX=.. . && make install

############

TEST_SOURCES = $(call rwildcard,$(TESTDIR)/,*.cpp)

TEST_OBJECTS = $(TEST_SOURCES:$(TESTDIR)/%.cpp=$(OBJDIR)/$(TESTDIR)/%.o)

TEST_TARGET = $(BINDIR)/test

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
all: $(LIBRARY) $(TEST_TARGET)

.PHONY: lib
lib: $(LIBRARY) $(INCLUDES)

.PHONY: test
test: $(TEST_TARGET)
	$(TEST_TARGET) --gtest_filter=-*CommandLine:HttpServerTest*:NetReceiverAndSenderTest*

.PHONY: clean
clean:
	@rm -rf $(OBJDIR)
	@rm -rf $(BINDIR)
	@rm -rf $(LIBDIR)
	@rm -rf $(INCDIR)

.PHONY: dump
dump:
	@echo $(SOURCES)
	@echo $(OBJECTS)
	@echo $(LIBRARY)
	@echo $(HEADERS)
	@echo $(INCLUDES)
	@echo $(TEST_SOURCES)
	@echo $(TEST_OBJECTS)
	@echo $(TEST_TARGET)
	@echo $(GTESTLIBS)
	@echo $(DEPENDENCIES)

-include $(DEPENDENCIES)
