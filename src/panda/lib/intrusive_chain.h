#pragma once
#include <cassert>
#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <vector>

namespace panda { namespace lib {

template <typename T> struct IntrusiveChainNode {
    template <class S> friend S    intrusive_chain_next (const S&);
    template <class S> friend S    intrusive_chain_prev (const S&);
    template <class S> friend void intrusive_chain_next (const S&, const S&);
    template <class S> friend void intrusive_chain_prev (const S&, const S&);

    IntrusiveChainNode () : next(), prev() {}

private:
    T next;
    T prev;
};

template <class T> T    intrusive_chain_next (const T& node)               { return node->next; }
template <class T> T    intrusive_chain_prev (const T& node)               { return node->prev; }
template <class T> void intrusive_chain_next (const T& node, const T& val) { node->next = val; }
template <class T> void intrusive_chain_prev (const T& node, const T& val) { node->prev = val; }

/// Iterator is not cyclic, so to work with the standard bidirectional algorithms it uses head and tail pointers alongside with the current one.
/// Typical cyclic implementation uses an extra sentinel Node object, which we will definitely do not want to allocate.
template <typename T> struct IntrusiveChainIterator {
    template <typename S> friend struct IntrusiveChain;
    template <typename S> friend struct IntrusiveChainIterator;

    using difference_type   = ptrdiff_t;
    using value_type        = T;
    using pointer           = T*;
    using reference         = T&;
    using iterator_category = std::bidirectional_iterator_tag;

    IntrusiveChainIterator (const T& tail, const T& current = T()) : tail(tail), current(current) {}
    
    template <typename S>
    IntrusiveChainIterator (const IntrusiveChainIterator<S>& o) : tail(o.tail), current(o.current) {}

    T&   operator*  () { return current; }
    T*   operator-> () { return &current; }
    bool operator== (const IntrusiveChainIterator& other) const { return current == other.current; }
    bool operator!= (const IntrusiveChainIterator& other) const { return !(*this == other); }

    IntrusiveChainIterator& operator++ () {
        current = intrusive_chain_next(current);
        return *this;
    }

    IntrusiveChainIterator operator++ (int) {
        IntrusiveChainIterator pos(*this);
        ++*this;
        return pos;
    }

    IntrusiveChainIterator& operator-- () {
        if (current) current = intrusive_chain_prev(current);
        else         current = tail;
        return *this;
    }

    IntrusiveChainIterator operator-- (int) {
        IntrusiveChainIterator pos(*this);
        --*this;
        return pos;
    }

    IntrusiveChainIterator& operator= (const IntrusiveChainIterator& oth) {
        this->tail = oth.tail;
        this->current = oth.current;
        return *this;
    }

private:
    T tail;
    T current; // current=nullptr indicates end()
};

template <typename T> struct IntrusiveChain {
    using value_type             = T;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = T&;
    using const_reference        = const T&;
    using pointer                = T*;
    using const_pointer          = const T*;
    using iterator               = IntrusiveChainIterator<T>;
    using const_iterator         = IntrusiveChainIterator<T>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    IntrusiveChain () : head(), tail() {}

    IntrusiveChain (std::initializer_list<T> il) : IntrusiveChain() {
        for (const T& node : il) push_back(node);
    }

    ~IntrusiveChain () { clear(); }

    IntrusiveChain (const IntrusiveChain&) = delete;
    IntrusiveChain& operator= (const IntrusiveChain&) = delete;

    IntrusiveChain (IntrusiveChain&& other) noexcept {
        std::swap(head, other.head);
        std::swap(tail, other.tail);
    }

    IntrusiveChain& operator= (IntrusiveChain&& other) noexcept {
        if (this != &other) {
            std::swap(head, other.head);
            std::swap(tail, other.tail);
        } 
        return *this;
    }

    void push_back (const T& node) {
        if (empty()) {
            head = tail = node;
        } else {
            intrusive_chain_next(tail, node);
            intrusive_chain_prev(node, tail);
            intrusive_chain_next(node, T());
            tail = node;
        }
    }

    void push_front (const T& node) {
        if (empty()) {
            head = tail = node;
        } else {
            intrusive_chain_prev(head, node);
            intrusive_chain_prev(node, T());
            intrusive_chain_next(node, head);
            head = node;
        }
    }

    /// @return false if empty or the last one
    bool pop_back () {
        if (head == tail) {
            head = tail = T();
            return false;
        }
        auto removed = tail;
        tail = intrusive_chain_prev(tail);
        intrusive_chain_next(tail, T());
        intrusive_chain_prev(removed, T());
        return true;
    }

    /// @return false if empty or the last one
    bool pop_front () {
        if (head == tail) {
            head = tail = T();
            return false;
        }
        auto removed = head;
        head = intrusive_chain_next(head);
        intrusive_chain_prev(head, T());
        intrusive_chain_next(removed, T());
        return true;
    }

    const_iterator insert (const T& pos, const T& node) {
        if (pos) {
            auto prev = intrusive_chain_prev(pos);
            if (prev) {
                intrusive_chain_prev(node, prev);
                intrusive_chain_next(node, pos);
                intrusive_chain_next(prev, node);
                intrusive_chain_prev(pos, node);
            }
            else push_front(node);
        }
        else push_back(node);

        return const_iterator(tail, node);
    }

    const_iterator insert (const const_iterator& pos, const T& node) { return insert(pos.current, node); }

    const_iterator erase (const T& pos) {
        if (!pos) return end();

        auto prev = intrusive_chain_prev(pos);
        if (!prev) {
            pop_front();
            return cbegin();
        }

        auto next = intrusive_chain_next(pos);
        if (!next) {
            pop_back();
            return cend();
        }

        intrusive_chain_next(prev, next);
        intrusive_chain_prev(next, prev);
        intrusive_chain_next(pos, T());
        intrusive_chain_prev(pos, T());

        return const_iterator(tail, next);
    }

    const_iterator erase (const const_iterator& pos) { return erase(pos.current); }

    void clear () {
        for (auto node = head; node;) {
            auto next = intrusive_chain_next(node);
            intrusive_chain_next(node, T());
            intrusive_chain_prev(node, T());
            node = next;
        }
        head = tail = T();
    }

    reference front () { return head; }
    reference back  () { return tail; }

    const_reference front () const { return head; }
    const_reference back  () const { return tail; }

    iterator begin () { return iterator(tail, head); }
    iterator end   () { return iterator(tail); }

    const_iterator begin () const { return const_iterator(tail, head); }
    const_iterator end   () const { return const_iterator(tail); }
    
    const_iterator cbegin () const { return const_iterator(tail, head); }
    const_iterator cend   () const { return const_iterator(tail); }

    bool empty () const { return !head; }

    size_type size () const {
        size_type ret = 0;
        for (auto node = head; node; node = intrusive_chain_next(node)) ++ret;
        return ret;
    }

private:
    T head;
    T tail;
};

template <typename T> std::ostream& operator<< (std::ostream& out, const IntrusiveChain<T>& chain) {
    for (auto node : chain) {
        out << "node: ";
        if (node) {
            out << *node;
        } else {
            out << "null";
        }
        out << std::endl;
    }
    return out;
}

}}
