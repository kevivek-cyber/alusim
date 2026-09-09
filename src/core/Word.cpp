#include "Word.h"
#include <sstream>

namespace core {

std::string Word::toDecimal() const {
    std::ostringstream os;
    os << value_;
    return os.str();
}

std::string Word::toSignedDecimal() const {
    std::ostringstream os;
    os << signedVal();
    return os.str();
}

std::ostream& operator<<(std::ostream& os, const Word& w) {
    os << w.toHex();
    return os;
}

} // namespace core
