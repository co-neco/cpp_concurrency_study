#include <iostream>
#include <queue>
#include <mutex>
#include <memory>

template < class T >
class ts_queue {
public:
    inline ts_queue() {}
    inline ts_queue(const ts_queue& other) {
        std::lock_guard<std::mutex> m(other._m);
        this->_data = other._data;
    }
    ts_queue& operator=(const ts_queue&) = delete;

    inline void push(const T& item) {
        {
            std::lock_guard<std::mutex> m(this->_m);
            this->_data.push(item);
        }
        _data_con.notify_one();
    }

    inline bool pop(T& item) {
        std::lock_guard<std::mutex> m(this->_m);
        if (_data.empty()) return false;
        item = std::move(_data.front());
        _data.pop();
        return true;
    }

    inline std::shared_ptr<T> pop() {
        std::lock_guard<std::mutex> m(this->_m);
        if (_data.empty()) return false;
        auto item = std::make_shared<T>(std::move(_data.front()));
        _data.pop();
        return item;
    }

    inline bool wait_and_pop(T& item) {
        std::unique_lock<std::mutex> m(this->_m);
        this->_data_con.wait(m, [this] { return !this->_data.empty();});
        item = std::move(_data.front());
        _data.pop();
        return true;
    }

    inline std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> m(this->_m);
        this->_data_con.wait(m, [this] { return !this->_data.empty();});
        auto item = std::make_shared<T>(std::move(_data.front()));
        _data.pop();
        return item;
    }

    inline bool empty() const {
        std::lock_guard<std::mutex> m(this->_m);
        return _data.empty();
    }

    inline int size() const {
        std::lock_guard<std::mutex> m(this->_m);
        return _data.size();
    }

    inline void clear() {
        std::lock_guard<std::mutex> m(this->_m);
        _data.swap(std::queue<std::string>());
    }

private:
    mutable std::mutex _m;
    std::queue<T> _data;
    std::condition_variable _data_con;
};