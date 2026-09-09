// ---------------------------------------------------------------------------
// LinkedList.h  --  our own singly linked list (no std::list)
//
// Covers: PL Practical 3 (singly linked list), PSOOP Practical 10 (templates),
//         PSOOP Practical 4 (dynamic memory: new/delete, pointers)
// ---------------------------------------------------------------------------
#ifndef DS_LINKEDLIST_H
#define DS_LINKEDLIST_H

#include <cstddef>
#include "../core/Exceptions.h"

namespace ds {

template <typename T>
class LinkedList {
private:
    struct Node {
        T     data;
        Node* next;
        explicit Node(const T& d) : data(d), next(0) {}
    };

    Node*  head_;
    Node*  tail_;
    size_t size_;

public:
    LinkedList() : head_(0), tail_(0), size_(0) {}

    // deep copy -- PSOOP: copy constructor
    LinkedList(const LinkedList& other) : head_(0), tail_(0), size_(0) {
        for (Node* n = other.head_; n != 0; n = n->next) pushBack(n->data);
    }

    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            clear();
            for (Node* n = other.head_; n != 0; n = n->next) pushBack(n->data);
        }
        return *this;
    }

    ~LinkedList() { clear(); }

    void pushBack(const T& value) {
        Node* n = new Node(value);
        if (tail_ == 0) { head_ = tail_ = n; }
        else            { tail_->next = n; tail_ = n; }
        ++size_;
    }

    void pushFront(const T& value) {
        Node* n = new Node(value);
        n->next = head_;
        head_ = n;
        if (tail_ == 0) tail_ = n;
        ++size_;
    }

    // remove first element equal to value; returns true if something was removed
    bool remove(const T& value) {
        Node* prev = 0;
        for (Node* cur = head_; cur != 0; prev = cur, cur = cur->next) {
            if (cur->data == value) {
                if (prev == 0) head_ = cur->next;
                else           prev->next = cur->next;
                if (cur == tail_) tail_ = prev;
                delete cur;
                --size_;
                return true;
            }
        }
        return false;
    }

    bool contains(const T& value) const {
        for (Node* n = head_; n != 0; n = n->next)
            if (n->data == value) return true;
        return false;
    }

    T& at(size_t index) {
        if (index >= size_) throw core::IndexOutOfRangeException("LinkedList::at");
        Node* n = head_;
        for (size_t i = 0; i < index; ++i) n = n->next;
        return n->data;
    }

    const T& at(size_t index) const {
        if (index >= size_) throw core::IndexOutOfRangeException("LinkedList::at");
        Node* n = head_;
        for (size_t i = 0; i < index; ++i) n = n->next;
        return n->data;
    }

    // insertion sort over the links (used by the demo screens)
    template <typename Compare>
    void sort(Compare less) {
        if (size_ < 2) return;
        Node* sorted = 0;
        Node* cur    = head_;
        while (cur != 0) {
            Node* next = cur->next;
            if (sorted == 0 || less(cur->data, sorted->data)) {
                cur->next = sorted;
                sorted = cur;
            } else {
                Node* s = sorted;
                while (s->next != 0 && !less(cur->data, s->next->data)) s = s->next;
                cur->next = s->next;
                s->next   = cur;
            }
            cur = next;
        }
        head_ = sorted;
        tail_ = head_;
        while (tail_ != 0 && tail_->next != 0) tail_ = tail_->next;
    }

    void clear() {
        Node* n = head_;
        while (n != 0) { Node* nx = n->next; delete n; n = nx; }
        head_ = tail_ = 0;
        size_ = 0;
    }

    size_t size()  const { return size_; }
    bool   empty() const { return size_ == 0; }

    // lightweight forward iteration without exposing Node
    class Iterator {
        Node* cur_;
    public:
        explicit Iterator(Node* n) : cur_(n) {}
        bool      hasNext() const { return cur_ != 0; }
        T&        next()          { T& v = cur_->data; cur_ = cur_->next; return v; }
    };
    Iterator iterator() { return Iterator(head_); }

    class ConstIterator {
        const Node* cur_;
    public:
        explicit ConstIterator(const Node* n) : cur_(n) {}
        bool     hasNext() const { return cur_ != 0; }
        const T& next()          { const T& v = cur_->data; cur_ = cur_->next; return v; }
    };
    ConstIterator iterator() const { return ConstIterator(head_); }
};

} // namespace ds

#endif // DS_LINKEDLIST_H
