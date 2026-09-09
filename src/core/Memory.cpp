#include "Memory.h"

namespace core {

Word Memory::read(unsigned int address) {
    address &= 0xFFFFu;
    u16 v = 0;
    cells_.get(address, v);          // absent cells read as zero
    ++readCount_;
    lastAddress_      = address;
    lastWasWrite_     = false;
    touchedThisCycle_ = true;
    return Word(v);
}

void Memory::write(unsigned int address, Word value) {
    address &= 0xFFFFu;
    cells_.put(address, value.raw());
    ++writeCount_;
    lastAddress_      = address;
    lastWasWrite_     = true;
    touchedThisCycle_ = true;
}

Word Memory::peek(unsigned int address) const {
    u16 v = 0;
    cells_.get(address & 0xFFFFu, v);
    return Word(v);
}

bool Memory::isSet(unsigned int address) const {
    return cells_.contains(address & 0xFFFFu);
}

void Memory::clear() {
    cells_.clear();
    readCount_  = 0;
    writeCount_ = 0;
    lastAddress_  = 0;
    lastWasWrite_ = false;
    touchedThisCycle_ = false;
}

void Memory::usedAddresses(ds::LinkedList<unsigned int>& out) const {
    out.clear();
    for (size_t b = 0; b < cells_.buckets(); ++b) {
        size_t depth = cells_.bucketDepth(b);
        for (size_t d = 0; d < depth; ++d) {
            unsigned int key = 0;
            u16          val = 0;
            if (cells_.bucketEntry(b, d, key, val)) out.pushBack(key);
        }
    }
    // ascending order for a readable memory dump
    out.sort(std::less<unsigned int>());
}

} // namespace core
