#include "Assembler.h"
#include "../core/RegisterFile.h"
#include "../core/Exceptions.h"
#include <sstream>
#include <fstream>
#include <cctype>

using namespace core;

namespace asmb {

std::string Assembler::trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::string Assembler::upper(const std::string& s) {
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i)
        r[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(r[i])));
    return r;
}

// Split on whitespace and commas; everything after ';' is a comment.
void Assembler::splitTokens(const std::string& line, ds::LinkedList<std::string>& out) {
    out.clear();
    std::string cur;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == ';') break;                       // comment to end of line
        if (std::isspace(static_cast<unsigned char>(c)) || c == ',') {
            if (!cur.empty()) { out.pushBack(cur); cur.clear(); }
        } else if (c == '[' || c == ']') {
            if (!cur.empty()) { out.pushBack(cur); cur.clear(); }
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) out.pushBack(cur);
}

int Assembler::parseRegister(const std::string& token, int lineNo) const {
    int idx = RegisterFile::gpIndexFromName(upper(token));
    if (idx < 0) {
        std::ostringstream os;
        os << "line " << lineNo << ": '" << token << "' is not a register (expected R0-R7)";
        throw AssemblyErrorException(os.str());
    }
    return idx;
}

// Accepts  #42   42   0x2A   0b1010   or a label name (resolved via symbols_)
Word Assembler::parseValue(const std::string& token, int lineNo) const {
    std::string t = token;
    if (!t.empty() && t[0] == '#') t = t.substr(1);
    if (t.empty()) {
        std::ostringstream os;
        os << "line " << lineNo << ": empty value";
        throw AssemblyErrorException(os.str());
    }

    // a known label?
    unsigned int addr = 0;
    if (symbols_.get(t, addr)) return Word(static_cast<u16>(addr));

    std::string up = upper(t);
    bool negative = false;
    size_t i = 0;
    if (up[0] == '-') { negative = true; i = 1; }

    long value = 0;
    if (up.size() > i + 1 && up[i] == '0' && up[i + 1] == 'X') {
        for (size_t k = i + 2; k < up.size(); ++k) {
            char c = up[k];
            int d;
            if (c >= '0' && c <= '9')      d = c - '0';
            else if (c >= 'A' && c <= 'F') d = 10 + (c - 'A');
            else {
                std::ostringstream os;
                os << "line " << lineNo << ": bad hex value '" << token << "'";
                throw AssemblyErrorException(os.str());
            }
            value = value * 16 + d;
        }
    } else if (up.size() > i + 1 && up[i] == '0' && up[i + 1] == 'B') {
        for (size_t k = i + 2; k < up.size(); ++k) {
            if (up[k] != '0' && up[k] != '1') {
                std::ostringstream os;
                os << "line " << lineNo << ": bad binary value '" << token << "'";
                throw AssemblyErrorException(os.str());
            }
            value = value * 2 + (up[k] - '0');
        }
    } else {
        for (size_t k = i; k < up.size(); ++k) {
            if (!std::isdigit(static_cast<unsigned char>(up[k]))) {
                std::ostringstream os;
                os << "line " << lineNo << ": '" << token
                   << "' is not a number or a known label";
                throw AssemblyErrorException(os.str());
            }
            value = value * 10 + (up[k] - '0');
        }
    }

    if (negative) value = -value;
    return Word(static_cast<u16>(value & 0xFFFF));
}

