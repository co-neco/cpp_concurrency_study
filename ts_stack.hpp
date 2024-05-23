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
    struct node {
        std::shared_ptr<T> _data;
        node* _next;
        node (const T& data): 
            _data(std::make_shared<T>(data)), _next(nullptr) { }
    };
    std::atomic<node*> _head, _to_be_deleted;
    std::atomic<unsigned int> _threads_in_pop;
public:
    stack(): _head(nullptr), _to_be_deleted(nullptr), _threads_in_pop(0) { }
    // Make sure no thread now is accessing current stack instance
    ~stack() { 
        auto cur = _head.load();
        while (cur) {
            auto next = cur->_next;
            delete cur;
            cur = next;
        }
    }

    void push(const T& data) {
        node* item = new node(data);
        item->_next = _head.load();
        while (!_head.compare_exchange_weak(item->_next, item));
    }

private:
    void chain_pending_nodes(node* first, node* last) {
        last->_next = _to_be_deleted;
        while (!_to_be_deleted.compare_exchange_weak(last->_next, first));
    }

    void chain_pending_nodes(node* nodes) {

        node* last = nodes;
        while (node* next = last->_next) {
            last = next;
        }
        chain_pending_nodes(nodes, last);
    }

    void chain_pending_node(node* n) {
        chain_pending_nodes(n, n);
    }

    void delete_nodes(node* nodes) {

        node* cur = nodes;
        while (cur) {
            auto next = cur->_next;
            delete cur;
            cur = next;
        }
    }

    void try_reclaim(std::atomic<node*> old_head) {

        if (_threads_in_pop == 1) {
            node* to_be_deleted = _to_be_deleted.exchange(nullptr);
            if (!--_threads_in_pop) {
                delete_nodes(to_be_deleted);
            }
            else {
                chain_pending_nodes(to_be_deleted);
            }
            delete old_head;
        }
        else {
            chain_pending_node(old_head);
            --_threads_in_pop;
        }
    }

public:
    std::shared_ptr<T> pop() {

        ++_threads_in_pop;
        node* old_head = _head.load();
        while(old_head && 
            !_head.compare_exchange_weak(old_head, old_head->_next));

        std::shared_ptr<T> res;
        if (old_head)
            res.swap(old_head->_data);

        try_reclaim(old_head);
        return res;
    }

    bool empty() {
        return !_head.load();
    }
};
}
}
