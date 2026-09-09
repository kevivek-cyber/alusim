// ---------------------------------------------------------------------------
// HashMap.h  --  our own hash table (no std::map / std::unordered_map)
//
// Used by : the assembler's label/symbol table, the opcode dispatch table,
//           and the sparse memory model.
// Covers  : PL Practical 11 (hashing -- all three collision strategies),
//           PSOOP Practical 10 (class template with two type parameters)
//
// Collision strategies are selectable so the hashing demo screen can show
// separate chaining, linear probing and quadratic probing side by side, which
// is exactly what PL Practical 11 (a), (b) and (c) ask for.
// ---------------------------------------------------------------------------
#ifndef DS_HASHMAP_H
#define DS_HASHMAP_H

#include <cstddef>
#include <string>
#include "../core/Exceptions.h"

namespace ds {

enum CollisionStrategy {
    SEPARATE_CHAINING,
    LINEAR_PROBING,
    QUADRATIC_PROBING
};

// -- default hash functors ---------------------------------------------------
template <typename K>
struct Hasher {
    size_t operator()(const K& key) const { return static_cast<size_t>(key); }
};

template <>
struct Hasher<std::string> {
    size_t operator()(const std::string& key) const {
        // simple polynomial rolling hash (folding method)
        size_t h = 0;
        for (size_t i = 0; i < key.size(); ++i)
            h = h * 31u + static_cast<unsigned char>(key[i]);
        return h;
    }
};

template <typename K, typename V, typename H = Hasher<K> >
class HashMap {
private:
    struct Entry {
        K      key;
        V      value;
        bool   occupied;
        bool   deleted;
        Entry* next;      // chain link (separate chaining only)
        Entry() : occupied(false), deleted(false), next(0) {}
        Entry(const K& k, const V& v)
            : key(k), value(v), occupied(true), deleted(false), next(0) {}
    };

    Entry**           table_;
    size_t            buckets_;
    size_t            count_;
    size_t            probes_;      // stats for the demo screen
    CollisionStrategy strategy_;
    H                 hash_;

public:
    explicit HashMap(size_t buckets = 31, CollisionStrategy s = SEPARATE_CHAINING)
        : table_(0), buckets_(buckets ? buckets : 1), count_(0), probes_(0), strategy_(s) {
        table_ = new Entry*[buckets_];
        for (size_t i = 0; i < buckets_; ++i) table_[i] = 0;
    }

    HashMap(const HashMap& other)
        : table_(0), buckets_(other.buckets_), count_(0), probes_(0),
          strategy_(other.strategy_) {
        table_ = new Entry*[buckets_];
        for (size_t i = 0; i < buckets_; ++i) table_[i] = 0;
        other.forEachInto(*this);
    }

    HashMap& operator=(const HashMap& other) {
        if (this != &other) {
            destroy();
            buckets_  = other.buckets_;
            strategy_ = other.strategy_;
            count_    = 0;
            probes_   = 0;
            table_    = new Entry*[buckets_];
            for (size_t i = 0; i < buckets_; ++i) table_[i] = 0;
            other.forEachInto(*this);
        }
        return *this;
    }

    ~HashMap() { destroy(); }

    // -- core operations -----------------------------------------------------
    void put(const K& key, const V& value) {
        size_t home = hash_(key) % buckets_;

        if (strategy_ == SEPARATE_CHAINING) {
            for (Entry* e = table_[home]; e != 0; e = e->next) {
                ++probes_;
                if (e->key == key) { e->value = value; return; }
            }
            Entry* e = new Entry(key, value);
            e->next = table_[home];
            table_[home] = e;
            ++count_;
            return;
        }

        // open addressing (linear / quadratic probing)
        for (size_t i = 0; i < buckets_; ++i) {
            size_t idx = probeIndex(home, i);
            ++probes_;
            Entry* e = table_[idx];
            if (e == 0) {
                table_[idx] = new Entry(key, value);
                ++count_;
                return;
            }
            if (e->occupied && e->key == key) { e->value = value; return; }
            if (!e->occupied) {                       // reuse a tombstone
                e->key = key; e->value = value;
                e->occupied = true; e->deleted = false;
                ++count_;
                return;
            }
        }
        throw core::IndexOutOfRangeException("HashMap is full (open addressing)");
    }

