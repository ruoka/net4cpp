CXX = clang++

CXXFLAGS = -std=c++1y -stdlib=libc++ -I./src/ -I../googletest/include/ -MMD # --analyze

LDFLAGS = -stdlib=libc++ ../googletest/make/gtest_main.a

SRCDIR = src

TESTDIR = test

OBJDIR = obj

BINDIR = bin

SOURCES := $(wildcard $(SRCDIR)/*.cpp) $(wildcard $(SRCDIR)/*/*.cpp)

TESTS := $(wildcard $(TESTDIR)/*.cpp) $(wildcard $(TESTDIR)/*/*.cpp)

OBJECTS := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o) $(TESTS:$(TESTDIR)/%.cpp=$(OBJDIR)/$(TESTDIR)/%.o)

DEPENDENCIES := $(OBJECTS:$(OBJDIR)/%.o=$(OBJDIR)/%.d)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/$(TESTDIR)/%.o: $(TESTDIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BINDIR)/test: $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

all: $(BINDIR)/test

.PHONY: clean
clean:
	@rm -rf $(OBJDIR)
	@rm -rf $(BINDIR)

.PHONY: test
test: $(BINDIR)/test
	$(BINDIR)/test --gtest_filter=-*.CommandLine

.PHONY: dump
dump:
	@echo $(SOURCES)
	@echo $(TESTS)
	@echo $(OBJECTS)
	@echo $(DEPENDENCIES)
	
-include $(DEPENDENCIES)
