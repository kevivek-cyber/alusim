#include "ALU.h"
#include <sstream>

namespace core {

const char* aluOpName(AluOp op) {
    switch (op) {
        case ALU_ADD:    return "ADD";
        case ALU_SUB:    return "SUB";
        case ALU_AND:    return "AND";
        case ALU_OR:     return "OR";
        case ALU_XOR:    return "XOR";
        case ALU_NOT:    return "NOT";
        case ALU_SHL:    return "SHL";
        case ALU_SHR:    return "SHR";
        case ALU_CMP:    return "CMP";
        case ALU_INC:    return "INC";
        case ALU_DEC:    return "DEC";
        case ALU_PASS_A: return "PASS";
        case ALU_PASS_B: return "PASS";
        default:         return "----";
    }
}

// Signed overflow on addition: both operands share a sign, and the result's
// sign differs from it.
bool ALU::computeOverflowAdd(Word a, Word b, Word r) {
    return (a.msb() == b.msb()) && (r.msb() != a.msb());
}

// Signed overflow on subtraction: operands differ in sign, and the result's
// sign differs from the minuend.
bool ALU::computeOverflowSub(Word a, Word b, Word r) {
    return (a.msb() != b.msb()) && (r.msb() != a.msb());
}

AluResult ALU::execute(AluOp op, Word a, Word b) {
    ++opCount_;

    AluResult res;
    u32 wide = 0;          // 32-bit scratch so we can see the carry out of bit 15

    switch (op) {
        case ALU_ADD:
            wide = static_cast<u32>(a.raw()) + static_cast<u32>(b.raw());
            res.value = Word(static_cast<u16>(wide));
            res.flags.carry    = (wide & 0x10000u) != 0;
            res.flags.overflow = computeOverflowAdd(a, b, res.value);
            break;

        case ALU_INC:
            wide = static_cast<u32>(a.raw()) + 1u;
            res.value = Word(static_cast<u16>(wide));
            res.flags.carry    = (wide & 0x10000u) != 0;
            res.flags.overflow = computeOverflowAdd(a, Word(1), res.value);
            break;

        case ALU_SUB:
        case ALU_CMP:
            wide = static_cast<u32>(a.raw()) - static_cast<u32>(b.raw());
            res.value = Word(static_cast<u16>(wide));
            res.flags.carry    = a.raw() < b.raw();          // borrow
            res.flags.overflow = computeOverflowSub(a, b, res.value);
            res.writesResult   = (op != ALU_CMP);
            break;

        case ALU_DEC:
            wide = static_cast<u32>(a.raw()) - 1u;
            res.value = Word(static_cast<u16>(wide));
            res.flags.carry    = a.raw() < 1u;
            res.flags.overflow = computeOverflowSub(a, Word(1), res.value);
            break;

        case ALU_AND: res.value = a & b;  break;
        case ALU_OR:  res.value = a | b;  break;
        case ALU_XOR: res.value = a ^ b;  break;
        case ALU_NOT: res.value = ~a;     break;

        case ALU_SHL:
            res.flags.carry = a.msb();                       // bit shifted out
            res.value = a << 1;
            break;

        case ALU_SHR:
            res.flags.carry = a.bit(0);                      // bit shifted out
            res.value = a >> 1;
            break;

        case ALU_PASS_A: res.value = a; break;
        case ALU_PASS_B: res.value = b; break;

        case ALU_NONE:
        default:
            res.value = Word(0);
            res.writesResult = false;
            break;
    }

    // Z and N are derived from the result for every operation.
    res.flags.zero     = (res.value.raw() == 0);
    res.flags.negative = res.value.msb();
    return res;
}

// ---------------------------------------------------------------------------
// Add-and-shift multiplication  (COAL Practical 5)
//
// The classic shift-add algorithm: for every set bit of the multiplier, add the
// multiplicand shifted left by that bit position.
// ---------------------------------------------------------------------------
Word ALU::multiplyAddShift(Word a, Word b, std::string* trace) {
    u32 product     = 0;
    u32 multiplicand = a.raw();
    u16 multiplier   = b.raw();

    std::ostringstream os;
    os << "Add-and-shift multiply: " << a.raw() << " x " << b.raw() << "\n";
    os << " step | multiplier bit | multiplicand | product\n";
    os << "------+----------------+--------------+---------\n";

    for (int i = 0; i < Word::BITS; ++i) {
        bool bitSet = ((multiplier >> i) & 1u) != 0;
        if (bitSet) product += multiplicand;

        os << "  " << (i < 10 ? " " : "") << i
           << "  |       " << (bitSet ? '1' : '0')
           << "        |   " << multiplicand
           << "   |  " << product << "\n";

        multiplicand <<= 1;
        if (multiplicand == 0) break;   // nothing left to contribute
    }

    if (trace) *trace = os.str();
    return Word(static_cast<u16>(product & 0xFFFFu));
}

// ---------------------------------------------------------------------------
// Successive-addition multiplication  (the other half of COAL Practical 5)
// ---------------------------------------------------------------------------
Word ALU::multiplySuccessiveAdd(Word a, Word b, std::string* trace) {
    u32 product = 0;
    u16 times   = b.raw();

    std::ostringstream os;
    os << "Successive-addition multiply: " << a.raw() << " x " << b.raw() << "\n";
    os << "Adding " << a.raw() << " to itself " << times << " time(s).\n";

    for (u16 i = 0; i < times; ++i) {
        product += a.raw();
        if (i < 8 || i == times - 1)
            os << "  after add #" << (i + 1) << " : " << product << "\n";
        else if (i == 8)
            os << "  ...\n";
    }

    if (trace) *trace = os.str();
    return Word(static_cast<u16>(product & 0xFFFFu));
}

} // namespace core
