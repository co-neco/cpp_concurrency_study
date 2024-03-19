#include <iostream>
#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include <map>

#include "ts_list.hpp"

namespace ts { namespace fine_tuned {

template < class Key, class Value, class Hash = std::hash<Key>>
class map {
private:
    class bucket {
    private:
        typedef std::pair<Key, Value> bucket_value;
        list<bucket_value> _list;

    public:
        bucket(): _list() {}
        bucket(const bucket& other) = delete;
        bucket& operator=(const bucket& other) = delete;
        bucket(bucket&& other) {
            this->_list = std::move(other._list);
        }
        bucket& operator=(bucket&& other) {
            this->_list = std::move(other._list);
            return *this;
        }

        bool find(const Key& key) const {
            auto item = _list.find_first_if([&](const bucket_value& data) {
                return data.first == key;
            });
            return (bool)item;
        }

        Value get(const Key& key) const {
            auto item = _list.find_first_if([&](const bucket_value& data) {
                return data.first == key;
            });
            return (bool)item ? item->second : Value();
        }

        void insert(const Key& key, const Value& value) {
            _list.insert(
                [&](const bucket_value& data) {
                    return data.first == key;
                }, bucket_value(key, value));
        }

        void erase(const Key& key) {
            _list.remove_if([&](const bucket_value& data) {
                return data.first == key;
            });
        }

        int size() const { return _list.size(); }
        void clear() { _list.clear(); }
        std::list<bucket_value> get_list() const { return _list.get_list(); }
    };

private:
    static const int _default_bucket_size = 19;
    const int _bucket_size;
    const Hash _hash;
    std::vector<bucket> _buckets;

public:
    map(
        int bucket_size = _default_bucket_size, 
        const Hash& hash = Hash())
        : _bucket_size(bucket_size),
          _hash(hash),
          _buckets(_bucket_size) { }
    map(const map&) = delete;
    map& operator=(const map&) = delete;

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

    int size() const {
        int size = 0;
        for (auto it = _buckets.cbegin(); it != _buckets.cend(); ++it)
            size += it->size();
        return size;
    }

    bool empty() const {
        return size() == 0;
    }
    
    void clear() {
        for (auto it = _buckets.begin(); it != _buckets.end(); ++it)
            it->clear();
    }

    std::map<Key, Value> get_map() const {
        std::map<Key, Value> map;
        for (auto it = _buckets.cbegin(); it != _buckets.cend(); ++it) {
            auto list = it->get_list();
            for (auto& ele : list)
                map.insert(ele);
        }
        return map;
    }
};
}// fine_tuned
}// ts