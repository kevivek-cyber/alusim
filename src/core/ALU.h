// ---------------------------------------------------------------------------
// ALU.h  --  the Arithmetic Logic Unit
//
// This is the heart of the project.
// Covers: COAL Practical 1 (design your ALU for arithmetic operations)
//         COA Unit 2       (signed add/sub, add-and-shift multiply, flags)
//
// The ALU is deliberately a *pure* unit: given two operands and an opcode it
// produces a result plus the four status flags, and touches nothing else.
// That makes it independently testable, which is how we verify flag behaviour.
// ---------------------------------------------------------------------------
#ifndef CORE_ALU_H
#define CORE_ALU_H

#include <string>
#include "Word.h"

namespace core {

// The operations the ALU can perform.
enum AluOp {
    ALU_ADD,
    ALU_SUB,
    ALU_AND,
    ALU_OR,
    ALU_XOR,
    ALU_NOT,
    ALU_SHL,
    ALU_SHR,
    ALU_CMP,     // like SUB but result is discarded, flags only
    ALU_INC,
    ALU_DEC,
    ALU_PASS_A,  // move / load path straight through the ALU
    ALU_PASS_B,
    ALU_NONE
};

const char* aluOpName(AluOp op);

// The four status flags every CPU keeps.
struct Flags {
    bool zero;      // result was 0
    bool carry;     // carry out of / borrow into the MSB (unsigned overflow)
    bool overflow;  // signed overflow
    bool negative;  // MSB of the result is set (sign bit)

    Flags() : zero(true), carry(false), overflow(false), negative(false) {}

    void clear() { zero = true; carry = overflow = negative = false; }

    std::string toString() const {
        std::string s;
        s += zero      ? 'Z' : '-';
        s += carry     ? 'C' : '-';
        s += overflow  ? 'V' : '-';
        s += negative  ? 'N' : '-';
        return s;
    }
};

// What one ALU operation produced.
struct AluResult {
    Word  value;
    Flags flags;
    bool  writesResult;   // CMP computes flags but writes nothing back

    AluResult() : writesResult(true) {}
};

class ALU {
private:
    // running totals shown on the stats screen
    unsigned long opCount_;

    static bool computeOverflowAdd(Word a, Word b, Word r);
    static bool computeOverflowSub(Word a, Word b, Word r);

public:
    ALU() : opCount_(0) {}

    // The single entry point. a and b are the operands, op selects the function.
    AluResult execute(AluOp op, Word a, Word b);

    // Add-and-shift multiplication, exposed separately because it is a
    // multi-step algorithm rather than a single ALU function.
    // Covers COAL Practical 5 (multiply via add-and-shift).
    // 'steps' receives a human-readable trace for the UI.
    static Word multiplyAddShift(Word a, Word b, std::string* trace);

    // Successive-addition multiply, the other half of COAL Practical 5.
    static Word multiplySuccessiveAdd(Word a, Word b, std::string* trace);

    unsigned long operationCount() const { return opCount_; }
    void          resetCount()           { opCount_ = 0; }
};

} // namespace core

#endif // CORE_ALU_H
