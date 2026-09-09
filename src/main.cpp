// ---------------------------------------------------------------------------
// Virtual Hardware Logic Analyzer & ALU Simulator
//
// MIT Academy of Engineering, Alandi, Pune
// School of Computer Engineering -- Software Engineering
//
// Team : Vivek Dhamale, Pranjal Bhandari, Sanika Patil, Mrudula Bongulwar
// Guide: Mrs. Jaya S. Mane
//
// Phase 1 -- core simulation engine with a console front end.
// ---------------------------------------------------------------------------
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cctype>

#include "core/CPU.h"
#include "core/ALU.h"
#include "asm/Assembler.h"
#include "ui/ConsoleView.h"

using namespace core;

namespace {

std::string trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::string upper(const std::string& s) {
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i)
        r[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(r[i])));
    return r;
}

void printBanner() {
    std::cout <<
    "\n"
    "===============================================================\n"
    "   VIRTUAL HARDWARE LOGIC ANALYZER  &  ALU SIMULATOR\n"
    "   MIT Academy of Engineering -- School of Computer Engineering\n"
    "===============================================================\n"
    "   Type  help  for the command list, or  demo  to load a sample\n"
    "\n";
}

void printHelp() {
    std::cout <<
    "\n"
    "  PROGRAM\n"
    "    load <file>      assemble a .asm file and load it\n"
    "    demo             load the built-in demo program\n"
    "    listing          show the assembled program\n"
    "    symbols          show the assembler symbol table (hash table)\n"
    "\n"
    "  EXECUTION\n"
    "    step   / s       advance one micro-step (fetch/decode/execute/writeback)\n"
    "    instr  / i       advance one whole instruction\n"
    "    run    / r       run until HLT\n"
    "    reset            reset the machine\n"
    "\n"
    "  VIEWS\n"
    "    regs             register pane\n"
    "    data             datapath diagram\n"
    "    wave [n]         waveform / logic analyzer (last n cycles)\n"
    "    mem              memory pane (sparse cells in use)\n"
    "    stack            call stack\n"
    "    out              program output\n"
    "    all              everything at once\n"
    "    radix <2|10|16>  number base for the register display\n"
    "\n"
    "  TOOLS\n"
    "    mul <a> <b>      add-and-shift multiply, with the full trace\n"
    "    muladd <a> <b>   successive-addition multiply, with the trace\n"
    "    alu <op> <a> <b> run one ALU operation and show the flags\n"
    "    gfx              graphics self-test (Bresenham, fill, clipping)\n"
    "\n"
    "    help             this list\n"
    "    quit / exit      leave the simulator\n"
    "\n";
}

const char* kDemoProgram =
    "; -------------------------------------------------------------\n"
    "; Demo: sum the numbers 1..5, store the total, then multiply it\n"
    "; by 3 using the add-and-shift multiplier.\n"
    "; -------------------------------------------------------------\n"
    "        LOAD  R0, #0        ; running total\n"
    "        LOAD  R1, #1        ; counter\n"
    "        LOAD  R2, #6        ; loop limit (stop when counter == 6)\n"
    "loop:   ADD   R0, R1        ; total = total + counter\n"
    "        INC   R1            ; counter = counter + 1\n"
    "        MOV   R3, R1\n"
    "        CMP   R3, R2        ; counter == limit?\n"
    "        JNZ   loop          ; not yet -- go round again\n"
    "        STORE R0, [0x40]    ; keep the total in memory\n"
    "        OUT   R0            ; print the total (should be 15)\n"
    "        LOAD  R4, #3\n"
    "        MUL   R0, R4        ; total * 3 via add-and-shift\n"
    "        OUT   R0            ; print it (should be 45)\n"
    "        HLT\n";

bool loadSource(CPU& cpu, asmb::Assembler& as, const std::string& source) {
    ds::LinkedList<Instruction*> program;
    if (!as.assemble(source, program)) {
        std::cout << "\n  Assembly failed:\n";
        ds::LinkedList<std::string>::ConstIterator it = as.errors().iterator();
        while (it.hasNext()) std::cout << "    " << it.next() << "\n";
        std::cout << "\n";
        return false;
    }
    size_t n = program.size();
    cpu.loadProgram(program);
    std::cout << "  Loaded " << n << " instruction(s). PC reset to 0.\n";
    return true;
}

void showListing(const asmb::Assembler& as) {
    std::cout << "\n  ADDR  INSTRUCTION\n  ----  -----------\n";
    ds::LinkedList<std::string>::ConstIterator it = as.listing().iterator();
    while (it.hasNext()) std::cout << "  " << it.next() << "\n";
    std::cout << "\n";
}

