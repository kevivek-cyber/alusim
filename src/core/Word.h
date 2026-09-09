// ---------------------------------------------------------------------------
// Word.h  --  the 16-bit machine word the whole simulator moves around
//
// Covers: PSOOP Practical 3  (operator overloading -- arithmetic, comparison,
//                             stream insertion, with default + parameterised
//                             constructors)
//         PSOOP Practical 12 (type casting / casting operators)
// ---------------------------------------------------------------------------
#ifndef CORE_WORD_H
#define CORE_WORD_H

#include <string>
#include <ostream>

namespace core {

typedef unsigned short u16;
typedef short          s16;
typedef unsigned int   u32;

class Word {
private:
    u16 value_;

public:
    static const int BITS = 16;

    Word()                : value_(0) {}                       // default ctor
    Word(u16 v)           : value_(v) {}                       // parameterised ctor
    Word(const Word& o)   : value_(o.value_) {}                // copy ctor

    Word& operator=(const Word& o) { value_ = o.value_; return *this; }

    // -- accessors -----------------------------------------------------------
    u16  raw()      const { return value_; }
    s16  signedVal()const { return static_cast<s16>(value_); }
    bool bit(int i) const { return ((value_ >> i) & 1u) != 0; }
    bool msb()      const { return bit(BITS - 1); }

    // conversion operator -- PSOOP Practical 12
    operator u16() const { return value_; }

    // -- arithmetic / logic operators ---------------------------------------
    Word operator+(const Word& o) const { return Word(static_cast<u16>(value_ + o.value_)); }
    Word operator-(const Word& o) const { return Word(static_cast<u16>(value_ - o.value_)); }
    Word operator&(const Word& o) const { return Word(static_cast<u16>(value_ & o.value_)); }
    Word operator|(const Word& o) const { return Word(static_cast<u16>(value_ | o.value_)); }
    Word operator^(const Word& o) const { return Word(static_cast<u16>(value_ ^ o.value_)); }
    Word operator~()              const { return Word(static_cast<u16>(~value_)); }
    Word operator<<(int n)        const { return Word(static_cast<u16>(value_ << n)); }
    Word operator>>(int n)        const { return Word(static_cast<u16>(value_ >> n)); }

    Word& operator+=(const Word& o) { value_ = static_cast<u16>(value_ + o.value_); return *this; }
    Word& operator-=(const Word& o) { value_ = static_cast<u16>(value_ - o.value_); return *this; }

    Word& operator++()    { ++value_; return *this; }              // pre-increment
    Word  operator++(int) { Word t(*this); ++value_; return t; }   // post-increment
    Word& operator--()    { --value_; return *this; }
    Word  operator--(int) { Word t(*this); --value_; return t; }

    // -- comparison ----------------------------------------------------------
    bool operator==(const Word& o) const { return value_ == o.value_; }
    bool operator!=(const Word& o) const { return value_ != o.value_; }
    bool operator< (const Word& o) const { return value_ <  o.value_; }
    bool operator> (const Word& o) const { return value_ >  o.value_; }
    bool operator<=(const Word& o) const { return value_ <= o.value_; }
    bool operator>=(const Word& o) const { return value_ >= o.value_; }

    // -- formatting ----------------------------------------------------------
    std::string toBinary(int width = BITS) const {
        std::string s;
        for (int i = width - 1; i >= 0; --i) {
            s += bit(i) ? '1' : '0';
            if (i % 4 == 0 && i != 0) s += ' ';
        }
        return s;
    }

    std::string toHex(int digits = 4) const {
        static const char* D = "0123456789ABCDEF";
        std::string s(digits, '0');
        u16 v = value_;
        for (int i = digits - 1; i >= 0; --i) { s[i] = D[v & 0xF]; v >>= 4; }
        return "0x" + s;
    }

    std::string toDecimal() const;
    std::string toSignedDecimal() const;
};

// stream insertion -- PSOOP Practical 3
std::ostream& operator<<(std::ostream& os, const Word& w);

} // namespace core

#endif // CORE_WORD_H
