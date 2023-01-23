#pragma once

#include "Storage.h"
#include "bcos-task/Task.h"
#include <bcos-utilities/NullLock.h>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/throw_exception.hpp>
#include <forward_list>
#include <functional>
#include <mutex>
#include <set>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>

namespace bcos::storage2::memory_storage
{

template <class Object>
concept HasMemberSize = requires(Object object)
{
    // clang-format off
    { object.size() } -> std::integral;
    // clang-format on
};

using Empty = std::monostate;

enum Attribute : int
{
    NONE = 0x0,
    ORDERED = 0x1,
    CONCURRENT = 0x2,
    MRU = 0x4
};

template <class KeyType, class ValueType = Empty, Attribute attribute = Attribute::NONE,
    class BucketHasher = void>
class MemoryStorage
{
private:
    constexpr static bool withOrdered = (attribute & Attribute::ORDERED) != 0;
    constexpr static bool withConcurrent = (attribute & Attribute::CONCURRENT) != 0;
    constexpr static bool withMRU = (attribute & Attribute::MRU) != 0;
    constexpr static unsigned MAX_BUCKETS = 64;  // Support up to 64 buckets for concurrent, enough?
    constexpr unsigned getBucketSize() { return withConcurrent ? MAX_BUCKETS : 1; }

    static_assert(!withConcurrent || !std::is_void_v<BucketHasher>);

    constexpr static unsigned DEFAULT_CAPACITY = 32L * 1024 * 1024;  // For mru
    using Mutex = std::conditional_t<withConcurrent, std::mutex, Empty>;
    using Lock =
        std::conditional_t<withConcurrent, std::unique_lock<std::mutex>, utilities::NullLock>;

    struct Data
    {
        KeyType key;
        [[no_unique_address]] ValueType value;
    };

    using IndexType = std::conditional_t<withOrdered,
        boost::multi_index::ordered_unique<boost::multi_index::member<Data, KeyType, &Data::key>>,
        boost::multi_index::hashed_unique<boost::multi_index::member<Data, KeyType, &Data::key>>>;
    using Container = std::conditional_t<withMRU,
        boost::multi_index_container<Data,
            boost::multi_index::indexed_by<IndexType, boost::multi_index::sequenced<>>>,
        boost::multi_index_container<Data, boost::multi_index::indexed_by<IndexType>>>;

    struct Bucket
    {
        Container container;
        [[no_unique_address]] Mutex mutex;  // For concurrent
        [[no_unique_address]] std::conditional_t<withMRU, int64_t, Empty> capacity = {};  // For mru
    };
    using Buckets = std::conditional_t<withConcurrent, std::vector<Bucket>, std::array<Bucket, 1>>;

    Buckets m_buckets;
    [[no_unique_address]] std::conditional_t<withMRU, int64_t, Empty> m_maxCapacity = {};

    std::tuple<std::reference_wrapper<Bucket>, Lock> getBucket(auto const& key)
    {
        if constexpr (!withConcurrent)
        {
            return std::make_tuple(std::ref(m_buckets[0]), Lock(Empty{}));
        }

        auto index = getBucketIndex(key);

        auto& bucket = m_buckets[index];
        return std::make_tuple(std::ref(bucket), Lock(bucket.mutex));
    }

    Bucket& getBucketByIndex(size_t index) { return m_buckets[index]; }

    size_t getBucketIndex(auto const& key) const
    {
        if constexpr (!withConcurrent)
        {
            return 0;
        }
        else
        {
            auto hash = BucketHasher{}(key);
            return hash % m_buckets.size();
        }
    }

    void updateMRUAndCheck(Bucket& bucket,
        typename Container::template nth_index<0>::type::iterator entryIt) requires withMRU
    {
        auto& index = bucket.container.template get<1>();
        auto seqIt = index.iterator_to(*entryIt);
        index.relocate(index.end(), seqIt);

        size_t clearCount = 0;
        while (bucket.capacity > m_maxCapacity && !bucket.container.empty())
        {
            auto const& item = index.front();
            bucket.capacity -= getSize(item.value);

            index.pop_front();
            ++clearCount;
        }
    }

    static int64_t getSize(auto const& object)
    {
        using ObjectType = std::remove_cvref_t<decltype(object)>;
        if constexpr (HasMemberSize<ObjectType>)
        {
            return object.size();
        }

        // Treat any no-size() object as trivial, TODO: fix it
        return sizeof(ObjectType);
    }

public:
    MemoryStorage(unsigned buckets = 0) requires(!withConcurrent) {}

    MemoryStorage(unsigned buckets = std::thread::hardware_concurrency()) requires(withConcurrent)
      : m_buckets(std::min(buckets, getBucketSize()))
    {}

    void setMaxCapacity(int64_t capacity) requires withMRU { m_maxCapacity = capacity; }

    class ReadIterator
    {
    public:
        friend class MemoryStorage;
        using Key = const KeyType&;
        using Value = const ValueType&;

