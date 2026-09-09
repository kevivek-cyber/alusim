// ---------------------------------------------------------------------------
// CPU.h  --  the datapath: ties registers, ALU, memory and the control unit
//            together and advances them one clock step at a time.
//
// Covers: COAL Practical 1 + 2 (ALU and control unit working as one machine)
//         COA Unit 3          (execution of a complete instruction, pipelining)
//         PL  Practical 6     (the call stack is our own Stack<T>)
//         PL  Practical 8/9   (fetch queue is our CircularQueue, history a Deque)
// ---------------------------------------------------------------------------
#ifndef CORE_CPU_H
#define CORE_CPU_H

#include <string>
#include "Word.h"
#include "ALU.h"
#include "RegisterFile.h"
#include "Memory.h"
#include "Instruction.h"
#include "../ds/Stack.h"
#include "../ds/Deque.h"
#include "../ds/LinkedList.h"

namespace core {

// The four micro-stages of one instruction.
enum Stage {
    STAGE_FETCH,
    STAGE_DECODE,
    STAGE_EXECUTE,
    STAGE_WRITEBACK
};

const char* stageName(Stage s);

// One recorded clock cycle -- this is what the waveform view plots and what the
// history scrubber steps back through.
struct CycleRecord {
    unsigned long  cycle;
    Stage          stage;
    Word           pc;
    std::string    instructionText;
    ControlSignals signals;
    Flags          flags;
    Word           aluA;
    Word           aluB;
    Word           aluResult;
    bool           aluUsed;

    CycleRecord() : cycle(0), stage(STAGE_FETCH), aluUsed(false) {}
};

class CPU {
private:
    RegisterFile   regs_;
    Memory         mem_;
    ALU            alu_;

    ds::Stack<Word>          callStack_;    // CALL / RET and PUSH / POP
    ds::Deque<CycleRecord>   history_;      // recent cycles, for scrubbing
    ds::LinkedList<std::string> output_;    // what OUT produced

    Instruction*   current_;       // instruction currently in the IR
    ControlSignals signals_;       // signals asserted this cycle
    Stage          stage_;
    unsigned long  cycleCount_;
    unsigned long  instructionCount_;
    bool           halted_;
    bool           branchTaken_;

    // last ALU activity, for the diagram
    Word aluA_, aluB_, aluResult_;
    bool aluUsed_;

    size_t maxHistory_;

    void recordCycle();

public:
    CPU();
    ~CPU();

    // -- program loading -----------------------------------------------------
    // Instructions are owned by the CPU; loadProgram takes ownership.
    void loadProgram(ds::LinkedList<Instruction*>& program, unsigned int baseAddress = 0);
    void clearProgram();

    // -- execution -----------------------------------------------------------
    void step();                 // advance exactly one micro-stage
    void stepInstruction();      // advance until the next instruction begins
    void run(unsigned long maxCycles = 100000);
    void reset();

    // -- state for the UI ----------------------------------------------------
    RegisterFile&       registers()       { return regs_; }
    const RegisterFile& registers() const { return regs_; }
    Memory&             memory()          { return mem_; }
    const Memory&       memory()    const { return mem_; }
    ALU&                alu()             { return alu_; }

    const ControlSignals& signals() const { return signals_; }
    Stage                 stage()   const { return stage_; }
    unsigned long         cycles()  const { return cycleCount_; }
    unsigned long         instructionsRetired() const { return instructionCount_; }
    bool                  halted()  const { return halted_; }

    Word aluA()      const { return aluA_; }
    Word aluB()      const { return aluB_; }
    Word aluResult() const { return aluResult_; }
    bool aluUsed()   const { return aluUsed_; }

    const ds::Deque<CycleRecord>&      history() const { return history_; }
    const ds::LinkedList<std::string>& output()  const { return output_; }
    const ds::Stack<Word>&             callStack() const { return callStack_; }

    std::string currentInstructionText() const;

    // -- called by Instruction subclasses during execute() --------------------
    void setSignals(const ControlSignals& s) { signals_ = s; }
    void markBranchTaken()                   { branchTaken_ = true; }
    void halt()                              { halted_ = true; }
    void emit(const std::string& text)       { output_.pushBack(text); }
    void setAluActivity(Word a, Word b, Word r) { aluA_ = a; aluB_ = b; aluResult_ = r; aluUsed_ = true; }

    ds::Stack<Word>& stack() { return callStack_; }

    // The program as loaded, indexed by address, so the listing can be drawn.
    Instruction* instructionAt(unsigned int address) const;
    unsigned int programSize() const { return programSize_; }

private:
    Instruction** program_;      // owned array of instruction pointers
    unsigned int  programSize_;
    unsigned int  programBase_;
};

} // namespace core

#endif // CORE_CPU_H
