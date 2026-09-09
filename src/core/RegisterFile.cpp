#include "RegisterFile.h"
#include <sstream>
#include <cctype>

namespace core {

RegisterFile::RegisterFile()
    : pc_("PC"), ir_("IR"), sp_("SP"), mar_("MAR"), mdr_("MDR"), flagsWritten_(false) {
    for (int i = 0; i < NUM_GP_REGISTERS; ++i)
        gp_[i].setName(gpName(i));
    reset();
}

std::string RegisterFile::gpName(int index) {
    std::ostringstream os;
    os << "R" << index;
    return os.str();
}

int RegisterFile::gpIndexFromName(const std::string& name) {
    if (name.size() < 2) return -1;
    char c0 = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
    if (c0 != 'R') return -1;
    for (size_t i = 1; i < name.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(name[i]))) return -1;
    int idx = 0;
    for (size_t i = 1; i < name.size(); ++i) idx = idx * 10 + (name[i] - '0');
    if (idx < 0 || idx >= NUM_GP_REGISTERS) return -1;
    return idx;
}

Word RegisterFile::readGP(int index) {
    if (index < 0 || index >= NUM_GP_REGISTERS)
        throw InvalidRegisterException("read index out of range");
    return gp_[index].read();
}

Word RegisterFile::peekGP(int index) const {
    if (index < 0 || index >= NUM_GP_REGISTERS)
        throw InvalidRegisterException("peek index out of range");
    return gp_[index].peek();
}

void RegisterFile::writeGP(int index, Word value) {
    if (index < 0 || index >= NUM_GP_REGISTERS)
        throw InvalidRegisterException("write index out of range");
    gp_[index].write(value);
}

const Register& RegisterFile::gp(int index) const {
    if (index < 0 || index >= NUM_GP_REGISTERS)
        throw InvalidRegisterException("gp index out of range");
    return gp_[index];
}

void RegisterFile::reset() {
    for (int i = 0; i < NUM_GP_REGISTERS; ++i) {
        gp_[i].forceSet(Word(0));
        gp_[i].clearActivity();
    }
    pc_.forceSet(Word(0));
    ir_.forceSet(Word(0));
    sp_.forceSet(Word(0xFFFE));   // stack grows downward from the top of memory
    mar_.forceSet(Word(0));
    mdr_.forceSet(Word(0));
    flags_.clear();
    flagsWritten_ = false;
    clearActivity();
}

void RegisterFile::clearActivity() {
    for (int i = 0; i < NUM_GP_REGISTERS; ++i) gp_[i].clearActivity();
    pc_.clearActivity();
    ir_.clearActivity();
    sp_.clearActivity();
    mar_.clearActivity();
    mdr_.clearActivity();
    flagsWritten_ = false;
}

} // namespace core
