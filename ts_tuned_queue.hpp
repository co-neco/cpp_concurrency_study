
#include <iostream>
#include <queue>
#include <mutex>
#include <memory>

namespace ts { 
namespace fine_tuned {

template < class T >
class queue {
private:
    struct node {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };

private:
    std::unique_ptr<node> _head;
    node* _tail;
    std::condition_variable _cond;
    std::mutex _hm; // head mutex
    std::mutex _tm; // tail mutex

public:
    queue(): _head(new node), _tail(_head.get()) { }
    queue(const queue&) = delete;
    queue& operator=(const queue&) = delete;

    void push(T data) {
        auto new_data = std::make_shared<T>(std::move(data));
        auto new_node = std::make_unique<node>();
        node* new_tail = new_node.get();

        {
            std::lock_guard<std::mutex> l(_tm);
            _tail->data = new_data;
            _tail->next = std::move(new_node);
            _tail = new_tail;
        }
        _cond.notify_one();
    }

    node* get_tail() {
        std::lock_guard<std::mutex> l(_tm);
        return _tail;
    }

    std::unique_lock<std::mutex> wait_item() {
        std::unique_lock<std::mutex> l(_hm);
        _cond.wait(l, [this] { return _head.get() != get_tail(); });
        return l;
    }

    std::unique_ptr<node> pop_head() {
        auto old_head = std::move(_head);
        _head = std::move(old_head->next);
        return old_head;
    }

    void wait_and_pop(T& data) {
        auto l = wait_item();
        data = std::move(*_head->data);
        pop_head();
    }

    std::shared_ptr<T> wait_and_pop() {
        auto l = wait_item();
        auto data = _head->data;
        pop_head();
        return data;
    }

    std::shared_ptr<T> try_pop() {
        std::lock_guard<std::mutex> l(_hm);
        if (_head.get() == get_tail())
            return nullptr;
        return pop_head()->data;
    }

    bool try_pop(T& data) {
        std::lock_guard<std::mutex> l(_hm);
        if (_head.get() == get_tail())
            return false;
        data = std::move(*_head->data);
        pop_head();
        return true;
    }

    bool empty() {
        std::lock_guard<std::mutex> l(_hm);
        return _head.get() == get_tail();
    }

    int size() {
        int size = 0;
        std::scoped_lock l(_hm, _tm);
        node* cur = _head.get();
        
        while (cur != _tail) {
            ++size;
            cur = cur->next.get();
        }

        return size;
    }

    void clear() {
        std::scoped_lock l(_hm, _tm);
        if (_head.get() == _tail)
            return;

        auto cur = std::move(_head);
        
        // Do not call next.reset() directly, which would result in stack overflow
        // if current queue had many nodes.
        do  {
            auto next = std::move(cur->next);
            cur.reset();
            cur = std::move(next);
        } while (cur.get() != _tail);

        _head = std::move(cur);
    }
};
}// fine_tuned

namespace lock_free {

template < class T >
class queue {
private:
    struct node;
    struct counted_node_ptr {
        int external_count;
        node* ptr;
    };
    struct node_counter {
        unsigned int internal_count : 30;
        unsigned int external_counters : 2;
    };
    struct node {
        std::atomic<T*> _data;
        counted_node_ptr _next;
        std::atomic<node_counter> _counter;
        node() {
            node_counter counter = { 0, 2 };
            _counter.exchange(counter);

            _next = { 0, nullptr };
        }
        void release_ref() {
            
            node_counter old_counter = _counter.load();
            node_counter new_counter;
            do {
                new_counter = old_counter;
                --new_counter.internal_count;
            } while (!_counter.compare_exchange_strong(old_counter, new_counter));

            node_counter counter = _counter.load();
            if (!counter.external_counters && !counter.internal_count)
                delete this;
        }
    };

    std::atomic<counted_node_ptr> _head, _tail;

    static void increase_external_count(std::atomic<counted_node_ptr>& node, counted_node_ptr& old_node) {

        counted_node_ptr new_node;
        do {
            new_node = old_node;
            ++new_node.external_count;
        } while (!node.compare_exchange_strong(old_node, new_node));
    }

    static void free_external_count(const counted_node_ptr& node) {

        int increase_num = node.external_count - 2;
        node_counter old_counter = node.ptr->_counter.load();
        node_counter new_counter;
        do {
            new_counter = old_counter;
            new_counter.internal_count += increase_num;
            --new_counter.external_counters;
        } while (node.ptr->_counter.compare_exchange_strong(old_counter, new_counter));

        node_counter counter = node.ptr->_counter.load();
        if (!counter.external_counters && !counter.internal_count)
            delete node.ptr;
    }

public:
    queue() {
        node* n = new node;
        counted_node_ptr counted_node = { 1, n };
        _head.store(counted_node);
        _tail.store(counted_node);
    }
    ~queue() {
        while (pop()) { }
        delete _head.load().ptr;
    }

    void push(T data) {

        std::unique_ptr<T> new_data = std::make_unique<T>(std::move(data));
        node* n = new node;
        counted_node_ptr new_node = { 1, n };


        while (true) {
            counted_node_ptr old_tail = _tail.load();
            increase_external_count(_tail, old_tail);
            T* old_data = nullptr;
            if (old_tail.ptr->_data.compare_exchange_strong(old_data, new_data.get())) {
                old_tail.ptr->_next = new_node;
                // Get the latest tail node, which contains current external count that may change.
                old_tail = _tail.exchange(new_node);
                free_external_count(old_tail);
                new_data.release();
                break;
            }
            old_tail.ptr->release_ref();
        }
    }

    std::unique_ptr<T> pop() {

        std::unique_ptr<T> res;

        while (true) {
            counted_node_ptr old_head = _head.load();
            increase_external_count(_head, old_head);
            if (old_head.ptr == _tail.load().ptr) {
                old_head.ptr->release_ref();
                return std::unique_ptr<T>();
            }

            if (_head.compare_exchange_strong(old_head, old_head.ptr->_next)) {
                res.reset(old_head.ptr->_data.load());
                free_external_count(old_head);
                return res;
            }
            old_head.ptr->release_ref();
        }
    }
};
}
}// ts