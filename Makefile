# ---------------------------------------------------------------------------
# Virtual Hardware Logic Analyzer & ALU Simulator
#
#   make          build the simulator  -> vsim.exe
#   make test     build and run the unit tests
#   make clean    remove build artefacts
#   make run      build then start the simulator with the demo program
# ---------------------------------------------------------------------------

CXX      := g++
CXXFLAGS := -std=c++14 -Wall -Wextra -O2
SRCDIR   := src
BUILDDIR := build
TARGET   := vsim.exe
TESTBIN  := vtest.exe

SOURCES := $(SRCDIR)/core/Word.cpp \
           $(SRCDIR)/core/ALU.cpp \
           $(SRCDIR)/core/RegisterFile.cpp \
           $(SRCDIR)/core/Memory.cpp \
           $(SRCDIR)/core/Instruction.cpp \
           $(SRCDIR)/core/CPU.cpp \
           $(SRCDIR)/asm/Assembler.cpp \
           $(SRCDIR)/ui/Canvas.cpp \
           $(SRCDIR)/ui/ConsoleView.cpp

MAIN     := $(SRCDIR)/main.cpp
TESTSRC  := tests/test_all.cpp

OBJECTS  := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SOURCES))
MAINOBJ  := $(BUILDDIR)/main.o

.PHONY: all clean run test dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BUILDDIR)/core $(BUILDDIR)/asm $(BUILDDIR)/ui

$(TARGET): $(OBJECTS) $(MAINOBJ)
	$(CXX) $(OBJECTS) $(MAINOBJ) -o $@
	@echo "built $(TARGET)"

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: dirs $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(TESTSRC) $(OBJECTS) -o $(TESTBIN)
	./$(TESTBIN)

run: all
	./$(TARGET) programs/sum.asm

clean:
	rm -rf $(BUILDDIR) $(TARGET) $(TESTBIN)
	@echo "cleaned"
