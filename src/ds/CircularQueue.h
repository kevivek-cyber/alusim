// ---------------------------------------------------------------------------
// CircularQueue.h  --  our own array-backed circular queue (no std::queue)
//
// Used by : the instruction fetch queue and the pipeline stage buffer;
//           also drives the Round-Robin scheduler demo screen.
// Covers  : PL Practical 8 (circular queue, Round Robin CPU scheduling),
//           PSOOP Practical 10 (class template),
//           PSOOP Practical 4 (dynamic memory: new[]/delete[])
// ---------------------------------------------------------------------------
#ifndef DS_CIRCULARQUEUE_H
#define DS_CIRCULARQUEUE_H

#include <cstddef>
#include "../core/Exceptions.h"

namespace ds {

template <typename T>
class CircularQueue {
private:
    T*     buf_;
    size_t capacity_;
    size_t front_;
    size_t count_;

public:
    explicit CircularQueue(size_t capacity = 16)
        : buf_(new T[capacity ? capacity : 1]),
          capacity_(capacity ? capacity : 1),
          front_(0), count_(0) {}

    CircularQueue(const CircularQueue& other)
        : buf_(new T[other.capacity_]), capacity_(other.capacity_),
          front_(0), count_(0) {
        for (size_t i = 0; i < other.count_; ++i) enqueue(other.at(i));
    }

    CircularQueue& operator=(const CircularQueue& other) {
        if (this != &other) {
            delete[] buf_;
            buf_      = new T[other.capacity_];
            capacity_ = other.capacity_;
            front_    = 0;
            count_    = 0;
            for (size_t i = 0; i < other.count_; ++i) enqueue(other.at(i));
        }
        return *this;
    }

    ~CircularQueue() { delete[] buf_; }

    void enqueue(const T& value) {
        if (count_ == capacity_)
            throw core::IndexOutOfRangeException("CircularQueue::enqueue on full queue");
        buf_[(front_ + count_) % capacity_] = value;
        ++count_;
    }

    T dequeue() {
        if (count_ == 0)
            throw core::IndexOutOfRangeException("CircularQueue::dequeue on empty queue");
        T value = buf_[front_];
        front_  = (front_ + 1) % capacity_;
        --count_;
        return value;
    }

    T& front() {
        if (count_ == 0) throw core::IndexOutOfRangeException("CircularQueue::front on empty queue");
        return buf_[front_];
    }

    // logical index 0 == front
    const T& at(size_t i) const {
        if (i >= count_) throw core::IndexOutOfRangeException("CircularQueue::at");
        return buf_[(front_ + i) % capacity_];
    }

    T& at(size_t i) {
        if (i >= count_) throw core::IndexOutOfRangeException("CircularQueue::at");
        return buf_[(front_ + i) % capacity_];
    }

    void   clear()    { front_ = 0; count_ = 0; }
    size_t size()     const { return count_; }
    size_t capacity() const { return capacity_; }
    bool   empty()    const { return count_ == 0; }
    bool   full()     const { return count_ == capacity_; }
};

} // namespace ds

#endif // DS_CIRCULARQUEUE_H
