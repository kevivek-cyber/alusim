// ---------------------------------------------------------------------------
// Instruction.h  --  the instruction set, as a polymorphic class hierarchy
//
// Every instruction is a class deriving from the abstract base Instruction.
// The control unit holds a base-class pointer and calls execute() without
// knowing which concrete instruction it is holding -- that is the polymorphism
// the whole fetch/decode/execute loop is built on.
//
// Covers: PSOOP Practical 6 (inheritance -- several levels),
//         PSOOP Practical 7 (polymorphism -- virtual execute()),
//         PSOOP Practical 12 (dynamic_cast in the control unit),
//         COAL Practical 2   (each instruction produces its control signals)
// ---------------------------------------------------------------------------
#ifndef CORE_INSTRUCTION_H
#define CORE_INSTRUCTION_H

#include <string>
#include "Word.h"
#include "ALU.h"

namespace core {

class RegisterFile;
class Memory;
class CPU;

// ---------------------------------------------------------------------------
// The control-signal vector.  This is what the waveform (logic analyzer) view
// plots as one row per signal, per clock cycle.
// ---------------------------------------------------------------------------
struct ControlSignals {
    bool  pcOut;        // PC is driving the address bus
    bool  pcLoad;       // PC is being loaded (a branch happened)
    bool  pcInc;        // PC is being incremented
    bool  marLoad;      // memory address register is loading
    bool  memRead;      // memory read strobe
    bool  memWrite;     // memory write strobe
    bool  irLoad;       // instruction register is loading
    bool  regRead;      // register file is being read
    bool  regWrite;     // register file is being written
    bool  aluEnable;    // ALU is performing an operation
    bool  flagWrite;    // status flags are being updated
    bool  stackOp;      // stack pointer is moving
    bool  busActive;    // the shared data bus is carrying something
    bool  halt;         // machine is stopping
    AluOp aluOp;

    ControlSignals()
        : pcOut(false), pcLoad(false), pcInc(false), marLoad(false),
          memRead(false), memWrite(false), irLoad(false), regRead(false),
          regWrite(false), aluEnable(false), flagWrite(false), stackOp(false),
          busActive(false), halt(false), aluOp(ALU_NONE) {}

    void clear() { *this = ControlSignals(); }

    // Names in the fixed order the waveform view draws them.
    static int         signalCount();
    static const char* signalName(int i);
    bool               signalValue(int i) const;
};

// ---------------------------------------------------------------------------
// Abstract base class.
// ---------------------------------------------------------------------------
class Instruction {
protected:
    std::string mnemonic_;
    int         rd_;        // destination register index, -1 if unused
    int         rs_;        // source register index, -1 if unused
    Word        operand_;   // immediate value or address
    bool        hasOperand_;

public:
    Instruction(const std::string& m, int rd, int rs, Word operand, bool hasOperand)
        : mnemonic_(m), rd_(rd), rs_(rs), operand_(operand), hasOperand_(hasOperand) {}

    virtual ~Instruction() {}                    // virtual dtor -- PSOOP P1

    // The two pure virtual functions every instruction must supply.
    virtual void           execute(CPU& cpu) = 0;
    virtual ControlSignals signals() const   = 0;

    // Human-readable form for the program listing.
    virtual std::string toString() const;

