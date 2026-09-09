#include "Instruction.h"
#include "CPU.h"
#include <sstream>

namespace core {

// ---------------------------------------------------------------------------
// ControlSignals -- the fixed row order used by the waveform view
// ---------------------------------------------------------------------------
namespace {
const char* kSignalNames[] = {
    "PCout", "PCload", "PCinc", "MARload", "MemRead", "MemWrite",
    "IRload", "RegRead", "RegWrite", "ALUen", "FlagWr", "StackOp",
    "BusAct", "Halt"
};
const int kSignalCount = 14;
}

int ControlSignals::signalCount() { return kSignalCount; }

const char* ControlSignals::signalName(int i) {
    if (i < 0 || i >= kSignalCount) return "?";
    return kSignalNames[i];
}

bool ControlSignals::signalValue(int i) const {
    switch (i) {
        case 0:  return pcOut;
        case 1:  return pcLoad;
        case 2:  return pcInc;
        case 3:  return marLoad;
        case 4:  return memRead;
        case 5:  return memWrite;
        case 6:  return irLoad;
        case 7:  return regRead;
        case 8:  return regWrite;
        case 9:  return aluEnable;
        case 10: return flagWrite;
        case 11: return stackOp;
        case 12: return busActive;
        case 13: return halt;
        default: return false;
    }
}

// ---------------------------------------------------------------------------
// Instruction base
// ---------------------------------------------------------------------------
std::string Instruction::toString() const {
    return mnemonic_;
}

// ---------------------------------------------------------------------------
// NOP / HLT
// ---------------------------------------------------------------------------
void NopInstruction::execute(CPU&) { /* deliberately does nothing */ }

ControlSignals NopInstruction::signals() const {
    ControlSignals s;
    s.pcInc = true;
    return s;
}

void HaltInstruction::execute(CPU& cpu) { cpu.halt(); }

ControlSignals HaltInstruction::signals() const {
    ControlSignals s;
    s.halt = true;
    return s;
}

// ---------------------------------------------------------------------------
// LOAD Rd, #imm
// ---------------------------------------------------------------------------
void LoadImmediateInstruction::execute(CPU& cpu) {
    cpu.registers().writeGP(rd_, operand_);
    cpu.setAluActivity(operand_, Word(0), operand_);
}

ControlSignals LoadImmediateInstruction::signals() const {
    ControlSignals s;
    s.regWrite  = true;
    s.busActive = true;
    s.pcInc     = true;
    s.aluEnable = true;
    s.aluOp     = ALU_PASS_B;
    return s;
}

std::string LoadImmediateInstruction::toString() const {
    std::ostringstream os;
    os << "LOAD  " << RegisterFile::gpName(rd_) << ", #" << operand_.raw();
    return os.str();
}

// ---------------------------------------------------------------------------
// LOADM Rd, addr
// ---------------------------------------------------------------------------
void LoadMemoryInstruction::execute(CPU& cpu) {
    Word v = cpu.memory().read(operand_.raw());
    cpu.registers().mar().write(operand_);
    cpu.registers().mdr().write(v);
    cpu.registers().writeGP(rd_, v);
    cpu.setAluActivity(v, Word(0), v);
}

ControlSignals LoadMemoryInstruction::signals() const {
    ControlSignals s;
    s.marLoad   = true;
    s.memRead   = true;
    s.regWrite  = true;
    s.busActive = true;
    s.pcInc     = true;
    return s;
}

std::string LoadMemoryInstruction::toString() const {
    std::ostringstream os;
    os << "LOADM " << RegisterFile::gpName(rd_) << ", [" << operand_.toHex() << "]";
    return os.str();
}

// ---------------------------------------------------------------------------
// STORE Rs, addr
// ---------------------------------------------------------------------------
void StoreInstruction::execute(CPU& cpu) {
    Word v = cpu.registers().readGP(rs_);
    cpu.registers().mar().write(operand_);
    cpu.registers().mdr().write(v);
    cpu.memory().write(operand_.raw(), v);
    cpu.setAluActivity(v, Word(0), v);
}

ControlSignals StoreInstruction::signals() const {
    ControlSignals s;
    s.marLoad   = true;
    s.memWrite  = true;
    s.regRead   = true;
    s.busActive = true;
    s.pcInc     = true;
    return s;
}

std::string StoreInstruction::toString() const {
    std::ostringstream os;
    os << "STORE " << RegisterFile::gpName(rs_) << ", [" << operand_.toHex() << "]";
    return os.str();
}

// ---------------------------------------------------------------------------
// MOV Rd, Rs
// ---------------------------------------------------------------------------
void MoveInstruction::execute(CPU& cpu) {
    Word v = cpu.registers().readGP(rs_);
    cpu.registers().writeGP(rd_, v);
    cpu.setAluActivity(v, Word(0), v);
}

ControlSignals MoveInstruction::signals() const {
    ControlSignals s;
    s.regRead   = true;
    s.regWrite  = true;
    s.busActive = true;
    s.aluEnable = true;
    s.aluOp     = ALU_PASS_A;
    s.pcInc     = true;
    return s;
}

std::string MoveInstruction::toString() const {
    std::ostringstream os;
    os << "MOV   " << RegisterFile::gpName(rd_) << ", " << RegisterFile::gpName(rs_);
    return os.str();
}

// ---------------------------------------------------------------------------
// Two-register ALU instructions
// ---------------------------------------------------------------------------
void AluBinaryInstruction::execute(CPU& cpu) {
    Word a = cpu.registers().readGP(rd_);
    Word b = cpu.registers().readGP(rs_);
    AluResult r = cpu.alu().execute(op_, a, b);
    if (r.writesResult) cpu.registers().writeGP(rd_, r.value);
    cpu.registers().setFlags(r.flags);
    cpu.setAluActivity(a, b, r.value);
}

ControlSignals AluBinaryInstruction::signals() const {
    ControlSignals s;
    s.regRead   = true;
    s.regWrite  = (op_ != ALU_CMP);
    s.aluEnable = true;
    s.aluOp     = op_;
    s.flagWrite = true;
    s.busActive = true;
    s.pcInc     = true;
    return s;
}

std::string AluBinaryInstruction::toString() const {
    std::ostringstream os;
    os << mnemonic_;
    for (size_t i = mnemonic_.size(); i < 6; ++i) os << ' ';
    os << RegisterFile::gpName(rd_) << ", " << RegisterFile::gpName(rs_);
    return os.str();
}

// ---------------------------------------------------------------------------
// Single-register ALU instructions
// ---------------------------------------------------------------------------
void AluUnaryInstruction::execute(CPU& cpu) {
    Word a = cpu.registers().readGP(rd_);
    AluResult r = cpu.alu().execute(op_, a, Word(0));
    cpu.registers().writeGP(rd_, r.value);
    cpu.registers().setFlags(r.flags);
    cpu.setAluActivity(a, Word(0), r.value);
}

ControlSignals AluUnaryInstruction::signals() const {
    ControlSignals s;
    s.regRead   = true;
    s.regWrite  = true;
    s.aluEnable = true;
    s.aluOp     = op_;
    s.flagWrite = true;
    s.busActive = true;
    s.pcInc     = true;
    return s;
}

std::string AluUnaryInstruction::toString() const {
    std::ostringstream os;
    os << mnemonic_;
    for (size_t i = mnemonic_.size(); i < 6; ++i) os << ' ';
    os << RegisterFile::gpName(rd_);
    return os.str();
}

// ---------------------------------------------------------------------------
// Branches
// ---------------------------------------------------------------------------
bool JumpInstruction::conditionMet(const Flags& f) const {
    switch (cond_) {
        case ALWAYS:       return true;
        case IF_ZERO:      return f.zero;
        case IF_NOT_ZERO:  return !f.zero;
        case IF_CARRY:     return f.carry;
        case IF_NOT_CARRY: return !f.carry;
        case IF_NEGATIVE:  return f.negative;
        default:           return false;
    }
}

void JumpInstruction::execute(CPU& cpu) {
    if (conditionMet(cpu.registers().flags())) {
        cpu.registers().pc().write(operand_);
        cpu.markBranchTaken();
    }
}

ControlSignals JumpInstruction::signals() const {
    ControlSignals s;
    s.pcLoad    = true;
    s.busActive = true;
    s.pcInc     = (cond_ != ALWAYS);   // falls through if not taken
    return s;
}

std::string JumpInstruction::toString() const {
    std::ostringstream os;
    os << mnemonic_;
    for (size_t i = mnemonic_.size(); i < 6; ++i) os << ' ';
    os << operand_.toHex();
    return os.str();
}

// ---------------------------------------------------------------------------
// CALL / RET -- our own Stack<Word> is the hardware call stack
// ---------------------------------------------------------------------------
void CallInstruction::execute(CPU& cpu) {
    // The return address is the instruction *after* the CALL.  Because CALL
    // marks the branch as taken, the writeback stage will not advance the PC
    // for us, so we have to add the +1 here ourselves.
    Word returnTo = cpu.registers().pc().peek() + Word(1);
    cpu.stack().push(returnTo);
    cpu.registers().sp().write(Word(static_cast<u16>(cpu.stack().size())));
    cpu.registers().pc().write(operand_);
    cpu.markBranchTaken();
}

ControlSignals CallInstruction::signals() const {
    ControlSignals s;
    s.pcLoad    = true;
    s.stackOp   = true;
    s.busActive = true;
    return s;
}

std::string CallInstruction::toString() const {
    std::ostringstream os;
    os << "CALL  " << operand_.toHex();
    return os.str();
}

void ReturnInstruction::execute(CPU& cpu) {
    // Stack::pop() throws StackUnderflowException if there is nothing to return
    // to -- that propagates up and is reported by the console UI.
    Word returnTo = cpu.stack().pop();
    cpu.registers().sp().write(Word(static_cast<u16>(cpu.stack().size())));
    cpu.registers().pc().write(returnTo);
    cpu.markBranchTaken();
}

ControlSignals ReturnInstruction::signals() const {
    ControlSignals s;
    s.pcLoad    = true;
    s.stackOp   = true;
    s.busActive = true;
    return s;
}

// ---------------------------------------------------------------------------
// PUSH / POP
// ---------------------------------------------------------------------------
void PushInstruction::execute(CPU& cpu) {
    Word v = cpu.registers().readGP(rs_);
    cpu.stack().push(v);
    cpu.registers().sp().write(Word(static_cast<u16>(cpu.stack().size())));
    cpu.setAluActivity(v, Word(0), v);
}

ControlSignals PushInstruction::signals() const {
    ControlSignals s;
    s.regRead   = true;
    s.stackOp   = true;
    s.busActive = true;
    s.pcInc     = true;
    return s;
}

std::string PushInstruction::toString() const {
    std::ostringstream os;
    os << "PUSH  " << RegisterFile::gpName(rs_);
    return os.str();
}

void PopInstruction::execute(CPU& cpu) {
    Word v = cpu.stack().pop();
    cpu.registers().writeGP(rd_, v);
    cpu.registers().sp().write(Word(static_cast<u16>(cpu.stack().size())));
    cpu.setAluActivity(v, Word(0), v);
}

ControlSignals PopInstruction::signals() const {
    ControlSignals s;
    s.regWrite  = true;
    s.stackOp   = true;
    s.busActive = true;
    s.pcInc     = true;
    return s;
}

std::string PopInstruction::toString() const {
    std::ostringstream os;
    os << "POP   " << RegisterFile::gpName(rd_);
    return os.str();
}

// ---------------------------------------------------------------------------
// OUT
// ---------------------------------------------------------------------------
void OutInstruction::execute(CPU& cpu) {
    Word v = cpu.registers().readGP(rs_);
    std::ostringstream os;
    os << RegisterFile::gpName(rs_) << " = " << v.raw()
       << "  (" << v.toHex() << ", signed " << v.signedVal() << ")";
    cpu.emit(os.str());
    cpu.setAluActivity(v, Word(0), v);
}

ControlSignals OutInstruction::signals() const {
    ControlSignals s;
    s.regRead   = true;
    s.busActive = true;
    s.pcInc     = true;
    return s;
}

std::string OutInstruction::toString() const {
    std::ostringstream os;
    os << "OUT   " << RegisterFile::gpName(rs_);
    return os.str();
}

// ---------------------------------------------------------------------------
// MUL -- add-and-shift, a multi-step ALU operation (COAL Practical 5)
// ---------------------------------------------------------------------------
void MultiplyInstruction::execute(CPU& cpu) {
    Word a = cpu.registers().readGP(rd_);
    Word b = cpu.registers().readGP(rs_);
    Word product = ALU::multiplyAddShift(a, b, 0);
    cpu.registers().writeGP(rd_, product);

    Flags f;
    f.zero     = (product.raw() == 0);
    f.negative = product.msb();
    cpu.registers().setFlags(f);
    cpu.setAluActivity(a, b, product);
}

ControlSignals MultiplyInstruction::signals() const {
    ControlSignals s;
    s.regRead   = true;
    s.regWrite  = true;
    s.aluEnable = true;
    s.aluOp     = ALU_ADD;      // internally it is a sequence of adds and shifts
    s.flagWrite = true;
    s.busActive = true;
    s.pcInc     = true;
    return s;
}

std::string MultiplyInstruction::toString() const {
    std::ostringstream os;
    os << "MUL   " << RegisterFile::gpName(rd_) << ", " << RegisterFile::gpName(rs_);
    return os.str();
}

} // namespace core