core::Instruction* Assembler::buildInstruction(const std::string& m,
                                               ds::LinkedList<std::string>& ops,
                                               int lineNo) {
    const size_t n = ops.size();

    // --- no-operand instructions -------------------------------------------
    if (m == "NOP")  return new NopInstruction();
    if (m == "HLT" || m == "HALT") return new HaltInstruction();
    if (m == "RET")  return new ReturnInstruction();

    // --- helpers ------------------------------------------------------------
    std::ostringstream err;
    if (n == 0) {
        err << "line " << lineNo << ": '" << m << "' needs operands";
        throw AssemblyErrorException(err.str());
    }

    // --- two-operand ALU ----------------------------------------------------
    struct { const char* name; AluOp op; } binOps[] = {
        {"ADD", ALU_ADD}, {"SUB", ALU_SUB}, {"AND", ALU_AND},
        {"OR",  ALU_OR},  {"XOR", ALU_XOR}, {"CMP", ALU_CMP}
    };
    for (int i = 0; i < 6; ++i) {
        if (m == binOps[i].name) {
            if (n != 2) {
                err << "line " << lineNo << ": " << m << " needs two registers";
                throw AssemblyErrorException(err.str());
            }
            int rd = parseRegister(ops.at(0), lineNo);
            int rs = parseRegister(ops.at(1), lineNo);
            return new AluBinaryInstruction(m, binOps[i].op, rd, rs);
        }
    }

    // --- one-operand ALU ----------------------------------------------------
    struct { const char* name; AluOp op; } unOps[] = {
        {"NOT", ALU_NOT}, {"SHL", ALU_SHL}, {"SHR", ALU_SHR},
        {"INC", ALU_INC}, {"DEC", ALU_DEC}
    };
    for (int i = 0; i < 5; ++i) {
        if (m == unOps[i].name) {
            if (n != 1) {
                err << "line " << lineNo << ": " << m << " needs one register";
                throw AssemblyErrorException(err.str());
            }
            return new AluUnaryInstruction(m, unOps[i].op, parseRegister(ops.at(0), lineNo));
        }
    }

    // --- data movement ------------------------------------------------------
    if (m == "LOAD") {
        if (n != 2) { err << "line " << lineNo << ": LOAD needs Rd and a value";
                      throw AssemblyErrorException(err.str()); }
        return new LoadImmediateInstruction(parseRegister(ops.at(0), lineNo),
                                            parseValue(ops.at(1), lineNo));
    }
    if (m == "LOADM") {
        if (n != 2) { err << "line " << lineNo << ": LOADM needs Rd and an address";
                      throw AssemblyErrorException(err.str()); }
        return new LoadMemoryInstruction(parseRegister(ops.at(0), lineNo),
                                         parseValue(ops.at(1), lineNo));
    }
    if (m == "STORE") {
        if (n != 2) { err << "line " << lineNo << ": STORE needs Rs and an address";
                      throw AssemblyErrorException(err.str()); }
        return new StoreInstruction(parseRegister(ops.at(0), lineNo),
                                    parseValue(ops.at(1), lineNo));
    }
    if (m == "MOV") {
        if (n != 2) { err << "line " << lineNo << ": MOV needs two registers";
                      throw AssemblyErrorException(err.str()); }
        return new MoveInstruction(parseRegister(ops.at(0), lineNo),
                                   parseRegister(ops.at(1), lineNo));
    }
    if (m == "MUL") {
        if (n != 2) { err << "line " << lineNo << ": MUL needs two registers";
                      throw AssemblyErrorException(err.str()); }
        return new MultiplyInstruction(parseRegister(ops.at(0), lineNo),
                                       parseRegister(ops.at(1), lineNo));
    }

    // --- branches -----------------------------------------------------------
    struct { const char* name; JumpInstruction::Condition c; } jumps[] = {
        {"JMP", JumpInstruction::ALWAYS},
        {"JZ",  JumpInstruction::IF_ZERO},
        {"JNZ", JumpInstruction::IF_NOT_ZERO},
        {"JC",  JumpInstruction::IF_CARRY},
        {"JNC", JumpInstruction::IF_NOT_CARRY},
        {"JN",  JumpInstruction::IF_NEGATIVE}
    };
    for (int i = 0; i < 6; ++i) {
        if (m == jumps[i].name) {
            if (n != 1) { err << "line " << lineNo << ": " << m << " needs a target";
                          throw AssemblyErrorException(err.str()); }
            return new JumpInstruction(m, jumps[i].c, parseValue(ops.at(0), lineNo));
        }
    }

    if (m == "CALL") {
        if (n != 1) { err << "line " << lineNo << ": CALL needs a target";
                      throw AssemblyErrorException(err.str()); }
        return new CallInstruction(parseValue(ops.at(0), lineNo));
    }
    if (m == "PUSH") return new PushInstruction(parseRegister(ops.at(0), lineNo));
    if (m == "POP")  return new PopInstruction(parseRegister(ops.at(0), lineNo));
    if (m == "OUT")  return new OutInstruction(parseRegister(ops.at(0), lineNo));

    err << "line " << lineNo << ": unknown instruction '" << m << "'";
    throw InvalidOpcodeException(err.str());
}

bool Assembler::assemble(const std::string& source,
                         ds::LinkedList<core::Instruction*>& out) {
    symbols_.clear();
    errors_.clear();
    listing_.clear();
    out.clear();

    // ---- collect the non-empty source lines --------------------------------
    ds::LinkedList<SourceLine> lines;
    {
        std::istringstream in(source);
        std::string raw;
        int lineNo = 0;
        while (std::getline(in, raw)) {
            ++lineNo;
            std::string t = trim(raw);
            if (t.empty() || t[0] == ';') continue;
            lines.pushBack(SourceLine(t, lineNo));
        }
    }

    // ---- pass 1: record label addresses in the hash table -------------------
    {
        unsigned int address = 0;
        ds::LinkedList<SourceLine>::Iterator it = lines.iterator();
        while (it.hasNext()) {
            SourceLine& sl = it.next();
            std::string t = sl.text;

            size_t colon = t.find(':');
            if (colon != std::string::npos) {
                std::string label = trim(t.substr(0, colon));
                if (!label.empty()) symbols_.put(label, address);
                t = trim(t.substr(colon + 1));
                sl.text = t;                  // strip the label for pass 2
            }
            if (!t.empty()) ++address;
        }
    }

    // ---- pass 2: build the instructions ------------------------------------
    bool ok = true;
    unsigned int address = 0;
    ds::LinkedList<SourceLine>::Iterator it = lines.iterator();
    while (it.hasNext()) {
        SourceLine& sl = it.next();
        if (sl.text.empty()) continue;

        ds::LinkedList<std::string> tokens;
        splitTokens(sl.text, tokens);
        if (tokens.empty()) continue;

        std::string mnemonic = upper(tokens.at(0));
        ds::LinkedList<std::string> operands;
        for (size_t i = 1; i < tokens.size(); ++i) operands.pushBack(tokens.at(i));

        try {
            core::Instruction* instr = buildInstruction(mnemonic, operands, sl.lineNumber);
            out.pushBack(instr);

            std::ostringstream ls;
            ls.width(4);
            ls << address << "  " << instr->toString();
            listing_.pushBack(ls.str());
            ++address;
        }
        catch (const SimulatorException& e) {
            // PSOOP Practical 8: catching our own exception type
            errors_.pushBack(std::string(e.what()));
            ok = false;
        }
    }

    if (!ok) {
        // clean up anything already built so we do not leak
        ds::LinkedList<core::Instruction*>::Iterator cit = out.iterator();
        while (cit.hasNext()) delete cit.next();
        out.clear();
    }
    return ok;
}

bool Assembler::assembleFile(const std::string& path,
                             ds::LinkedList<core::Instruction*>& out) {
    std::ifstream in(path.c_str());
    if (!in) {
        errors_.clear();
        errors_.pushBack("cannot open file: " + path);
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return assemble(ss.str(), out);
}

} // namespace asmb
