#ifndef LINKED_LIST_HPP_
#define LINKED_LIST_HPP_

#include "containers/allocator.hpp"
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <utility>

template <typename T>
struct LinkedList {
    struct Node {
        T value;
        std::uintptr_t np = 0; // Bitwise XOR representation: prev ^ next
    };

    Node* head = nullptr;
    Node* tail = nullptr;
    std::size_t size = 0;
};

inline std::uintptr_t to_uint(void* ptr) 
{
    return reinterpret_cast<std::uintptr_t>(ptr);
}

template <typename NodePtr>
NodePtr to_ptr(std::uintptr_t val) 
{
    return reinterpret_cast<NodePtr>(val);
}

// --- Core API Functions ---
template <typename T>
void init(LinkedList<T> &self) 
{
    self.head = nullptr;
    self.tail = nullptr;
    self.size = 0;
}

template <typename T>
void append(LinkedList<T> &self, Allocator &allocator, const T &value) 
{
    using Node = typename LinkedList<T>::Node;
    void* raw_mem = allocator.allocate(alignof(Node), sizeof(Node));
    auto* new_node = static_cast<Node*>(raw_mem);
    
    new (&new_node->value) T(value);
    
    if (!self.tail) {
        new_node->np = 0; 
        self.head = new_node;
    } else {
        new_node->np = to_uint(self.tail) ^ 0;
        self.tail->np = self.tail->np ^ to_uint(new_node);
    }
    
    self.tail = new_node;
    self.size++;
}

template <typename T>
void append(LinkedList<T> &self, Allocator &allocator, T &&value) 
{
    using Node = typename LinkedList<T>::Node;
    void* raw_mem = allocator.allocate(alignof(Node), sizeof(Node));
    auto* new_node = static_cast<Node*>(raw_mem);
    
    new (&new_node->value) T(std::move(value));
    
    if (!self.tail) {
        new_node->np = 0;
        self.head = new_node;
    } else {
        new_node->np = to_uint(self.tail) ^ 0;
        self.tail->np = self.tail->np ^ to_uint(new_node);
    }
    
    self.tail = new_node;
    self.size++;
}

template <typename T>
void prepend(LinkedList<T> &self, Allocator &allocator, const T &value) 
{
    using Node = typename LinkedList<T>::Node;
    void* raw_mem = allocator.allocate(alignof(Node), sizeof(Node));
    auto* new_node = static_cast<Node*>(raw_mem);
    
    new (&new_node->value) T(value);
    
    if (!self.head) {
        new_node->np = 0;
        self.tail = new_node;
    } else {
        new_node->np = 0 ^ to_uint(self.head);
        self.head->np = to_uint(new_node) ^ self.head->np;
    }
    
    self.head = new_node;
    self.size++;
}

template <typename T>
bool pop(LinkedList<T> &self, Allocator &allocator) 
{
    if (!self.tail) return false;

    using Node = typename LinkedList<T>::Node;
    Node* node_to_remove = self.tail;
    Node* prev_node = to_ptr<Node*>(node_to_remove->np);

    if (prev_node) {
        prev_node->np = prev_node->np ^ to_uint(node_to_remove);
        self.tail = prev_node;
    } else {
        self.head = nullptr;
        self.tail = nullptr;
    }

    node_to_remove->value.~T();
    allocator.free(node_to_remove, sizeof(Node));
    self.size--;
    return true;
}

template <typename T>
bool pop_front(LinkedList<T> &self, Allocator &allocator) 
{
    if (!self.head) return false;

    using Node = typename LinkedList<T>::Node;
    Node* node_to_remove = self.head;
    Node* next_node = to_ptr<Node*>(node_to_remove->np);

    if (next_node) {
        next_node->np = to_uint(node_to_remove) ^ next_node->np;
        self.head = next_node;
    } else {
        self.head = nullptr;
        self.tail = nullptr;
    }

    node_to_remove->value.~T();
    allocator.free(node_to_remove, sizeof(Node));
    self.size--;
    return true;
}

template <typename T>
void clear(LinkedList<T> &self, Allocator &allocator) 
{
    using Node = typename LinkedList<T>::Node;
    Node* current = self.head;
    std::uintptr_t prev_addr = 0;

    while (current) {
        std::uintptr_t next_addr = current->np ^ prev_addr;
        Node* next = to_ptr<Node*>(next_addr);
        
        current->value.~T();
        allocator.free(current, sizeof(Node));
        
        prev_addr = to_uint(current);
        current = next;
    }
    
    self.head = nullptr;
    self.tail = nullptr;
    self.size = 0;
}

template <typename T>
void deinit(LinkedList<T> &self, Allocator &allocator) 
{
    clear(self, allocator);
}

// --- Iterator Support ---
template <typename T>
struct LinkedListIterator {
    using Node = typename LinkedList<T>::Node;
    
    Node* current = nullptr;
    std::uintptr_t prev_addr = 0;

    LinkedListIterator& operator++() 
    {
        if (current) {
            std::uintptr_t next_addr = current->np ^ prev_addr;
            prev_addr = to_uint(current);
            current = to_ptr<Node*>(next_addr);
        }
        return *this;
    }

    T& operator*() const 
    { 
        return current->value; 
    }
    
    Node* operator->() const 
    { 
        return current; 
    }

    bool operator==(const LinkedListIterator& other) const 
    { 
        return current == other.current; 
    }
    
    bool operator!=(const LinkedListIterator& other) const 
    { 
        return current != other.current; 
    }
};

template <typename T>
LinkedListIterator<T> begin(const LinkedList<T> &self) 
{
    return LinkedListIterator<T>{ self.head, 0 };
}

template <typename T>
LinkedListIterator<T> end(const LinkedList<T> &) 
{
    return LinkedListIterator<T>{ nullptr, 0 };
}

template <typename T>
std::ostream &operator<<(std::ostream &os, LinkedList<T> &self)
{
    os << "[";
    auto it = begin(self);
    auto it_end = end(self);
    
    while (it != it_end) {
        os << *it;
        ++it;
        if (it != it_end) {
            os << ", ";
        }
    }
    
    os << "]";
    return os;
}

#endif // !LINKED_LIST_HPP_
