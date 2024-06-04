#pragma once

#include <iostream>
#include <stack>
#include <mutex>
#include <memory>
#include <atomic>

namespace ts {

template < class T >
class stack {
public:
    inline stack() { }
    inline stack(const stack& other) {
        std::lock_guard<std::mutex> m(other._m);
        this->_data = other._data;
    }
    stack& operator=(const stack&) = delete;

    inline void push(T value) {

        std::lock_guard<std::mutex> lk(this->_m);
        _data.push(std::move(value));
    }

    inline bool pop(T& value) {

        std::unique_lock<std::mutex> lk(this->_m);
        if(_data.empty()) return false;
        value = std::move(_data.top());
        _data.pop();
        lk.unlock();
        return true;
    }

    inline std::shared_ptr<T> pop() {

        std::unique_lock<std::mutex> lk(this->_m);
        if (_data.empty()) return std::shared_ptr<T>();
        auto value = std::make_shared<T>(std::move(_data.top()));
        _data.pop();
        lk.unlock();
        return value;
    }

    inline bool empty() const {
        std::lock_guard<std::mutex> lk(this->_m);
        return _data.empty();
    }

    inline int size() const {
        std::lock_guard<std::mutex> lk(this->_m);
        return _data.size();
    }

private:
    std::stack<T> _data;
    mutable std::mutex _m;
};

namespace lock_free {
template < class T >
class stack {
private:
    struct node;
    struct counted_node {
        int external_count;
        node* ptr;
    };
    struct node {
        std::shared_ptr<T> _data;
        std::atomic<int> _internal_count;
        counted_node _next;
        // Used for allocation and deallocation test
        static int _allocated_num, _deallocated_num;
        node (const T& data): 
            _data(std::make_shared<T>(data)), 
            _internal_count(0), _next() { }
        void* operator new(std::size_t count) {
            auto ptr = ::operator new(count);
            ++_allocated_num;
            return ptr;
        }
        void operator delete(void* ptr) {
            ::operator delete(static_cast<node*>(ptr));
            ++_deallocated_num;
        }
    };
public:
    static bool is_mem_operation_correct() {
        return node::_allocated_num == node::_deallocated_num;
    }
private:
    std::atomic<counted_node> _head;
public:
    stack() {
        counted_node n = { 0, nullptr };
        _head.store(n);
    }
    // Make sure no thread now is accessing current stack instance
    ~stack() { 
        while (pop()) {}
    }
public:
    void push(const T& data) {
        node* item = new node(data);
        counted_node n = { 1, item };
        item->_next = _head.load();
        while (!_head.compare_exchange_weak(item->_next, n, 
            std::memory_order_release/*, std::memory_order_relaxed*/));
    }

    void increase_external_count(counted_node& old_head) {

        counted_node new_node;
        do {
            new_node = old_head;
            ++new_node.external_count;
        } while (!_head.compare_exchange_strong(old_head, new_node, 
                                                std::memory_order_acquire, 
                                                std::memory_order_relaxed));
        old_head.external_count= new_node.external_count;
    }

    std::shared_ptr<T> pop() {

        counted_node old_head = _head.load(std::memory_order_relaxed);

        while (true) {
            increase_external_count(old_head);
            node* ptr = old_head.ptr;
            if (!ptr)
                return std::shared_ptr<T>();

            if (_head.compare_exchange_strong(old_head, ptr->_next, 
                                              std::memory_order_relaxed)) {

                std::shared_ptr<T> res;
                res.swap(ptr->_data);

                int increase_count = old_head.external_count - 2;
                if (ptr->_internal_count.fetch_add(increase_count, 
                                                   std::memory_order_release) == -increase_count) {
                    delete ptr;
                }
                return res;
            }
            else if (ptr->_internal_count.fetch_sub(1, std::memory_order_relaxed) == 1) {
                auto count = ptr->_internal_count.load(std::memory_order_acquire);
                delete ptr;
            }
        }

        return std::shared_ptr<T>();
    }
};

template < class T >
int stack<T>::node::_allocated_num = 0;
template < class T >
int stack<T>::node::_deallocated_num = 0;
}// lock_free
}// ts
