// ---------------------------------------------------------------------------
// Memory.h  --  sparse main memory
//
// A full 64K x 16-bit array would be wasteful when a demo program touches a
// handful of cells, so memory is stored sparsely in our own hash table and only
// the cells actually written are kept.  That is the same idea as the sparse
// matrix representation from the Data Structures syllabus.
//
// Covers: PL Practical 2 (sparse representation), PL Practical 11 (hashing),
//         COA Unit 1     (memory + MAR/MDR interface)
// ---------------------------------------------------------------------------
#ifndef CORE_MEMORY_H
#define CORE_MEMORY_H

#include <cstddef>
#include "Word.h"
#include "../ds/HashMap.h"
#include "../ds/LinkedList.h"

namespace core {

class Memory {
private:
    ds::HashMap<unsigned int, u16> cells_;   // address -> value, sparse
    unsigned long readCount_;
    unsigned long writeCount_;
    unsigned int  lastAddress_;
    bool          lastWasWrite_;
    bool          touchedThisCycle_;

public:
    static const unsigned int SIZE = 0x10000;   // 64K addressable words

    Memory() : cells_(1021, ds::SEPARATE_CHAINING),
               readCount_(0), writeCount_(0),
               lastAddress_(0), lastWasWrite_(false), touchedThisCycle_(false) {}

    Word read(unsigned int address);
    void write(unsigned int address, Word value);

    // read without counting as a bus access (used by the UI to draw memory)
    Word peek(unsigned int address) const;
    bool isSet(unsigned int address) const;

    void clear();

    // Collect the addresses that currently hold data, in ascending order,
    // so the memory viewer can list only the interesting cells.
    void usedAddresses(ds::LinkedList<unsigned int>& out) const;

    unsigned long readCount()  const { return readCount_; }
    unsigned long writeCount() const { return writeCount_; }
    size_t        cellsUsed()  const { return cells_.size(); }

    unsigned int lastAddress()  const { return lastAddress_; }
    bool         lastWasWrite() const { return lastWasWrite_; }
    bool         touched()      const { return touchedThisCycle_; }
    void         clearActivity()      { touchedThisCycle_ = false; }
};

} // namespace core

#endif // CORE_MEMORY_H
