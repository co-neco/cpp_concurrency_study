
#include <iostream>
#include <queue>
#include <mutex>
#include <memory>

namespace ts { namespace fine_tuned {

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
}// ts