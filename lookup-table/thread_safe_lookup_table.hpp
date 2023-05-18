#ifndef THREAD_SAFE_LOOKUP_TABLE_HPP
#define THREAD_SAFE_LOOKUP_TABLE_HPP

#include <cassert>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>
#include <list>
#include <algorithm>

template <typename Key, typename Value, typename Hash = std::hash<Key>, typename EqualTo = std::equal_to<Key>>
class ThreadSafeLookupTable
{
private:
    class Bucket
    {
    private:
        using bucket_value = std::pair<Key, Value>;
        using bucket_data = std::list<bucket_value>;
        using bucket_iterator = typename bucket_data::iterator;
        using bucket_const_iterator = typename bucket_data::const_iterator;

        bucket_data data_;
        mutable std::shared_mutex mutex_;

        bucket_iterator find_entry_for(const Key& key)
        {
            return std::find_if(data_.begin(), data_.end(), [&key](const auto& item) { return EqualTo{}(item.first, key); });
        }

        bucket_const_iterator find_entry_for(const Key& key) const
        {
            return std::find_if(data_.begin(), data_.end(), [&key](const auto& item) { return EqualTo{}(item.first, key); });
        }

    public:
        Value value_for(const Key& key, const Value& default_value) const
        {
            std::shared_lock lk{mutex_}; // CS for readers (many readers can go inside)

            bucket_const_iterator found_entry = find_entry_for(key);

            return (found_entry == data_.end()) ? default_value : found_entry->second;
        }

        void add_or_update_mapping(const Key& key, const Value& value)
        {
            std::unique_lock lk{mutex_}; // CS for writer (only one writer allowed)

            const bucket_iterator found_entry = find_entry_for(key);

            if (found_entry == data_.end())
                data_.push_back(bucket_value{key, value});
            else
                found_entry->second = value;
        }

        void remove_mapping(const Key& key)
        {
            std::unique_lock lk{mutex_}; // CS for writer (only one writer allowed)

            const bucket_iterator found_entry = find_entry_for(key);
            if (found_entry != data_.end())
                data_.erase(found_entry);
        }
    };

    std::vector<std::unique_ptr<Bucket>> buckets_;
    Hash hasher_;

    Bucket& get_bucket(const Key& key) const
    {
        auto const bucket_index = hasher_(key) % buckets_.size();

        return *buckets_[bucket_index];
    }

public:
    using key_type = Key;
    using value_type = Value;
    using hash_type = Hash;
    using equal_to_type = EqualTo;

    ThreadSafeLookupTable(unsigned int bucket_count = 19, const Hash& hasher = Hash{})
        : buckets_(bucket_count), hasher_{hasher}
    {
        for (unsigned int i = 0; i < bucket_count; ++i)
        {
            buckets_[i] = std::make_unique<Bucket>();
        }
    }

    ThreadSafeLookupTable(const ThreadSafeLookupTable&) = delete;
    ThreadSafeLookupTable& operator=(const ThreadSafeLookupTable&) = delete;

    value_type value_for(const key_type& key, const value_type& default_value = value_type()) const
    {
        return get_bucket(key).value_for(key, default_value);
    }

    void add_or_update_mapping(const key_type& key, const value_type& value)
    {
        get_bucket(key).add_or_update_mapping(key, value);
    }

    void remove_mapping(const key_type& key)
    {
        get_bucket(key).remove_mapping(key);
    }
};

#endif // THREAD_SAFE_LOOKUP_TABLE_HPP
