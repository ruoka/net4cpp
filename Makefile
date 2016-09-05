CXX = clang++

CXXFLAGS = -I$(SRCDIR) -std=c++1z -stdlib=libc++ -MMD# -D DEBUG=1

LDFLAGS = -stdlib=libc++

SRCDIR = src

TESTDIR = test

OBJDIR = obj

BINDIR = bin

GTESTDIR = ../googletest/googletest

GTESTLIB = $(GTESTDIR)/make/gtest_main.a


#TARGETS = $(addprefix $(BINDIR)/, yyz)

#MAINS	= $(TARGETS:$(BINDIR)/%=$(SRCDIR)/%.cpp)

SOURCES = $(filter-out $(MAINS), $(wildcard $(SRCDIR)/*.cpp $(SRCDIR)/*/*.cpp $(SRCDIR)/*/*/*.cpp))

OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGETS): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(@:$(BINDIR)/%=$(SRCDIR)/%.cpp) $(OBJECTS) -MF $(@:$(BINDIR)/%=$(OBJDIR)/%.d) -o $@


GTEST_TARGET = $(BINDIR)/test

GTEST_SOURCES = $(wildcard $(TESTDIR)/*.cpp $(TESTDIR)/*/*.cpp $(TESTDIR)/*/*/*.cpp $(TESTDIR)/*/*/*/*.cp)

GTEST_OBJECTS = $(GTEST_SOURCES:$(TESTDIR)/%.cpp=$(OBJDIR)/$(TESTDIR)/%.o)

$(OBJDIR)/$(TESTDIR)/%.o: $(TESTDIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) -I$(GTESTDIR)/include/ $(CXXFLAGS) -c $< -o $@

$(GTEST_TARGET): $(OBJECTS) $(GTEST_OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $(OBJECTS) $(GTEST_OBJECTS) $(GTESTLIB) -o $@


DEPENDENCIES = $(MAINS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.d) $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.d) $(GTEST_SOURCES:$(TESTDIR)/%.cpp=$(OBJDIR)/%.d)


all: $(TARGETS) $(GTEST_TARGET)

.PHONY: clean
clean:
	@rm -rf $(OBJDIR)
	@rm -rf $(BINDIR)

.PHONY: test
test: $(GTEST_TARGET)
	$(GTEST_TARGET) --gtest_filter=-*.CommandLine:HttpServer.*

.PHONY: dump
dump:
	@echo $(TARGETS)
	@echo $(MAINS)
	@echo $(SOURCES)
	@echo $(OBJECTS)
	@echo $(GTEST_TARGET)
	@echo $(GTEST_SOURCES)
	@echo $(GTEST_OBJECTS)
	@echo $(DEPENDENCIES)

-include $(DEPENDENCIES)
