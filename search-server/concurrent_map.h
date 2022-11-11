#pragma once
#include <map>
#include <vector>
#include <mutex>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Data {
        std::mutex mut;
        std::map<Key, Value> data;
    };

    struct Access {
        std::lock_guard<std::mutex> lock_;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) : size_(bucket_count), data_(size_) {}

    Access operator[](const Key& key);

    std::map<Key, Value> BuildOrdinaryMap();

private:
    size_t size_;
    std::vector<Data> data_;
};

template<typename Key, typename Value>
typename ConcurrentMap<Key, Value>::Access
ConcurrentMap<Key, Value>::operator[](const Key& key) {
    uint16_t i = key % size_;
    return Access{ std::lock_guard(data_[i].mut), data_[i].data[key] };
}

template<typename Key, typename Value>
std::map<Key, Value> ConcurrentMap<Key,Value>::BuildOrdinaryMap() {
    std::map<Key, Value> result;
    for (size_t i = 0; i < size_; ++i) {
        std::lock_guard guard(data_[i].mut);
        result.insert(data_.at(i).data.begin(), data_.at(i).data.end());
    }
    return result;
}