        task::AwaitableValue<bool> hasValue() const { return {*m_it != nullptr}; }
        task::AwaitableValue<bool> next() &
        {
            if (!m_started)
            {
                m_started = true;
                return {m_it != m_iterators.end()};
            }
            return {(++m_it) != m_iterators.end()};
        }
        task::AwaitableValue<Key> key() const { return {(*m_it)->key}; }
        task::AwaitableValue<Value> value() const { return {(*m_it)->value}; }

        void release()
        {
            if constexpr (withConcurrent)
            {
                m_bucketLocks.clear();
            }
        }

    private:
        typename std::vector<const Data*>::iterator m_it;
        std::vector<const Data*> m_iterators;
        [[no_unique_address]] std::conditional_t<withConcurrent, std::forward_list<Lock>, Empty>
            m_bucketLocks;
        bool m_started = false;
    };

    class SeekIterator
    {
    public:
        friend class MemoryStorage;
        using Key = const KeyType&;
        using Value = const ValueType&;

        task::AwaitableValue<bool> next() &
        {
            if (!m_started)
            {
                m_started = true;
                return {m_it != m_end};
            }
            return {(++m_it) != m_end};
        }
        task::AwaitableValue<Key> key() const { return {m_it->key}; }
        task::AwaitableValue<Value> value() const { return {m_it->value}; }

        void release() { m_bucketLock.unlock(); }

    private:
        typename Container::iterator m_it;
        typename Container::iterator m_end;
        [[no_unique_address]] Lock m_bucketLock;
        bool m_started = false;
    };

    task::AwaitableValue<ReadIterator> read(RANGES::input_range auto const& keys) &
    {
        task::AwaitableValue<ReadIterator> outputAwaitable(ReadIterator{});
        ReadIterator& output = outputAwaitable.value();
        if constexpr (RANGES::sized_range<std::remove_cvref_t<decltype(keys)>>)
        {
            output.m_iterators.reserve(RANGES::size(keys));
        }

        std::conditional_t<withConcurrent, std::bitset<MAX_BUCKETS>, Empty> locks;
        for (auto&& key : keys)
        {
            auto bucketIndex = getBucketIndex(key);
            auto& bucket = getBucketByIndex(bucketIndex);

            if constexpr (withConcurrent)
            {
                if (!locks[bucketIndex])
                {
                    locks[bucketIndex] = true;
                    output.m_bucketLocks.emplace_front(std::unique_lock(bucket.mutex));
                }
            }

            auto const& index = bucket.container.template get<0>();

            auto it = index.find(key);
            if (it != index.end())
            {
                if constexpr (withMRU)
                {
                    updateMRUAndCheck(bucket, it);
                }
                output.m_iterators.emplace_back(std::addressof(*it));
            }
            else
            {
                output.m_iterators.emplace_back(nullptr);
            }
        }
        output.m_it = output.m_iterators.begin();

        return outputAwaitable;
    }

    task::AwaitableValue<SeekIterator> seek(auto const& key) & requires(withOrdered)
    {
        auto [bucket, lock] = getBucket(key);
        auto const& index = bucket.get().container.template get<0>();

        auto it = index.lower_bound(key);

        task::AwaitableValue<SeekIterator> output({});
        auto& seekIt = output.value();
        seekIt.m_it = it;
        seekIt.m_end = index.end();
        seekIt.m_bucketLock = std::move(lock);

        return output;
    }

    task::AwaitableValue<void> write(
        RANGES::input_range auto&& keys, RANGES::input_range auto&& values)
    {
        for (auto&& [key, value] : RANGES::zip_view(
                 std::forward<decltype(keys)>(keys), std::forward<decltype(values)>(values)))
        {
            auto [bucket, lock] = getBucket(key);
            auto const& index = bucket.get().container.template get<0>();

            std::conditional_t<withMRU, int64_t, Empty> updatedCapacity;
            if constexpr (withMRU)
            {
                updatedCapacity = getSize(value);
            }

            typename Container::iterator it;
            if constexpr (withOrdered)
            {
                it = index.lower_bound(key);
            }
            else
            {
                it = index.find(key);
            }
            if (it != index.end() && it->key == key)
            {
                auto& existsValue = it->value;
                if constexpr (withMRU)
                {
                    updatedCapacity -= getSize(existsValue);
                }

                bucket.get().container.modify(
                    it, [newValue = std::forward<decltype(value)>(value)](Data& data) {
                        data.value = std::forward<decltype(newValue)>(newValue);
                    });
            }
            else
            {
                it = bucket.get().container.emplace_hint(it, Data{.key = key, .value = value});
            }

            if constexpr (withMRU)
            {
                bucket.get().capacity += updatedCapacity;
                updateMRUAndCheck(bucket.get(), it);
            }
        }

        return {};
    }

    task::AwaitableValue<void> remove(RANGES::input_range auto const& keys)
    {
        for (auto&& key : keys)
        {
            auto [bucket, lock] = getBucket(key);
            auto const& index = bucket.get().container.template get<0>();

            auto it = index.find(key);
            if (it != index.end())
            {
                auto& existsValue = it->value;
                if constexpr (withMRU)
                {
                    bucket.get().capacity -= getSize(existsValue);
                }
                bucket.get().container.erase(it);
            }
        }

        return {};
    }
};

}  // namespace bcos::storage2::memory_storage