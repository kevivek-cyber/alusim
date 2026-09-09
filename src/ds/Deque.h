// ---------------------------------------------------------------------------
// Deque.h  --  our own doubly-ended queue on a doubly linked list
//
// Used by : the clock-cycle history scrubber (step backward / forward through
//           recorded cycles) and the input/output-restricted deque demo.
// Covers  : PL Practical 9 (deque, input/output restricted),
//           PL Practical 4 (doubly linked list),
//           PSOOP Practical 10 (class template)
// ---------------------------------------------------------------------------
#ifndef DS_DEQUE_H
#define DS_DEQUE_H

#include <cstddef>
#include "../core/Exceptions.h"

namespace ds {

// Restriction modes for the shopping-mall demo (PL Practical 9).
enum DequeMode {
    DEQUE_FULL,               // both ends open
    DEQUE_INPUT_RESTRICTED,   // insert at rear only; delete at both ends
    DEQUE_OUTPUT_RESTRICTED   // insert at both ends; delete at front only
};

template <typename T>
class Deque {
private:
    struct Node {
        T     data;
        Node* prev;
        Node* next;
        explicit Node(const T& d) : data(d), prev(0), next(0) {}
    };

    Node*     head_;
    Node*     tail_;
    size_t    size_;
    size_t    capacity_;   // 0 == unbounded
    DequeMode mode_;

public:
    explicit Deque(size_t capacity = 0, DequeMode mode = DEQUE_FULL)
        : head_(0), tail_(0), size_(0), capacity_(capacity), mode_(mode) {}

    Deque(const Deque& other)
        : head_(0), tail_(0), size_(0),
          capacity_(other.capacity_), mode_(DEQUE_FULL) {
        for (Node* n = other.head_; n != 0; n = n->next) pushBack(n->data);
        mode_ = other.mode_;
    }

    Deque& operator=(const Deque& other) {
        if (this != &other) {
            clear();
            capacity_ = other.capacity_;
            mode_     = DEQUE_FULL;
            for (Node* n = other.head_; n != 0; n = n->next) pushBack(n->data);
            mode_ = other.mode_;
        }
        return *this;
    }

    ~Deque() { clear(); }

    void setMode(DequeMode m) { mode_ = m; }
    DequeMode mode() const    { return mode_; }

    void pushFront(const T& value) {
        if (mode_ == DEQUE_INPUT_RESTRICTED)
            throw core::SimulatorException("pushFront blocked: input-restricted deque");
        checkSpace();
        Node* n = new Node(value);
        n->next = head_;
        if (head_) head_->prev = n; else tail_ = n;
        head_ = n;
        ++size_;
    }

    void pushBack(const T& value) {
        checkSpace();
        Node* n = new Node(value);
        n->prev = tail_;
        if (tail_) tail_->next = n; else head_ = n;
        tail_ = n;
        ++size_;
    }

    T popFront() {
        if (size_ == 0) throw core::IndexOutOfRangeException("Deque::popFront on empty deque");
        Node* n = head_;
        T value = n->data;
        head_ = n->next;
        if (head_) head_->prev = 0; else tail_ = 0;
        delete n;
        --size_;
        return value;
    }

    T popBack() {
        if (mode_ == DEQUE_OUTPUT_RESTRICTED)
            throw core::SimulatorException("popBack blocked: output-restricted deque");
        if (size_ == 0) throw core::IndexOutOfRangeException("Deque::popBack on empty deque");
        Node* n = tail_;
        T value = n->data;
        tail_ = n->prev;
        if (tail_) tail_->next = 0; else head_ = 0;
        delete n;
        --size_;
        return value;
    }

    T& front() {
        if (size_ == 0) throw core::IndexOutOfRangeException("Deque::front on empty deque");
        return head_->data;
    }

    T& back() {
        if (size_ == 0) throw core::IndexOutOfRangeException("Deque::back on empty deque");
        return tail_->data;
    }

    const T& at(size_t i) const {
        if (i >= size_) throw core::IndexOutOfRangeException("Deque::at");
        const Node* n = head_;
        for (size_t k = 0; k < i; ++k) n = n->next;
        return n->data;
    }

    void clear() {
        Node* n = head_;
        while (n) { Node* nx = n->next; delete n; n = nx; }
        head_ = tail_ = 0;
        size_ = 0;
    }

    size_t size()  const { return size_; }
    bool   empty() const { return size_ == 0; }

private:
    void checkSpace() {
        if (capacity_ != 0 && size_ >= capacity_)
            throw core::IndexOutOfRangeException("Deque is full");
    }
};

} // namespace ds

#endif // DS_DEQUE_H
