#pragma once
#include "bcos-framework/storage2/Storage.h"
#include "bcos-framework/transaction-executor/StateKey.h"
#include <bcos-task/Trait.h>
#include <roaring/roaring.hh>
#include <type_traits>

namespace bcos::transaction_scheduler
{

template <class Storage, class KeyType>
class ReadWriteSetStorage
{
private:
    Storage& m_storage;
    roaring::Roaring m_readSet;
    roaring::Roaring m_writeSet;

    void putSet(bool write, auto const& key) { putSet(write, std::hash<KeyType>{}(key)); }
    void putSet(bool write, size_t hash)
    {
        if (write)
        {
            m_writeSet.add(hash);
        }
        else
        {
            m_readSet.add(hash);
        }
    }

public:
    friend auto tag_invoke(storage2::tag_t<storage2::readSome> /*unused*/,
        ReadWriteSetStorage& storage, RANGES::input_range auto&& keys)
        -> task::Task<task::AwaitableReturnType<
            std::invoke_result_t<storage2::ReadSome, Storage&, decltype(keys)>>>
    {
        for (auto&& key : keys)
        {
            storage.putSet(false, std::forward<decltype(key)>(key));
        }
        co_return co_await storage2::readSome(
            storage.m_storage, std::forward<decltype(keys)>(keys));
    }

    friend auto tag_invoke(storage2::tag_t<storage2::readSome> /*unused*/,
        ReadWriteSetStorage& storage, RANGES::input_range auto&& keys,
        const storage2::READ_FRONT_TYPE& /*unused*/)
        -> task::Task<task::AwaitableReturnType<
            std::invoke_result_t<storage2::ReadSome, Storage&, decltype(keys)>>>
    {
        co_return co_await storage2::readSome(
            storage.m_storage, std::forward<decltype(keys)>(keys));
    }

    friend auto tag_invoke(
        storage2::tag_t<storage2::readOne> /*unused*/, ReadWriteSetStorage& storage, auto&& key)
        -> task::Task<task::AwaitableReturnType<
            std::invoke_result_t<storage2::ReadOne, Storage&, decltype(key)>>>
    {
        storage.putSet(false, key);
        co_return co_await storage2::readOne(storage.m_storage, std::forward<decltype(key)>(key));
    }

    friend auto tag_invoke(storage2::tag_t<storage2::readOne> /*unused*/,
        ReadWriteSetStorage& storage, auto&& key, storage2::READ_FRONT_TYPE /*unused*/)
        -> task::Task<task::AwaitableReturnType<
            std::invoke_result_t<storage2::ReadOne, Storage&, decltype(key)>>>
    {
        co_return co_await storage2::readOne(storage.m_storage, std::forward<decltype(key)>(key));
    }

    friend auto tag_invoke(storage2::tag_t<storage2::writeSome> /*unused*/,
        ReadWriteSetStorage& storage, RANGES::input_range auto&& keys,
        RANGES::input_range auto&& values)
        -> task::Task<task::AwaitableReturnType<
            std::invoke_result_t<storage2::WriteSome, Storage&, decltype(keys), decltype(values)>>>
    {
        for (auto&& key : keys)
        {
            storage.putSet(true, std::forward<decltype(key)>(key));
        }
        co_return co_await storage2::writeSome(
            storage.m_storage, keys, std::forward<decltype(values)>(values));
    }

    friend auto tag_invoke(storage2::tag_t<storage2::removeSome> /*unused*/,
        ReadWriteSetStorage& storage, RANGES::input_range auto const& keys)
        -> task::Task<task::AwaitableReturnType<
            std::invoke_result_t<storage2::RemoveSome, Storage&, decltype(keys)>>>
    {
        for (auto&& key : keys)
        {
            storage.putSet(true, std::forward<decltype(key)>(key));
        }
        co_return co_await storage2::removeSome(storage.m_storage, keys);
    }

    using Key = KeyType;
    using Value = typename task::AwaitableReturnType<decltype(storage2::readOne(
        m_storage, std::declval<KeyType>()))>::value_type;

    ReadWriteSetStorage(Storage& storage) : m_storage(storage) {}

    const auto& readSet() const& { return m_readSet; }
    const auto& writeSet() const& { return m_writeSet; }

    void mergeWriteSet(auto& inputWriteSet) { m_writeSet |= inputWriteSet.writeSet(); }

    // RAW: read after write
    bool hasRAWIntersection(const auto& rhs) const { return m_writeSet.intersect(rhs.readSet()); }
};

}  // namespace bcos::transaction_scheduler