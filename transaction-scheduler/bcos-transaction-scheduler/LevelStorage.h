#pragma once
#include <bcos-framework/transaction-executor/TransactionExecutor.h>
#include <boost/throw_exception.hpp>
#include <range/v3/range/access.hpp>
#include <range/v3/range_fwd.hpp>
#include <type_traits>
#include <variant>

namespace bcos::transaction_scheduler
{
struct DuplicateMutableStorage : public bcos::Error
{
};
struct NotExistsMutableStorage : public bcos::Error
{
};
struct NotExistsImmutableStorage : public bcos::Error
{
};

template <class Storage, class Key>
struct StorageReadIteratorTrait
{
    using type = typename task::AwaitableReturnType<decltype(std::declval<Storage>().read(
        storage2::single<Key>(std::declval<Key>())))>;
};
template <class Storage, class Key>
using StorageReadIteratorType = typename StorageReadIteratorTrait<Storage, Key>::type;

template <transaction_executor::StateStorage MutableStorage,
    transaction_executor::StateStorage BackendStorage, class CachedStorage = void>
requires(std::is_void_v<CachedStorage> ||
         transaction_executor::StateStorage<CachedStorage>) class LevelStorage
{
private:
    constexpr static bool withCacheStorage = !std::is_void_v<CachedStorage>;

    std::shared_ptr<MutableStorage> m_mutableStorage;                // Mutable storage
    std::list<std::shared_ptr<MutableStorage>> m_immutableStorages;  // Read only storages
    [[no_unique_address]] std::conditional_t<withCacheStorage,
        std::add_lvalue_reference_t<CachedStorage>,
        std::monostate>
        m_cacheStorage;                // Cache
                                       // storage
    BackendStorage& m_backendStorage;  // Backend storage

    std::mutex m_immutablesMutex;
    std::mutex m_mergeMutex;

public:
    template <RANGES::borrowed_range KeyRange, transaction_executor::StateStorage... StorageType>
    class ReadIterator
    {
    public:
        friend class LevelStorage;
        using Key = const transaction_executor::StateKey&;
        using Value = const transaction_executor::StateValue&;

        ReadIterator(KeyRange&& keyRange, LevelStorage& storage)
          : m_keyRange(std::forward<decltype(keyRange)>(keyRange)),
            m_storage(storage),
            m_keyRangeIt(RANGES::begin(m_keyRange))
        {}

        task::AwaitableValue<bool> next() &
        {
            m_innerIt = std::monostate{};
            if (!m_started)
            {
                m_started = true;
                return m_keyRangeIt != RANGES::end(m_keyRange);
            }
            return ++m_keyRangeIt != RANGES::end(m_keyRange);
        }
        task::Task<bool> hasValue() const
        {
            co_await query(*m_keyRangeIt);
            co_return co_await std::visit(
                [](auto&& iterator) -> task::Task<bool> {
                    using IteratorType = std::remove_cvref_t<decltype(iterator)>;
                    if constexpr (!std::is_same_v<std::monostate, IteratorType>)
                    {
                        co_return co_await iterator.hasValue();
                    }
                    co_return false;
                },
                m_innerIt);
        }
        task::Task<Key> key() const
        {
            co_await query(*m_keyRangeIt);
            co_return co_await std::visit(
                [](auto&& iterator) -> task::Task<Key> {
                    using IteratorType = std::remove_cvref_t<decltype(iterator)>;
                    if constexpr (!std::is_same_v<std::monostate, IteratorType>)
                    {
                        co_return co_await iterator.key();
                    }
                },
                m_innerIt);
        }
        task::Task<Value> value() const
        {
            co_await query(*m_keyRangeIt);
            co_return co_await std::visit(
                [](auto&& iterator) -> task::Task<Value> {
                    using IteratorType = std::remove_cvref_t<decltype(iterator)>;
                    if constexpr (!std::is_same_v<std::monostate, IteratorType>)
                    {
                        co_return co_await iterator.value();
                    }
                },
                m_innerIt);
        }

        void release() {}

    private:
        // Query from top to buttom
        task::Task<void> query(Key key) const
        {
            if (m_innerIt.index() != 0)
            {
                co_return;
            }

            if (m_storage.m_mutableStorage &&
                co_await queryAndSetIt(*m_storage.m_mutableStorage, key))
            {
                co_return;
            }

            for (auto& storage : m_storage.m_immutableStorages)
            {
                if (co_await queryAndSetIt(*storage, key))
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

        task::Task<bool> queryAndSetIt(auto& storage, auto const& key) const
        {
            auto it = co_await storage.read(storage2::single(key));
            co_await it.next();
            if (co_await it.hasValue())
            {
                m_innerIt.template emplace<decltype(it)>(std::move(it));
                co_return true;
            }
            co_return false;
        }

        KeyRange m_keyRange;
        LevelStorage& m_storage;
        RANGES::iterator_t<KeyRange> m_keyRangeIt;
        mutable std::variant<std::monostate,
            StorageReadIteratorType<StorageType, transaction_executor::StateKey>...>
            m_innerIt;
        bool m_started = false;
    };
    LevelStorage(BackendStorage& backendStorage) requires(!withCacheStorage)
      : m_backendStorage(backendStorage)
    {}

    // template <typename std::enable_if_t<withCacheStorage>>
    // LevelStorage(
    //     BackendStorage& backendStorage, std::add_lvalue_reference_t<CachedStorage> cacheStorage)
    //   : m_backendStorage(backendStorage), m_cacheStorage(cacheStorage)
    // {}

    auto read(RANGES::borrowed_range auto&& keys)
        -> task::AwaitableValue<ReadIterator<decltype(keys), BackendStorage, MutableStorage>>
    {
        auto it = ReadIterator<decltype(keys), BackendStorage, MutableStorage>(
            std::forward<decltype(keys)>(keys), *this);
        return task::AwaitableValue<decltype(it)>(std::move(it));
    }

    task::Task<void> write(RANGES::input_range auto&& keys, RANGES::input_range auto&& values)
    {
        if (!m_mutableStorage)
        {
            BOOST_THROW_EXCEPTION(NotExistsMutableStorage{});
        }

        co_await m_mutableStorage->write(
            std::forward<decltype(keys)>(keys), std::forward<decltype(values)>(values));
        co_return;
    }

    task::Task<void> remove(RANGES::input_range auto const& keys)
    {
        if (!m_mutableStorage)
        {
            BOOST_THROW_EXCEPTION(NotExistsMutableStorage{});
        }

        co_await m_mutableStorage->remove(keys);
        co_return;
    }

    LevelStorage fork() const
    {
        std::unique_lock lock(m_immutablesMutex);
        LevelStorage levelStorage(m_backendStorage);
        levelStorage.m_immutableStorages = m_immutableStorages;

        return levelStorage;
    }

    template <class... Args>
    void newMutable(Args... args)
    {
        std::unique_lock lock(m_immutablesMutex);
        if (m_mutableStorage)
        {
            BOOST_THROW_EXCEPTION(DuplicateMutableStorage{});
        }

        m_mutableStorage = std::make_shared<MutableStorage>(args...);
    }

    void dropMutable() { m_mutableStorage.reset(); }

    void pushMutableToImmutableFront()
    {
        if (!m_mutableStorage)
        {
            BOOST_THROW_EXCEPTION(NotExistsMutableStorage{});
        }
        std::unique_lock lock(m_immutablesMutex);
        m_immutableStorages.push_front(m_mutableStorage);
        m_mutableStorage.reset();
    }

    void popImmutableFront()
    {
        std::unique_lock lock(m_immutablesMutex);
        if (m_immutableStorages.empty())
        {
            BOOST_THROW_EXCEPTION(NotExistsImmutableStorage{});
        }
        m_immutableStorages.pop_front();
    }

    task::Task<void> mergeAndPopImmutableBack()
    {
        std::unique_lock mergeLock(m_mergeMutex);
        std::unique_lock immutablesLock(m_immutablesMutex);
        if (m_immutableStorages.empty())
        {
            BOOST_THROW_EXCEPTION(NotExistsImmutableStorage{});
        }
        auto immutableStorage = std::move(m_immutableStorages.back());
        m_immutableStorages.pop_back();
        immutablesLock.unlock();

        auto it = co_await immutableStorage->seek(transaction_executor::EMPTY_STATE_KEY);
        while (co_await it.next())
        {
            if (co_await it.hasValue())
            {
                co_await storage2::writeOne(
                    m_backendStorage, co_await it.key(), co_await it.value());
            }
            else
            {
                co_await storage2::removeOne(m_backendStorage, co_await it.key());
            }
        }
    }
};
}  // namespace bcos::transaction_scheduler