// Shows the symbol table the way PL Practical 11 asks: bucket by bucket, so the
// chaining is visible.
void showSymbols(const asmb::Assembler& as) {
    const ds::HashMap<std::string, unsigned int>& sym = as.symbols();
    std::cout << "\n  SYMBOL TABLE  (" << sym.size() << " labels in "
              << sym.buckets() << " buckets, separate chaining, load factor "
              << sym.loadFactor() << ")\n";
    std::cout << "  bucket  label -> address\n  ------  ----------------\n";
    bool any = false;
    for (size_t b = 0; b < sym.buckets(); ++b) {
        size_t depth = sym.bucketDepth(b);
        for (size_t d = 0; d < depth; ++d) {
            std::string k;
            unsigned int v = 0;
            if (sym.bucketEntry(b, d, k, v)) {
                std::cout << "   " << b << (d ? "  (chained) " : "        ")
                          << k << " -> " << v << "\n";
                any = true;
            }
        }
    }
    if (!any) std::cout << "   (no labels used in this program)\n";
    std::cout << "\n";
}

void runAluCommand(CPU& cpu, std::istringstream& args) {
    std::string opName;
    long a = 0, b = 0;
    if (!(args >> opName)) { std::cout << "  usage: alu <op> <a> <b>\n"; return; }
    args >> a >> b;

    opName = upper(opName);
    struct { const char* n; AluOp op; } table[] = {
        {"ADD", ALU_ADD}, {"SUB", ALU_SUB}, {"AND", ALU_AND}, {"OR", ALU_OR},
        {"XOR", ALU_XOR}, {"NOT", ALU_NOT}, {"SHL", ALU_SHL}, {"SHR", ALU_SHR},
        {"CMP", ALU_CMP}, {"INC", ALU_INC}, {"DEC", ALU_DEC}
    };
    for (int i = 0; i < 11; ++i) {
        if (opName == table[i].n) {
            AluResult r = cpu.alu().execute(table[i].op, Word(static_cast<u16>(a)),
                                                          Word(static_cast<u16>(b)));
            std::cout << "\n  " << opName << "  A=" << Word(static_cast<u16>(a)).toHex()
                      << "  B=" << Word(static_cast<u16>(b)).toHex() << "\n"
                      << "  result  " << r.value.toHex()
                      << "  (" << r.value.raw() << " unsigned, "
                      << r.value.signedVal() << " signed)\n"
                      << "  binary  " << r.value.toBinary() << "\n"
                      << "  flags   " << r.flags.toString()
                      << "   Z=" << r.flags.zero << " C=" << r.flags.carry
                      << " V=" << r.flags.overflow << " N=" << r.flags.negative << "\n\n";
            return;
        }
    }
    std::cout << "  unknown ALU op '" << opName << "'\n";
}

// A quick visual proof that the graphics routines work -- this is the
// Computer Graphics practical content running inside the simulator.
void graphicsSelfTest() {
    ui::Canvas c(70, 22);
    c.clear();

    // Bresenham lines radiating from a point
    for (int i = 0; i <= 8; ++i)
        c.drawLineBresenham(5, 3, 5 + i * 3, 10, '.');

    // a DDA line for comparison
    c.drawLineDDA(5, 12, 32, 19, '=');
    c.drawText(34, 19, "<- DDA line");

    // Bresenham circle
    c.drawCircleBresenham(50, 8, 6, '*');
    c.drawText(44, 16, "Bresenham circle");

    // scan-line filled triangle
    ui::Point tri[3];
    tri[0] = ui::Point(10, 13);
    tri[1] = ui::Point(24, 13);
    tri[2] = ui::Point(17, 20);
    c.scanlineFillPolygon(tri, 3, ':');
    c.drawText(10, 21, "scanline-filled polygon");

    // a clipped line: drawn from outside the window, clipped to the box
    ui::ClipWindow win(40, 17, 66, 21);
    c.drawRect(40, 17, 27, 5, '+');
    bool visible = c.clipAndDrawLine(20, 15, 80, 24, win, '#');
    c.drawText(42, 18, visible ? "Cohen-Sutherland: clipped" : "fully rejected");

    std::cout << "\n  GRAPHICS SELF-TEST -- all drawn with our own algorithms\n";
    std::cout << "  (Bresenham line + circle, DDA line, scanline fill, clipping)\n\n";
    c.render(std::cout);
    std::cout << "\n";
}

} // namespace

