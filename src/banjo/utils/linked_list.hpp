#ifndef BANJO_UTILS_LINKED_LIST_H
#define BANJO_UTILS_LINKED_LIST_H

#include <cassert>
#include <utility>

namespace banjo {

template <typename T>
struct LinkedListNode {
    T value;
    LinkedListNode *prev = nullptr;
    LinkedListNode *next = nullptr;
    LinkedListNode(T value) : value(std::move(value)) {}
};

template <typename T>
class LinkedListIter {

    template <typename U>
    friend class LinkedList;

private:
    LinkedListNode<T> *node;

public:
    LinkedListIter(LinkedListNode<T> *node) : node(node) {}
    LinkedListIter() : node(nullptr) {}

    LinkedListNode<T> *get_node() const { return node; }
    LinkedListIter get_prev() { return node->prev; }
    LinkedListIter get_next() { return node->next; }

    friend bool operator==(const LinkedListIter &rhs, const LinkedListIter &lhs) { return rhs.node == lhs.node; }
    friend bool operator!=(const LinkedListIter &rhs, const LinkedListIter &lhs) { return !(rhs == lhs); }
    friend bool operator<(const LinkedListIter &rhs, const LinkedListIter &lhs) { return lhs.node < rhs.node; }
    friend bool operator<=(const LinkedListIter &rhs, const LinkedListIter &lhs) { return lhs.node <= rhs.node; }
    friend bool operator>(const LinkedListIter &rhs, const LinkedListIter &lhs) { return lhs.node > rhs.node; }
    friend bool operator>=(const LinkedListIter &rhs, const LinkedListIter &lhs) { return lhs.node >= rhs.node; }

    LinkedListIter &operator++() {
        node = node->next;
        return *this;
    }

    LinkedListIter &operator--() {
        node = node->prev;
        return *this;
    }

    T &operator*() { return node->value; }
    const T &operator*() const { return node->value; }

    T *operator->() { return &node->value; }
    const T *operator->() const { return &node->value; }

    operator bool() const { return node; }
    bool operator!() const { return !node; }
};

template <typename T>
class LinkedList {

private:
    LinkedListNode<T> *header;
    LinkedListNode<T> *trailer;
    unsigned size;

public:
    LinkedList() : size(0) {
        header = new LinkedListNode<T>({});
        trailer = new LinkedListNode<T>({});

        header->prev = nullptr;
        header->next = trailer;

        trailer->prev = header;
        trailer->next = nullptr;
    }

    LinkedList(const LinkedList &other) : LinkedList() {
        for (T value : other) {
            append(value);
        }
    }

    LinkedList(LinkedList &&other) noexcept
      : header(std::exchange(other.header, nullptr)),
        trailer(std::exchange(other.trailer, nullptr)),
        size(other.size) {}

    ~LinkedList() {
        if (!header) {
            return;
        }

        for (LinkedListIter<T> iter = header->next; iter; ++iter) {
            delete iter.get_prev().node;
        }

        delete trailer;
    }

    LinkedList &operator=(const LinkedList &other) { return *this = LinkedList(other); }

    LinkedList &operator=(LinkedList &&other) noexcept {
        std::swap(header, other.header);
        std::swap(trailer, other.trailer);
        size = other.size;
        return *this;
    }

    LinkedListIter<T> append(T value) { return append(create_iter(std::move(value))); }

    LinkedListIter<T> insert_before(LinkedListIter<T> iter, T value) { return insert_before(iter, create_iter(value)); }
    LinkedListIter<T> insert_after(LinkedListIter<T> iter, T value) { return insert_after(iter, create_iter(value)); }
    LinkedListIter<T> replace(LinkedListIter<T> iter, T value) { return replace(iter, create_iter(value)); }
    LinkedListIter<T> append(LinkedListIter<T> new_iter) { return insert_before(trailer, new_iter); }

    LinkedListIter<T> insert_before(LinkedListIter<T> iter, LinkedListIter<T> new_iter) {
        new_iter.node->next = iter.node;
        new_iter.node->prev = iter.node->prev;
        iter.node->prev->next = new_iter.node;
        iter.node->prev = new_iter.node;

        size += 1;
        return new_iter;
    }

    LinkedListIter<T> insert_after(LinkedListIter<T> iter, LinkedListIter<T> new_iter) {
        new_iter.node->prev = iter.node;
        new_iter.node->next = iter.node->next;
        iter.node->next->prev = new_iter.node;
        iter.node->next = new_iter.node;

        size += 1;
        return new_iter.node;
    }

    void remove(LinkedListIter<T> iter) {
        assert(iter != header && iter != trailer);

        iter.node->prev->next = iter.node->next;
        iter.node->next->prev = iter.node->prev;

        delete iter.node;
        size -= 1;
    }

    LinkedListIter<T> replace(LinkedListIter<T> iter, LinkedListIter<T> new_iter) {
        assert(iter != header && iter != trailer);

        new_iter.node->prev = iter.node->prev;
        new_iter.node->next = iter.node->next;
        iter.node->prev->next = new_iter.node;
        iter.node->next->prev = new_iter.node;

        delete iter.node;
        return new_iter.node;
    }

    LinkedListIter<T> create_iter(T value) { return new LinkedListNode<T>(std::move(value)); }

    LinkedListIter<T> get_header() const { return header; }
    LinkedListIter<T> get_trailer() const { return trailer; }
    unsigned get_size() const { return size; }

    LinkedListIter<T> get_first_iter() const { return header->next; }
    LinkedListIter<T> get_last_iter() const { return trailer->prev; }
    T &get_first() const { return *get_first_iter(); }
    T &get_last() const { return *get_last_iter(); }

    LinkedListIter<T> begin() const { return header->next; }
    LinkedListIter<T> end() const { return trailer; }
};

} // namespace banjo

template <typename T>
struct std::hash<banjo::LinkedListIter<T>> {
    std::size_t operator()(const banjo::LinkedListIter<T> &iter) const noexcept { return (std::size_t)iter.get_node(); }
};

#endif
