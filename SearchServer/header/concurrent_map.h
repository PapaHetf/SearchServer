#pragma once

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Bucket {
        std::map<Key, Value> map_ = {};
        std::mutex mutex_;
    };

    struct Access {
        Access(Value& value, std::mutex& m): ref_to_value(value), lock_(m) {};
        std::mutex& lock_;
        Value& ref_to_value;
        ~Access() { lock_.unlock(); };
    };
    ConcurrentMap() = default;
    explicit ConcurrentMap(size_t bucket_count): buckets(bucket_count) {};

    Access operator[](const Key& key) {
        uint64_t key_64 = static_cast<uint64_t>(key);
        uint64_t part = key_64 % buckets.size();
        buckets[part].mutex_.lock();
        return { buckets[part].map_[key], buckets[part].mutex_};
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result_map;
        for (auto& value : buckets) {
            std::lock_guard guard(value.mutex_);
            result_map.insert(value.map_.begin(), value.map_.end());
        }
        return result_map;
    }

private:
    std::vector<Bucket> buckets;
};