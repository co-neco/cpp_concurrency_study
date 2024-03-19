
#include <iostream>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <map>

#include "ts_list.hpp"

namespace ts { 
namespace tuned {

template < class Key, class Value, class Hash = std::hash<Key>>
class ts_map {
private:
    class bucket {
        typedef std::pair<Key, Value> bucket_value;
        ts::list<bucket_value> _list;
        typedef typename std::list<bucket_value>::const_iterator const_bucket_iterator;
        typedef typename std::list<bucket_value>::iterator bucket_iterator;
        mutable std::shared_mutex _m;
        friend class ts_map;

    public:
        bucket(): _list() {}
        bucket(const bucket& other) {
            this->_list = other._list;
        }
        bucket& operator=(const bucket& other) {
            this->_list = other._list;
            return *this;
        }
        bucket(bucket&& other) {
            this->_list = std::move(other._list);
        }
        bucket& operator=(bucket&& other) {
            this->_list = std::move(other._list);
            return *this;
        }

        const_bucket_iterator find_cons_iterator(const Key& key) const {
            return std::find_if(_list.cbegin(), _list.cend(), [&](const bucket_value& item) {
                return item.first == key;
                });
        }

        bucket_iterator find_iterator(const Key& key) {
            return std::find_if(_list.begin(), _list.end(), [&](const bucket_value& item) {
                return item.first == key;
                });
        }

        bool find(const Key& key) const {
            std::shared_lock l(_m);
            auto it = find_cons_iterator(key);
            return it != _list.cend();
        }

        Value get(const Key& key) const {
            std::shared_lock l(_m);
            auto it = find_cons_iterator(key);
            return it == _list.cend() ? Value() : it->second;
        }

        void insert(const Key& key, const Value& value) {
            std::unique_lock l(_m);
            auto it = find_iterator(key);
            if (it == _list.cend())
                _list.push_back(bucket_value(key, value));
            else
                it->second = value;
        }

        void erase(const Key& key) {
            std::unique_lock l(_m);
            auto it = find_cons_iterator(key);
            if (it != _list.cend())
                _list.erase(it);
        }
    };

private:
    static const int _default_bucket_size = 19;
    const int _bucket_size;
    const Hash _hash;
    std::vector<bucket> _buckets;

public:
    ts_map(
        int bucket_size = _default_bucket_size, 
        const Hash& hash = Hash())
        : _bucket_size(bucket_size),
          _hash(hash),
          _buckets(_bucket_size) { }
    ts_map(const ts_map&) = delete;
    ts_map& operator=(const ts_map&) = delete;

    bucket& get_bucket(const Key& key) {
        return _buckets[_hash(key)%_bucket_size];
    }
    const bucket& get_cons_bucket(const Key& key) const {
        return _buckets[_hash(key)%_bucket_size];
    }

    bool find(const Key& key) const {
        return get_cons_bucket(key).find(key);
    }

    Value get(const Key& key) const {
        return get_cons_bucket(key).get(key);
    }

    void insert(const Key& key, const Value& value) {
        get_bucket(key).insert(key, value);
    }

    void erase(const Key& key) {
        get_bucket(key).erase(key);
    }

    std::vector<std::unique_lock<std::shared_mutex>> lock_all_buckets() const {

        std::vector<std::unique_lock<std::shared_mutex>> lock_vector;
        for (auto it = _buckets.cbegin(); it != _buckets.cend(); ++it) {
            lock_vector.push_back(std::unique_lock(it->_m));
        }

        return lock_vector;
    }

    int size() const {
        int size = 0;
        auto lock_vector = lock_all_buckets();
        for (auto it = _buckets.cbegin(); it != _buckets.cend(); ++it)
            size += it->_list.size();
        return size;
    }

    bool empty() const {
        return size() == 0;
    }
    
    void clear() {
        auto lock_vector = lock_all_buckets();
        for (auto it = _buckets.begin(); it != _buckets.end(); ++it)
            it->_list.clear();
    }

    std::map<Key, Value> get_map() {
        std::map<Key, Value> map;
        auto lock_vector = lock_all_buckets();
        for (auto it = _buckets.cbegin(); it != _buckets.cend(); ++it) {
            auto& list = it->_list;
            for (auto& ele : list)
                map.insert(ele);
        }
        return map;
    }
};
} // namespace tuned
} // namespace ts