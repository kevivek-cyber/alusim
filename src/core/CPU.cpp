#include "CPU.h"
#include <sstream>

namespace core {

const char* stageName(Stage s) {
    switch (s) {
        case STAGE_FETCH:     return "FETCH";
        case STAGE_DECODE:    return "DECODE";
        case STAGE_EXECUTE:   return "EXECUTE";
        case STAGE_WRITEBACK: return "WRITEBACK";
        default:              return "?";
    }
}

CPU::CPU()
    : callStack_(256),
      history_(0, ds::DEQUE_FULL),
      current_(0),
      stage_(STAGE_FETCH),
      cycleCount_(0),
      instructionCount_(0),
      halted_(false),
      branchTaken_(false),
      aluUsed_(false),
      maxHistory_(512),
      program_(0),
      programSize_(0),
      programBase_(0) {}

CPU::~CPU() {
    clearProgram();
}

void CPU::clearProgram() {
    if (program_) {
        for (unsigned int i = 0; i < programSize_; ++i) delete program_[i];
        delete[] program_;
        program_ = 0;
    }
    programSize_ = 0;
    current_     = 0;
}

void CPU::loadProgram(ds::LinkedList<Instruction*>& program, unsigned int baseAddress) {
    clearProgram();
    programSize_ = static_cast<unsigned int>(program.size());
    programBase_ = baseAddress;
    program_     = new Instruction*[programSize_ ? programSize_ : 1];

    ds::LinkedList<Instruction*>::Iterator it = program.iterator();
    unsigned int i = 0;
    while (it.hasNext()) program_[i++] = it.next();

    // the list no longer owns the pointers -- the CPU does
    program.clear();

    reset();
}

Instruction* CPU::instructionAt(unsigned int address) const {
    if (address < programBase_) return 0;
    unsigned int idx = address - programBase_;
    if (idx >= programSize_) return 0;
    return program_[idx];
}

void CPU::reset() {
    regs_.reset();
    mem_.clear();
    callStack_.clear();
    history_.clear();
    output_.clear();
    alu_.resetCount();

    current_     = 0;
    signals_.clear();
    stage_       = STAGE_FETCH;
    cycleCount_  = 0;
    instructionCount_ = 0;
    halted_      = false;
    branchTaken_ = false;
    aluUsed_     = false;
    aluA_ = aluB_ = aluResult_ = Word(0);

    regs_.pc().forceSet(Word(static_cast<u16>(programBase_)));
}

std::string CPU::currentInstructionText() const {
    if (current_) return current_->toString();
    return "(none)";
}

void CPU::recordCycle() {
    CycleRecord rec;
    rec.cycle           = cycleCount_;
    rec.stage           = stage_;
    rec.pc              = regs_.pc().peek();
    rec.instructionText = currentInstructionText();
    rec.signals         = signals_;
    rec.flags           = regs_.flags();
    rec.aluA            = aluA_;
    rec.aluB            = aluB_;
    rec.aluResult       = aluResult_;
    rec.aluUsed         = aluUsed_;

    history_.pushBack(rec);
    while (history_.size() > maxHistory_) history_.popFront();
}

// ---------------------------------------------------------------------------
// One micro-step of the clock.
//
// FETCH     : PC drives the address, the instruction is loaded into the IR
// DECODE    : the control unit works out the control signals for it
// EXECUTE   : the instruction performs its work (ALU / memory / branch)
// WRITEBACK : PC advances unless a branch already moved it
// ---------------------------------------------------------------------------
void CPU::step() {
    if (halted_) return;

    regs_.clearActivity();
    mem_.clearActivity();
    signals_.clear();
    aluUsed_ = false;

    switch (stage_) {
        case STAGE_FETCH: {
            unsigned int addr = regs_.pc().peek().raw();
            regs_.mar().write(Word(static_cast<u16>(addr)));
            current_ = instructionAt(addr);

            signals_.pcOut   = true;
            signals_.marLoad = true;
            signals_.memRead = true;
            signals_.irLoad  = true;
            signals_.busActive = true;

            if (current_ == 0) {
                // running off the end of the program stops the machine
                halted_ = true;
                signals_.halt = true;
            } else {
                regs_.ir().write(Word(static_cast<u16>(addr)));
            }
            stage_ = STAGE_DECODE;
            break;
        }

        case STAGE_DECODE: {
            if (current_ == 0) { halted_ = true; break; }
            // The control unit asks the instruction for its signal vector.
            signals_ = current_->signals();
            signals_.irLoad = false;      // IR already loaded during fetch
            stage_ = STAGE_EXECUTE;
            break;
        }

        case STAGE_EXECUTE: {
            if (current_ == 0) { halted_ = true; break; }
            branchTaken_ = false;
            signals_ = current_->signals();
            current_->execute(*this);     // <-- polymorphic dispatch
            stage_ = STAGE_WRITEBACK;
            break;
        }

        case STAGE_WRITEBACK: {
            if (current_ != 0 && !halted_ && !branchTaken_) {
                Word pc = regs_.pc().peek();
                regs_.pc().write(pc + Word(1));
                signals_.pcInc = true;
            }
            ++instructionCount_;
            stage_ = STAGE_FETCH;
            break;
        }
    }

    ++cycleCount_;
    recordCycle();
}

void CPU::stepInstruction() {
    if (halted_) return;
    // finish the current instruction, then stop at the start of the next fetch
    do { step(); } while (!halted_ && stage_ != STAGE_FETCH);
}

void CPU::run(unsigned long maxCycles) {
    unsigned long start = cycleCount_;
    while (!halted_ && (cycleCount_ - start) < maxCycles) step();
}

} // namespace core