    bool get(const K& key, V& out) const {
        Entry* e = find(key);
        if (e == 0) return false;
        out = e->value;
        return true;
    }

    bool contains(const K& key) const { return find(key) != 0; }

    V& operator[](const K& key) {
        Entry* e = find(key);
        if (e == 0) { put(key, V()); e = find(key); }
        return e->value;
    }

    bool remove(const K& key) {
        size_t home = hash_(key) % buckets_;
        if (strategy_ == SEPARATE_CHAINING) {
            Entry* prev = 0;
            for (Entry* e = table_[home]; e != 0; prev = e, e = e->next) {
                if (e->key == key) {
                    if (prev) prev->next = e->next; else table_[home] = e->next;
                    delete e;
                    --count_;
                    return true;
                }
            }
            return false;
        }
        for (size_t i = 0; i < buckets_; ++i) {
            size_t idx = probeIndex(home, i);
            Entry* e = table_[idx];
            if (e == 0) return false;
            if (e->occupied && e->key == key) {
                e->occupied = false;
                e->deleted  = true;      // tombstone keeps probe chains intact
                --count_;
                return true;
            }
        }
        return false;
    }

    // -- introspection used by the hashing demo screen ------------------------
    size_t buckets()    const { return buckets_; }
    size_t size()       const { return count_; }
    bool   empty()      const { return count_ == 0; }
    size_t probeCount() const { return probes_; }
    void   resetProbeCount()  { probes_ = 0; }
    double loadFactor() const { return static_cast<double>(count_) / static_cast<double>(buckets_); }
    CollisionStrategy strategy() const { return strategy_; }

    // walk one bucket's chain (separate chaining) or its single slot
    size_t bucketDepth(size_t idx) const {
        if (idx >= buckets_) return 0;
        if (strategy_ == SEPARATE_CHAINING) {
            size_t n = 0;
            for (Entry* e = table_[idx]; e != 0; e = e->next) ++n;
            return n;
        }
        return (table_[idx] && table_[idx]->occupied) ? 1 : 0;
    }

    bool bucketEntry(size_t idx, size_t depth, K& k, V& v) const {
        if (idx >= buckets_) return false;
        if (strategy_ == SEPARATE_CHAINING) {
            Entry* e = table_[idx];
            for (size_t i = 0; i < depth && e; ++i) e = e->next;
            if (!e) return false;
            k = e->key; v = e->value;
            return true;
        }
        if (depth != 0) return false;
        Entry* e = table_[idx];
        if (!e || !e->occupied) return false;
        k = e->key; v = e->value;
        return true;
    }

    void clear() {
        destroy();
        table_ = new Entry*[buckets_];
        for (size_t i = 0; i < buckets_; ++i) table_[i] = 0;
        count_  = 0;
        probes_ = 0;
    }

private:
    size_t probeIndex(size_t home, size_t i) const {
        if (strategy_ == QUADRATIC_PROBING) return (home + i * i) % buckets_;
        return (home + i) % buckets_;                       // linear probing
    }

    Entry* find(const K& key) const {
        size_t home = hash_(key) % buckets_;
        if (strategy_ == SEPARATE_CHAINING) {
            for (Entry* e = table_[home]; e != 0; e = e->next)
                if (e->key == key) return e;
            return 0;
        }
        for (size_t i = 0; i < buckets_; ++i) {
            size_t idx = probeIndex(home, i);
            Entry* e = table_[idx];
            if (e == 0) return 0;
            if (e->occupied && e->key == key) return e;
        }
        return 0;
    }

    void forEachInto(HashMap& dest) const {
        for (size_t b = 0; b < buckets_; ++b) {
            if (strategy_ == SEPARATE_CHAINING) {
                for (Entry* e = table_[b]; e != 0; e = e->next) dest.put(e->key, e->value);
            } else if (table_[b] && table_[b]->occupied) {
                dest.put(table_[b]->key, table_[b]->value);
            }
        }
    }

    void destroy() {
        if (!table_) return;
        for (size_t b = 0; b < buckets_; ++b) {
            Entry* e = table_[b];
            while (e) { Entry* nx = e->next; delete e; e = nx; }
        }
        delete[] table_;
        table_ = 0;
    }
};

} // namespace ds

#endif // DS_HASHMAP_H