int main(int argc, char** argv) {
    CPU            cpu;
    asmb::Assembler assembler;
    ui::ConsoleView view(cpu);

    printBanner();

    // A file given on the command line is loaded straight away.
    if (argc > 1) {
        std::ifstream in(argv[1]);
        if (in) {
            std::ostringstream ss;
            ss << in.rdbuf();
            loadSource(cpu, assembler, ss.str());
        } else {
            std::cout << "  could not open " << argv[1] << "\n";
        }
    }

    std::string line;
    for (;;) {
        std::cout << "sim> ";
        if (!std::getline(std::cin, line)) break;

        line = trim(line);
        if (line.empty()) continue;

        std::istringstream in(line);
        std::string cmd;
        in >> cmd;
        cmd = upper(cmd);

        try {
            if (cmd == "QUIT" || cmd == "EXIT" || cmd == "Q") {
                std::cout << "  bye.\n";
                break;
            }
            else if (cmd == "HELP" || cmd == "?") {
                printHelp();
            }
            else if (cmd == "DEMO") {
                loadSource(cpu, assembler, kDemoProgram);
            }
            else if (cmd == "LOAD") {
                std::string path;
                in >> path;
                if (path.empty()) { std::cout << "  usage: load <file>\n"; continue; }
                std::ifstream f(path.c_str());
                if (!f) { std::cout << "  could not open " << path << "\n"; continue; }
                std::ostringstream ss;
                ss << f.rdbuf();
                loadSource(cpu, assembler, ss.str());
            }
            else if (cmd == "LISTING")  { showListing(assembler); }
            else if (cmd == "SYMBOLS")  { showSymbols(assembler); }
            else if (cmd == "STEP" || cmd == "S") {
                cpu.step();
                view.drawStatusLine(std::cout);
                view.drawRegisters(std::cout);
                view.drawDatapath(std::cout);
            }
            else if (cmd == "INSTR" || cmd == "I") {
                cpu.stepInstruction();
                view.drawStatusLine(std::cout);
                view.drawRegisters(std::cout);
                view.drawProgram(std::cout);
            }
            else if (cmd == "RUN" || cmd == "R") {
                cpu.run();
                std::cout << "  ran to completion in " << cpu.cycles() << " cycles.\n";
                view.drawRegisters(std::cout);
                view.drawOutput(std::cout);
            }
            else if (cmd == "RESET") {
                cpu.reset();
                std::cout << "  machine reset.\n";
            }
            else if (cmd == "REGS")  { view.drawRegisters(std::cout); }
            else if (cmd == "DATA")  { view.drawDatapath(std::cout); }
            else if (cmd == "WAVE")  {
                int n = 32;
                if (in >> n) {} else n = 32;
                view.drawWaveform(std::cout, n);
            }
            else if (cmd == "MEM")   { view.drawMemory(std::cout); }
            else if (cmd == "STACK") { view.drawStack(std::cout); }
            else if (cmd == "OUT")   { view.drawOutput(std::cout); }
            else if (cmd == "ALL")   { view.drawAll(std::cout); }
            else if (cmd == "RADIX") {
                int r = 16;
                in >> r;
                if (r == 2 || r == 10 || r == 16) {
                    view.setRadix(r);
                    std::cout << "  radix set to " << r << "\n";
                } else {
                    std::cout << "  radix must be 2, 10 or 16\n";
                }
            }
            else if (cmd == "MUL" || cmd == "MULADD") {
                long a = 0, b = 0;
                in >> a >> b;
                std::string trace;
                Word r = (cmd == "MUL")
                    ? ALU::multiplyAddShift(Word(static_cast<u16>(a)), Word(static_cast<u16>(b)), &trace)
                    : ALU::multiplySuccessiveAdd(Word(static_cast<u16>(a)), Word(static_cast<u16>(b)), &trace);
                std::cout << "\n" << trace << "\n  product = " << r.raw()
                          << "  (" << r.toHex() << ")\n\n";
            }
            else if (cmd == "ALU") { runAluCommand(cpu, in); }
            else if (cmd == "GFX") { graphicsSelfTest(); }
            else {
                std::cout << "  unknown command '" << cmd << "' -- try  help\n";
            }
        }
        // PSOOP Practical 8: every simulator error surfaces here as our own
        // exception type, with the kind() telling you which one it was.
        catch (const SimulatorException& e) {
            std::cout << "  [" << e.kind() << "] " << e.what() << "\n";
        }
        catch (const std::exception& e) {
            std::cout << "  [std::exception] " << e.what() << "\n";
        }
    }

    return 0;
}
