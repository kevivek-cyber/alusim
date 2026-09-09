// ---------------------------------------------------------------------------
// Assembler.h  --  turns mnemonic source text into Instruction objects
//
// Two passes:
//   pass 1 -- walk the source, record every label and its address in our own
//             hash table (this is PL Practical 11's symbol table, doing real work)
//   pass 2 -- build the Instruction objects, resolving label references
//
// Covers: PL Practical 11 (hashing -- symbol table),
//         COAL Practicals 3-7 (assembly language programming),
//         PSOOP Practical 8   (throws AssemblyErrorException on bad input)
// ---------------------------------------------------------------------------
#ifndef ASM_ASSEMBLER_H
#define ASM_ASSEMBLER_H

#include <string>
#include "../core/Instruction.h"
#include "../ds/LinkedList.h"
#include "../ds/HashMap.h"

namespace asmb {

struct SourceLine {
    std::string text;
    int         lineNumber;
    SourceLine() : lineNumber(0) {}
    SourceLine(const std::string& t, int n) : text(t), lineNumber(n) {}
};

class Assembler {
private:
    ds::HashMap<std::string, unsigned int> symbols_;   // label -> address
    ds::LinkedList<std::string>            errors_;
    ds::LinkedList<std::string>            listing_;

    static std::string trim(const std::string& s);
    static std::string upper(const std::string& s);
    static void        splitTokens(const std::string& line,
                                   ds::LinkedList<std::string>& out);

    // parse helpers; throw AssemblyErrorException on bad input
    int          parseRegister(const std::string& token, int lineNo) const;
    core::Word   parseValue(const std::string& token, int lineNo) const;

    core::Instruction* buildInstruction(const std::string& mnemonic,
                                        ds::LinkedList<std::string>& operands,
                                        int lineNo);

public:
    Assembler() : symbols_(127, ds::SEPARATE_CHAINING) {}

    // Assemble source into a list of instructions (caller/CPU takes ownership).
    // Returns true on success; on failure errors() explains why.
    bool assemble(const std::string& source,
                  ds::LinkedList<core::Instruction*>& out);

    bool assembleFile(const std::string& path,
                      ds::LinkedList<core::Instruction*>& out);

    const ds::LinkedList<std::string>& errors()  const { return errors_; }
    const ds::LinkedList<std::string>& listing() const { return listing_; }

    // symbol table introspection -- the UI shows this to demonstrate hashing
    const ds::HashMap<std::string, unsigned int>& symbols() const { return symbols_; }
};

} // namespace asmb

#endif // ASM_ASSEMBLER_H
