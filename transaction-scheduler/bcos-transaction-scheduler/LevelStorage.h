#pragma once
#include <bcos-framework/transaction-executor/TransactionExecutor.h>
#include <boost/throw_exception.hpp>
#include <range/v3/range/access.hpp>
#include <type_traits>
#include <variant>

namespace bcos::transaction_scheduler
{
struct MutableStorageExists : public bcos::Error
{
};

template <class Storage, class Key>
struct StorageReadIteratorTrait
{
    using type = typename task::AwaitableReturnType<decltype(std::declval<Storage>->read(
        std::declval<Key>()))>;
};
template <class Storage, class Key>
using StorageReadIteratorType = typename StorageReadIteratorTrait<Storage, Key>::type;

template <transaction_executor::StateStorage MutableStorage,
    transaction_executor::StateStorage BackendStorage, class CacheStorage = void>
requires(std::is_void_v<CacheStorage> ||
         transaction_executor::StateStorage<CacheStorage>) class LevelStorage
{
private:
    constexpr static bool withCacheStorage = !(std::is_void_v<CacheStorage>);

    std::shared_ptr<MutableStorage> m_mutableStorage;                // Mutable storage
    std::list<std::shared_ptr<MutableStorage>> m_immutableStorages;  // Read only storages
    [[no_unique_address]] std::conditional_t<withCacheStorage, CacheStorage&, std::monostate>
        m_cacheStorage;                // Cache
                                       // storage
    BackendStorage& m_backendStorage;  // Backend storage
    std::mutex m_storageMutex;

    template <transaction_executor::StateStorage ReadableStorage>
    auto readFromStorage(ReadableStorage& storage, RANGES::input_range auto const& keys)
        -> task::Task<task::AwaitableReturnType<decltype(m_mutableStorage.read(keys))>>
    {
        auto it = storage.read(keys);
        co_return readFromStorage();
    }

public:
    template <RANGES::input_range KeyRange, transaction_executor::StateStorage... StorageType>
    class ReadIterator
    {
    public:
        friend class LevelStorage;
        using Key = const transaction_executor::StateKey&;
        using Value = const transaction_executor::StateValue&;

        ReadIterator(KeyRange const& keyRange, LevelStorage& storage)
          : m_keyRange(keyRange), m_storage(storage), m_keyRangeIt(RANGES::begin(m_keyRange))
        {}

        task::AwaitableValue<bool> next() &
        {
            if (!m_started)
            {
                m_started = true;
                return m_keyRangeIt != RANGES::end(m_keyRange);
            }
            return ++m_keyRangeIt != RANGES::end(m_keyRange);
        }
        task::Task<bool> hasValue() const
        {
            if (m_innerIt.index() == 0)
            {
                query(*m_keyRangeIt);
            }
            co_return std::visit(
                [](auto&& iterator) -> task::Task<bool> {
                    using IteratorType = std::remove_cvref_t<decltype(iterator)>;
                    if constexpr (!std::is_same_v<std::monostate, decltype(iterator)>)
                    {
                        co_return iterator.hasValue();
                    }
                },
                m_innerIt);
        }
        task::Task<Key> key() const {}
        task::Task<Value> value() const {}

        void release() {}

    private:
        // Query from top to buttom
        task::Task<void> query(Key key)
        {
            if (m_storage.m_mutableStorage &&
                co_await queryAndSetIt(m_storage.m_mutableStorage, key))
            {
                co_return;
            }

            for (auto& storage : m_storage.m_immutableStorages)
            {
                if (co_await queryAndSetIt(storage, key))
                {
                    co_return;
                }
            }

            if constexpr (withCacheStorage)
            {
                if (co_await queryAndSetIt(m_storage.m_cacheStorage, key))
                {
                    co_return;
                }
            }

            if (co_await queryAndSetIt(m_storage.m_backendStorage, key))
            {
                co_return;
            }
        }

        task::Task<bool> queryAndSetIt(auto& storage, auto const& key)
        {
            auto it = co_await storage->read(storage2::single(key));
            co_await it.next();
            if (co_await it.hasValue())
            {
                m_innerIt.template emplace<decltype(it)>(std::move(it));
                co_return true;
            }
            co_return false;
        }

        KeyRange const& m_keyRange;
        LevelStorage& m_storage;
        RANGES::iterator_t<KeyRange> m_keyRangeIt;
        std::variant<std::monostate,
            StorageReadIteratorType<StorageType, transaction_executor::StateKey>...>
            m_innerIt;
        bool m_started = false;
    };
    LevelStorage(BackendStorage& backendStorage) : m_backendStorage(backendStorage) {}

    auto read(RANGES::input_range auto&& keys) -> task::AwaitableValue<ReadIterator<decltype(keys)>>
    requires std::is_lvalue_reference_v<decltype(keys)>
    {
        co_return ReadIterator<decltype(keys), BackendStorage, MutableStorage>(keys, *this);
    }

    LevelStorage fork() const
    {
        std::unique_lock lock(m_storageMutex);
        LevelStorage levelStorage(m_backendStorage);
        levelStorage.m_immutableStorages = m_immutableStorages;

        return levelStorage;
    }

    template <class... Args>
    void newMutable(Args... args)
    {
        std::unique_lock lock(m_storageMutex);
        if (m_mutableStorage)
        {
            BOOST_THROW_EXCEPTION(MutableStorageExists{});
        }

        m_mutableStorage = std::make_shared<MutableStorage>(args...);
    }

    void dropMutable() { m_mutableStorage.reset(); }

    void pushMutableToImmutableFront()
    {
        std::unique_lock lock(m_storageMutex);
        m_immutableStorages.push_front(m_mutableStorage);
        m_mutableStorage.reset();
    }

    void popImmutableFront()
    {
        std::unique_lock lock(m_storageMutex);
        m_immutableStorages.pop_front();
    }

    void mergeAndPopImmutableBack() {}
};
}  // namespace bcos::transaction_scheduler