    const std::string& mnemonic()  const { return mnemonic_; }
    int                rd()        const { return rd_; }
    int                rs()        const { return rs_; }
    Word               operand()   const { return operand_; }
    bool               hasOperand()const { return hasOperand_; }
};

// ---------------------------------------------------------------------------
// Concrete instructions
// ---------------------------------------------------------------------------

class NopInstruction : public Instruction {
public:
    NopInstruction() : Instruction("NOP", -1, -1, Word(0), false) {}
    void           execute(CPU& cpu);
    ControlSignals signals() const;
};

class HaltInstruction : public Instruction {
public:
    HaltInstruction() : Instruction("HLT", -1, -1, Word(0), false) {}
    void           execute(CPU& cpu);
    ControlSignals signals() const;
};

// LOAD Rd, #imm
class LoadImmediateInstruction : public Instruction {
public:
    LoadImmediateInstruction(int rd, Word imm)
        : Instruction("LOAD", rd, -1, imm, true) {}
    void           execute(CPU& cpu);
    ControlSignals signals() const;
    std::string    toString() const;
};

// LOADM Rd, addr
class LoadMemoryInstruction : public Instruction {
public:
    LoadMemoryInstruction(int rd, Word addr)
        : Instruction("LOADM", rd, -1, addr, true) {}
    void           execute(CPU& cpu);
    ControlSignals signals() const;
    std::string    toString() const;
};

// STORE Rs, addr
class StoreInstruction : public Instruction {
public:
    StoreInstruction(int rs, Word addr)
        : Instruction("STORE", -1, rs, addr, true) {}
    void           execute(CPU& cpu);
    ControlSignals signals() const;
    std::string    toString() const;
};

// MOV Rd, Rs
class MoveInstruction : public Instruction {
public:
    MoveInstruction(int rd, int rs) : Instruction("MOV", rd, rs, Word(0), false) {}
    void           execute(CPU& cpu);
    ControlSignals signals() const;
    std::string    toString() const;
};

// Two-register ALU instructions: ADD, SUB, AND, OR, XOR, CMP
class AluBinaryInstruction : public Instruction {
private:
    AluOp op_;
public:
    AluBinaryInstruction(const std::string& m, AluOp op, int rd, int rs)
        : Instruction(m, rd, rs, Word(0), false), op_(op) {}
    void           execute(CPU& cpu);
    ControlSignals signals() const;
    std::string    toString() const;
    AluOp          op() const { return op_; }
};

// Single-register ALU instructions: NOT, SHL, SHR, INC, DEC
class AluUnaryInstruction : public Instruction {
private:
    AluOp op_;
public:
    AluUnaryInstruction(const std::string& m, AluOp op, int rd)
        : Instruction(m, rd, -1, Word(0), false), op_(op) {}
    void           execute(CPU& cpu);
    ControlSignals signals() const;
    std::string    toString() const;
    AluOp          op() const { return op_; }
};

// Branches: JMP, JZ, JNZ, JC, JNC, JN
class JumpInstruction : public Instruction {
public:
    enum Condition { ALWAYS, IF_ZERO, IF_NOT_ZERO, IF_CARRY, IF_NOT_CARRY, IF_NEGATIVE };
private:
    Condition cond_;
public:
    JumpInstruction(const std::string& m, Condition c, Word target)
        : Instruction(m, -1, -1, target, true), cond_(c) {}
    void           execute(CPU& cpu);
    ControlSignals signals() const;
    std::string    toString() const;
    Condition      condition() const { return cond_; }
    bool           conditionMet(const Flags& f) const;
};

// CALL addr  /  RET   -- these use the hardware call stack
class CallInstruction : public Instruction {
public:
    explicit CallInstruction(Word target) : Instruction("CALL", -1, -1, target, true) {}
    void           execute(CPU& cpu);
    ControlSignals signals() const;
    std::string    toString() const;
};

class ReturnInstruction : public Instruction {
public:
    ReturnInstruction() : Instruction("RET", -1, -1, Word(0), false) {}
    void           execute(CPU& cpu);
    ControlSignals signals() const;
};

// PUSH Rs / POP Rd
class PushInstruction : public Instruction {
public:
    explicit PushInstruction(int rs) : Instruction("PUSH", -1, rs, Word(0), false) {}
    void           execute(CPU& cpu);
    ControlSignals signals() const;
    std::string    toString() const;
};

class PopInstruction : public Instruction {
public:
    explicit PopInstruction(int rd) : Instruction("POP", rd, -1, Word(0), false) {}
    void           execute(CPU& cpu);
    ControlSignals signals() const;
    std::string    toString() const;
};

// OUT Rs -- print a register to the console output pane
class OutInstruction : public Instruction {
public:
    explicit OutInstruction(int rs) : Instruction("OUT", -1, rs, Word(0), false) {}
    void           execute(CPU& cpu);
    ControlSignals signals() const;
    std::string    toString() const;
};

// MUL Rd, Rs -- add-and-shift multiply, a multi-cycle ALU operation
// (COAL Practical 5)
class MultiplyInstruction : public Instruction {
public:
    MultiplyInstruction(int rd, int rs) : Instruction("MUL", rd, rs, Word(0), false) {}
    void           execute(CPU& cpu);
    ControlSignals signals() const;
    std::string    toString() const;
};

} // namespace core

#endif // CORE_INSTRUCTION_H
