// ---------------------------------------------------------------------------
// ConsoleView.h  --  draws the simulator state
//
// The datapath diagram is rasterised into a Canvas using our own Bresenham and
// scan-line code, then the Canvas is printed.  The waveform view is the
// logic-analyzer display: one row per control signal, one column per clock
// cycle, exactly like a real timing diagram.
//
// Covers: CGL Practicals 2/3/5 (the drawing is done with our own algorithms)
//         COAL Practical 2     (control signals made visible)
// ---------------------------------------------------------------------------
#ifndef UI_CONSOLEVIEW_H
#define UI_CONSOLEVIEW_H

#include <ostream>
#include "Canvas.h"
#include "../core/CPU.h"

namespace ui {

class ConsoleView {
private:
    core::CPU& cpu_;
    int        radix_;      // 2, 10 or 16 for the register display

public:
    explicit ConsoleView(core::CPU& cpu) : cpu_(cpu), radix_(16) {}

    void setRadix(int r) { radix_ = r; }
    int  radix() const   { return radix_; }

    std::string formatWord(core::Word w) const;

    // -- the individual panes ------------------------------------------------
    void drawRegisters(std::ostream& os) const;
    void drawDatapath(std::ostream& os) const;      // uses the Canvas
    void drawWaveform(std::ostream& os, int cycles = 32) const;
    void drawMemory(std::ostream& os, int maxRows = 16) const;
    void drawProgram(std::ostream& os, int contextLines = 6) const;
    void drawStack(std::ostream& os) const;
    void drawOutput(std::ostream& os) const;
    void drawStatusLine(std::ostream& os) const;

    // everything at once, the default screen after each step
    void drawAll(std::ostream& os) const;
};

} // namespace ui

#endif // UI_CONSOLEVIEW_H
