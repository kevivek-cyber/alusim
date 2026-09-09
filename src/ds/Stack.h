// ---------------------------------------------------------------------------
// Stack.h  --  our own stack (no std::stack)
//
// Used by : the CPU call stack (CALL/RET, PUSH/POP) and the expression
//           evaluator demo screen.
// Covers  : PL Practical 6 (stack, expression conversion/evaluation),
//           PSOOP Practical 10 (class template)
// ---------------------------------------------------------------------------
#ifndef DS_STACK_H
#define DS_STACK_H

#include <cstddef>
#include "../core/Exceptions.h"

namespace ds {

template <typename T>
class Stack {
private:
    struct Node {
        T     data;
        Node* below;
        Node(const T& d, Node* b) : data(d), below(b) {}
    };

    Node*  top_;
    size_t size_;
    size_t capacity_;   // 0 means "unbounded"

public:
    explicit Stack(size_t capacity = 0) : top_(0), size_(0), capacity_(capacity) {}

    Stack(const Stack& other) : top_(0), size_(0), capacity_(other.capacity_) {
        // copy preserving order: walk source top->bottom into a temp, then push back
        if (other.top_ == 0) return;
        size_t n = other.size_;
        T* buf = new T[n];
        size_t i = 0;
        for (Node* p = other.top_; p != 0; p = p->below) buf[i++] = p->data;
        for (size_t k = n; k > 0; --k) push(buf[k - 1]);
        delete[] buf;
    }

    Stack& operator=(const Stack& other) {
        if (this != &other) { Stack tmp(other); swap(tmp); }
        return *this;
    }

    ~Stack() { clear(); }

    void swap(Stack& other) {
        Node*  t = top_;      top_      = other.top_;      other.top_      = t;
        size_t s = size_;     size_     = other.size_;     other.size_     = s;
        size_t c = capacity_; capacity_ = other.capacity_; other.capacity_ = c;
    }

    void push(const T& value) {
        if (capacity_ != 0 && size_ >= capacity_)
            throw core::StackOverflowException("push beyond capacity");
        top_ = new Node(value, top_);
        ++size_;
    }

    T pop() {
        if (top_ == 0) throw core::StackUnderflowException("pop on empty stack");
        Node* n = top_;
        T value = n->data;
        top_ = n->below;
        delete n;
        --size_;
        return value;
    }

    T& peek() {
        if (top_ == 0) throw core::StackUnderflowException("peek on empty stack");
        return top_->data;
    }

    const T& peek() const {
        if (top_ == 0) throw core::StackUnderflowException("peek on empty stack");
        return top_->data;
    }

    void clear() {
        while (top_ != 0) { Node* n = top_->below; delete top_; top_ = n; }
        size_ = 0;
    }

    size_t size()  const { return size_; }
    bool   empty() const { return size_ == 0; }

    // top -> bottom traversal, used by the UI to draw the stack
    class ConstIterator {
        const Node* cur_;
    public:
        explicit ConstIterator(const Node* n) : cur_(n) {}
        bool     hasNext() const { return cur_ != 0; }
        const T& next()          { const T& v = cur_->data; cur_ = cur_->below; return v; }
    };
    ConstIterator iterator() const { return ConstIterator(top_); }
};

} // namespace ds

#endif // DS_STACK_H
