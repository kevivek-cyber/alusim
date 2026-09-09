#include "ConsoleView.h"
#include <sstream>
#include <iomanip>

using namespace core;

namespace ui {

std::string ConsoleView::formatWord(Word w) const {
    if (radix_ == 2)  return w.toBinary();
    if (radix_ == 10) return w.toDecimal();
    return w.toHex();
}

// ---------------------------------------------------------------------------
// Register pane
// ---------------------------------------------------------------------------
void ConsoleView::drawRegisters(std::ostream& os) const {
    const RegisterFile& r = cpu_.registers();

    os << "+-- REGISTERS " << (radix_ == 2 ? "(binary)" : radix_ == 10 ? "(decimal)" : "(hex)   ")
       << " ---------------------------------------------+\n";

    for (int i = 0; i < NUM_GP_REGISTERS; i += 2) {
        os << "| ";
        for (int k = 0; k < 2; ++k) {
            int idx = i + k;
            const Register& reg = r.gp(idx);
            char mark = reg.wasWritten() ? '*' : (reg.wasRead() ? '>' : ' ');
            std::ostringstream cell;
            cell << mark << reg.name() << " = " << formatWord(reg.peek());
            os << std::left << std::setw(radix_ == 2 ? 30 : 16) << cell.str() << " ";
        }
        os << "|\n";
    }

    os << "| ";
    {
        std::ostringstream pc, ir, sp;
        pc << " PC = " << formatWord(r.pc().peek());
        ir << " IR = " << formatWord(r.ir().peek());
        sp << " SP = " << formatWord(r.sp().peek());
        os << std::left << std::setw(radix_ == 2 ? 30 : 16) << pc.str() << " "
           << std::setw(radix_ == 2 ? 30 : 16) << ir.str() << " ";
    }
    os << "|\n";

    os << "| FLAGS " << r.flags().toString()
       << "   (Z=zero C=carry V=overflow N=negative)";
    os << "                    |\n";
    os << "+---------------------------------------------------------------+\n";
}

// ---------------------------------------------------------------------------
// Datapath diagram -- drawn into a Canvas with our own graphics routines
// ---------------------------------------------------------------------------
void ConsoleView::drawDatapath(std::ostream& os) const {
    const int W = 76, H = 21;
    Canvas c(W, H);
    c.clear();

    const ControlSignals& sig = cpu_.signals();
    Pixel busPix = sig.busActive ? PX_ACTIVE : PX_WIRE;

    // ---- component boxes (outlines drawn with Bresenham via drawBox) -------
    c.drawBox(1,  1, 24, 6, "REGISTER FILE");
    c.drawBox(29, 1, 20, 6, "CONTROL UNIT");
    c.drawBox(52, 1, 22, 6, "MEMORY");
    c.drawBox(20, 13, 24, 7, "ALU");

    // ---- labels ------------------------------------------------------------
    {
        const RegisterFile& r = cpu_.registers();
        std::ostringstream l1, l2, l3, l4;
        l1 << "R0=" << r.peekGP(0).toHex();
        l2 << "R1=" << r.peekGP(1).toHex();
        l3 << "R2=" << r.peekGP(2).toHex();
        l4 << "R3=" << r.peekGP(3).toHex();
        c.drawText(3,  2, l1.str());
        c.drawText(14, 2, l2.str());
        c.drawText(3,  3, l3.str());
        c.drawText(14, 3, l4.str());
        c.drawText(3,  4, std::string("PC=") + r.pc().peek().toHex());
        c.drawText(14, 4, std::string("SP=") + r.sp().peek().toHex());

        std::ostringstream st;
        st << "stage " << stageName(cpu_.stage());
        c.drawText(31, 2, st.str());
        c.drawText(31, 3, sig.aluEnable ? std::string("ALUop ") + aluOpName(sig.aluOp)
                                        : std::string("ALUop ----"));
        c.drawText(31, 4, std::string("flags ") + cpu_.registers().flags().toString());

        std::ostringstream m1, m2;
        m1 << "addr " << Word(static_cast<u16>(cpu_.memory().lastAddress())).toHex();
        m2 << (cpu_.memory().lastWasWrite() ? "WRITE" : "read ")
           << " cells " << cpu_.memory().cellsUsed();
        c.drawText(54, 2, m1.str());
        c.drawText(54, 3, m2.str());
    }

    // ---- the shared data bus (a long horizontal wire) ----------------------
    const int busY = 10;
    c.drawLineBresenham(2, busY, W - 3, busY, busPix);
    c.drawText(2, busY - 1, "DATA BUS");

    // ---- vertical drops from each block onto the bus -----------------------
    // Each drop is only "hot" when that block is actually driving the bus this
    // cycle -- this is the "watch the data move" part of the project.
    Pixel regPix = (sig.regRead || sig.regWrite) ? PX_ACTIVE : PX_WIRE;
    Pixel cuPix  = PX_ACTIVE;                                  // CU always drives control
    Pixel memPix = (sig.memRead || sig.memWrite) ? PX_ACTIVE : PX_WIRE;
    Pixel aluPix = sig.aluEnable ? PX_ACTIVE : PX_WIRE;

    c.drawLineBresenham(12, 7,  12, busY - 1, regPix);
    c.drawLineBresenham(39, 7,  39, busY - 1, cuPix);
    c.drawLineBresenham(63, 7,  63, busY - 1, memPix);
    c.drawLineBresenham(31, busY + 1, 31, 12, aluPix);

    // junction dots where each drop meets the bus
    c.setPixel(12, busY, regPix == PX_ACTIVE ? 'O' : 'o');
    c.setPixel(39, busY, 'O');
    c.setPixel(63, busY, memPix == PX_ACTIVE ? 'O' : 'o');
    c.setPixel(31, busY, aluPix == PX_ACTIVE ? 'O' : 'o');

    // ---- ALU internals -----------------------------------------------------
    {
        std::ostringstream a, b, r;
        a << "A = " << cpu_.aluA().toHex();
        b << "B = " << cpu_.aluB().toHex();
        r << "-> " << cpu_.aluResult().toHex();
        c.drawText(22, 14, a.str());
        c.drawText(22, 15, b.str());
        c.drawText(22, 16, sig.aluEnable ? std::string("op ") + aluOpName(sig.aluOp)
                                         : std::string("op idle"));
        c.drawText(22, 17, r.str());
        c.drawText(22, 18, std::string("flags ") + cpu_.registers().flags().toString());
    }

    // ---- flag feedback path back up to the control unit --------------------
    // Drawn through the Cohen-Sutherland clipper so that when the diagram is
    // panned or zoomed later, wires leaving the viewport are clipped properly.
    ClipWindow win(0, 0, W - 1, H - 1);
    Pixel flagPix = sig.flagWrite ? PX_ACTIVE : PX_WIRE;
    c.clipAndDrawLine(44, 16, 47, 16, win, flagPix);
    c.clipAndDrawLine(47, 16, 47, 5,  win, flagPix);
    c.clipAndDrawLine(47, 5,  49, 5,  win, flagPix);
    c.drawText(45, 12, "flags");

    os << "+-- DATAPATH  ('#' = active this cycle, '.' = idle) ------------+\n";
    c.render(os);
    os << "+---------------------------------------------------------------+\n";
}

// ---------------------------------------------------------------------------
// Waveform view -- the logic analyzer
// ---------------------------------------------------------------------------
void ConsoleView::drawWaveform(std::ostream& os, int cycles) const {
    const ds::Deque<CycleRecord>& hist = cpu_.history();
    int total = static_cast<int>(hist.size());
    if (total == 0) {
        os << "+-- WAVEFORM ---------------------------------------------------+\n"
           << "| (no cycles executed yet)                                      |\n"
           << "+---------------------------------------------------------------+\n";
        return;
    }

    int shown = (total < cycles) ? total : cycles;
    int start = total - shown;

    os << "+-- WAVEFORM / LOGIC ANALYZER  (last " << shown << " cycles) ";
    os << "------------------+\n";

    // cycle-number ruler
    os << "          ";
    for (int i = 0; i < shown; ++i) {
        unsigned long cyc = hist.at(start + i).cycle;
        os << static_cast<char>('0' + (cyc % 10));
    }
    os << "\n";

    // one row per control signal
    for (int s = 0; s < ControlSignals::signalCount(); ++s) {
        os << std::left << std::setw(9) << ControlSignals::signalName(s) << " ";
        for (int i = 0; i < shown; ++i) {
            bool now  = hist.at(start + i).signals.signalValue(s);
            bool prev = (i == 0) ? now : hist.at(start + i - 1).signals.signalValue(s);
            if (now && !prev)      os << '/';    // rising edge
            else if (!now && prev) os << '\\';   // falling edge
            else if (now)          os << '-';    // held high
            else                   os << '_';    // held low
        }
        os << "\n";
    }

    // stage track underneath
    os << std::left << std::setw(9) << "stage" << " ";
    for (int i = 0; i < shown; ++i) {
        switch (hist.at(start + i).stage) {
            case STAGE_FETCH:     os << 'F'; break;
            case STAGE_DECODE:    os << 'D'; break;
            case STAGE_EXECUTE:   os << 'E'; break;
            case STAGE_WRITEBACK: os << 'W'; break;
        }
    }
    os << "\n+---------------------------------------------------------------+\n";
}

// ---------------------------------------------------------------------------
// Memory pane -- only the cells actually in use (sparse storage)
// ---------------------------------------------------------------------------
void ConsoleView::drawMemory(std::ostream& os, int maxRows) const {
    ds::LinkedList<unsigned int> used;
    cpu_.memory().usedAddresses(used);

    os << "+-- MEMORY (" << cpu_.memory().cellsUsed() << " cells in use, sparse) "
       << "-----------------------+\n";

    if (used.empty()) {
        os << "| (empty -- nothing has been stored yet)                        |\n";
    } else {
        int shown = 0;
        ds::LinkedList<unsigned int>::Iterator it = used.iterator();
        while (it.hasNext() && shown < maxRows) {
            unsigned int addr = it.next();
            Word v = cpu_.memory().peek(addr);
            os << "|  [" << Word(static_cast<u16>(addr)).toHex() << "] = "
               << std::left << std::setw(28) << formatWord(v) << "            |\n";
            ++shown;
        }
        if (it.hasNext()) os << "|  ... more cells not shown                                     |\n";
    }
    os << "+---------------------------------------------------------------+\n";
}

// ---------------------------------------------------------------------------
// Program listing with the PC marker
// ---------------------------------------------------------------------------
void ConsoleView::drawProgram(std::ostream& os, int contextLines) const {
    unsigned int pc = cpu_.registers().pc().peek().raw();
    unsigned int size = cpu_.programSize();

    os << "+-- PROGRAM ----------------------------------------------------+\n";
    if (size == 0) {
        os << "| (no program loaded)                                           |\n";
    } else {
        int lo = static_cast<int>(pc) - contextLines / 2;
        if (lo < 0) lo = 0;
        int hi = lo + contextLines;
        if (hi > static_cast<int>(size)) { hi = static_cast<int>(size); lo = hi - contextLines; }
        if (lo < 0) lo = 0;

        for (int a = lo; a < hi; ++a) {
            Instruction* ins = cpu_.instructionAt(static_cast<unsigned int>(a));
            if (!ins) continue;
            const char* marker = (static_cast<unsigned int>(a) == pc) ? " >>" : "   ";
            os << "|" << marker << " " << std::setw(4) << a << "  "
               << std::left << std::setw(50) << ins->toString() << " |\n";
        }
    }
    os << "+---------------------------------------------------------------+\n";
}

// ---------------------------------------------------------------------------
// Call stack -- our own Stack<Word>
// ---------------------------------------------------------------------------
void ConsoleView::drawStack(std::ostream& os) const {
    const ds::Stack<Word>& st = cpu_.callStack();
    os << "+-- CALL STACK (depth " << st.size() << ") ";
    os << "----------------------------------+\n";
    if (st.empty()) {
        os << "| (empty)                                                       |\n";
    } else {
        ds::Stack<Word>::ConstIterator it = st.iterator();
        int shown = 0;
        while (it.hasNext() && shown < 8) {
            Word w = it.next();
            os << "|   " << (shown == 0 ? "top -> " : "       ")
               << std::left << std::setw(48) << formatWord(w) << "  |\n";
            ++shown;
        }
    }
    os << "+---------------------------------------------------------------+\n";
}

void ConsoleView::drawOutput(std::ostream& os) const {
    const ds::LinkedList<std::string>& out = cpu_.output();
    os << "+-- OUTPUT -----------------------------------------------------+\n";
    if (out.empty()) {
        os << "| (nothing printed yet -- use OUT Rn)                           |\n";
    } else {
        ds::LinkedList<std::string>::ConstIterator it = out.iterator();
        while (it.hasNext())
            os << "|  " << std::left << std::setw(60) << it.next() << " |\n";
    }
    os << "+---------------------------------------------------------------+\n";
}

void ConsoleView::drawStatusLine(std::ostream& os) const {
    os << "cycle " << cpu_.cycles()
       << " | stage " << stageName(cpu_.stage())
       << " | instr " << cpu_.currentInstructionText()
       << (cpu_.halted() ? "  [HALTED]" : "")
       << "\n";
}

void ConsoleView::drawAll(std::ostream& os) const {
    os << "\n";
    drawStatusLine(os);
    drawRegisters(os);
    drawDatapath(os);
    drawWaveform(os);
    drawProgram(os);
}

} // namespace ui
