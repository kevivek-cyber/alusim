// ---------------------------------------------------------------------------
// RegisterFile.h  --  general-purpose registers + PC, IR, SP and the flags
//
// Covers: COAL Practical 1/2 (registers feeding the ALU and control unit)
//         COA Unit 1        (register organisation)
//         PSOOP Practical 1 (class with constructor, destructor, members)
// ---------------------------------------------------------------------------
#ifndef CORE_REGISTERFILE_H
#define CORE_REGISTERFILE_H

#include <string>
#include "Word.h"
#include "ALU.h"
#include "Exceptions.h"

namespace core {

// One named register. Kept as a small class so the UI can ask it to describe
// itself, and so we can track whether it was touched this cycle (for the
// "light up what is active" visualisation).
class Register {
private:
    std::string name_;
    Word        value_;
    bool        readThisCycle_;
    bool        wroteThisCycle_;

public:
    Register() : name_("??"), readThisCycle_(false), wroteThisCycle_(false) {}
    explicit Register(const std::string& n)
        : name_(n), readThisCycle_(false), wroteThisCycle_(false) {}

    const std::string& name() const { return name_; }
    void setName(const std::string& n) { name_ = n; }

    Word read()  { readThisCycle_ = true;  return value_; }
    Word peek() const { return value_; }                    // read without marking
    void write(Word v) { value_ = v; wroteThisCycle_ = true; }
    void forceSet(Word v) { value_ = v; }                   // reset path, no marking

    bool wasRead()    const { return readThisCycle_; }
    bool wasWritten() const { return wroteThisCycle_; }
    void clearActivity()    { readThisCycle_ = wroteThisCycle_ = false; }
};

// Index constants for the general-purpose registers.
enum { NUM_GP_REGISTERS = 8 };

class RegisterFile {
private:
    Register gp_[NUM_GP_REGISTERS];   // R0..R7
    Register pc_;                     // program counter
    Register ir_;                     // instruction register
    Register sp_;                     // stack pointer
    Register mar_;                    // memory address register
    Register mdr_;                    // memory data register
    Flags    flags_;
    bool     flagsWritten_;

public:
    RegisterFile();

    // -- general purpose -----------------------------------------------------
    Word readGP(int index);
    Word peekGP(int index) const;
    void writeGP(int index, Word value);
    const Register& gp(int index) const;

    static std::string gpName(int index);
    static int         gpIndexFromName(const std::string& name);  // -1 if not a register

    // -- special purpose -----------------------------------------------------
    Register& pc()  { return pc_;  }
    Register& ir()  { return ir_;  }
    Register& sp()  { return sp_;  }
    Register& mar() { return mar_; }
    Register& mdr() { return mdr_; }

    const Register& pc()  const { return pc_;  }
    const Register& ir()  const { return ir_;  }
    const Register& sp()  const { return sp_;  }
    const Register& mar() const { return mar_; }
    const Register& mdr() const { return mdr_; }

    // -- flags ---------------------------------------------------------------
    const Flags& flags() const { return flags_; }
    void  setFlags(const Flags& f) { flags_ = f; flagsWritten_ = true; }
    bool  flagsWritten() const { return flagsWritten_; }

    // -- housekeeping --------------------------------------------------------
    void reset();
    void clearActivity();
};

} // namespace core

#endif // CORE_REGISTERFILE_H
