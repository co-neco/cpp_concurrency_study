#include <iostream>
#include <memory>
#include <mutex>
#include <list>

namespace ts {
template < class T >
class list {
private:
    struct node {
        mutable std::mutex _m;
        std::shared_ptr<T> _data;
        std::unique_ptr<node> _next;
        node (): _data(), _next() { }
        node (const T& data): _data(std::make_shared<T>(data)), _next() { }
    };

public:
    list(): _head() { }
    ~list() { remove_if([](const T&) { return true; }); }
    list(const list&) = delete;
    list& operator=(const list&) = delete;
    // Move when some node is locked ???
    list(list&& other) { _head = std::move(other._head);}
    list& operator=(list&& other) { 
        _head = std::move(other._head); 
        return *this;
    }

    void push_back(const T& data) {
        auto item = std::make_unique<node>(data);
        std::unique_lock l(_head._m);
        item->_next = std::move(_head._next);
        _head._next = std::move(item);
    }

    template < typename Predicate >
    std::shared_ptr<T> find_first_if(Predicate p) const {
        const node* cur = &_head;
        std::unique_lock l(_head._m);
        while (auto next = cur->_next.get()) {
            std::unique_lock nl(next->_m);
            l.unlock();

            if (p(*next->_data))
                return next->_data;

            cur = next;
            l = std::move(nl);
        }
        return std::shared_ptr<T>();
    }

    template < typename Predicate >
    void insert(Predicate p, const T& data) {
        node* cur = &_head;
        std::unique_lock l(_head._m);
        while (auto next = cur->_next.get()) {
            std::unique_lock nl(next->_m);
            l.unlock();

            if (p(*next->_data)) {
                *next->_data = data;
                return;
            }
            else {
                cur = next;
                l = std::move(nl);
            }
        }

        l.unlock();
        push_back(data);
    }

    template < typename Predicate >
    void remove_if(Predicate p) {
        node* cur = &_head;
        std::unique_lock l(_head._m);
        while (auto next = cur->_next.get()) {
            std::unique_lock nl(next->_m);
            if (p(*next->_data)) {
                auto item = std::move(cur->_next);
                cur->_next = std::move(next->_next);
                nl.unlock();
                item.reset();
            }
            else {
                l.unlock();
                cur = next;
                l = std::move(nl);
            }
        }
    }

    int size() const {
        int size = 0;
        const node* cur = &_head;
        std::unique_lock l(_head._m);
        while (auto next = cur->_next.get()) {
            std::unique_lock nl(next->_m);
            l.unlock();
            ++size;
            cur = next;
            l = std::move(nl);
        }
        return size;
    }

    void clear() {
        remove_if([](const T&) { return true; });
    }

    std::list<T> get_list() const {
        std::list<T> list;
        const node* cur = &_head;
        std::unique_lock l(_head._m);
        while (auto next = cur->_next.get()) {
            std::unique_lock nl(next->_m);
            l.unlock();
            list.push_back(*next->_data);
            cur = next;
            l = std::move(nl);
        }
        return list;
    }

private:
    node _head;
};
}