#pragma once

#include <iostream>
#include <stack>
#include <mutex>
#include <memory>

template < class T >
class ts_stack {

public:
    inline ts_stack() { }
    inline ts_stack(const ts_stack& other) {
        std::lock_guard<std::mutex> m(other._m);
        this->_data = other._data;
    }
    ts_stack& operator=(const ts_stack&) = delete;

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