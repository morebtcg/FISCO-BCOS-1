#pragma once

#include <bcos-concepts/Basic.h>
#include <bcos-concepts/ByteBuffer.h>
#include <bcos-task/Coroutine.h>
#include <bcos-task/Task.h>
#include <bcos-task/Trait.h>
#include <bcos-utilities/Ranges.h>
#include <boost/throw_exception.hpp>
#include <optional>
#include <range/v3/view/transform.hpp>
#include <type_traits>

namespace bcos::storage2
{

template <class IteratorType>
concept Iterator = requires(IteratorType iterator)
{
    typename IteratorType::Key;
    typename IteratorType::Value;

    std::convertible_to<task::AwaiterReturnType<decltype(iterator.hasValue())>, bool>;
    std::convertible_to<task::AwaiterReturnType<decltype(iterator.next())>, bool>;
    std::same_as<typename task::AwaiterReturnType<decltype(iterator.key())>,
        typename IteratorType::Key>;
    std::same_as<typename task::AwaiterReturnType<decltype(iterator.value())>,
        typename IteratorType::Value>;
};

template <class Impl>
class StorageBase
{
public:
    static constexpr std::string_view SYS_TABLES{"s_tables"};

    // *** Pure interfaces
    template <class ImplClass = Impl>
    task::Task<typename ImplClass::ReadIterator> read(RANGES::input_range auto const& keys)
    {
        co_return co_await impl().impl_read(keys);
    }

    template <class ImplClass = Impl>
    task::Task<typename ImplClass::SeekIterator> seek(auto const& key)
    {
        co_return co_await impl().impl_seek(key);
    }

    task::Task<void> write(RANGES::input_range auto&& keys, RANGES::input_range auto&& values)
    {
        co_return co_await impl().impl_write(
            std::forward<decltype(keys)>(keys), std::forward<decltype(values)>(values));
    }

    task::Task<void> remove(RANGES::input_range auto const& keys)
    {
        co_return co_await impl().impl_remove(keys);
    }
    // *** Pure interfaces

    template <class ImplClass = Impl>
    task::Task<std::optional<typename ImplClass::ReadIterator::Value>> readOne(auto const& key)
    {
        using ValueType = typename ImplClass::ReadIterator::Value;
        std::optional<ValueType> value;
        auto it = co_await read(RANGES::single_view(std::addressof(key)) |
                                RANGES::views::transform([](auto const* ptr) { return *ptr; }));
        co_await it.next();
        if (co_await it.hasValue())
        {
            value.emplace(std::move(co_await it.value()));
        }

        co_return value;
    }

    task::Task<void> writeOne(auto&& key, auto&& value)
    {
        using WriteKeyType = decltype(key);
        if constexpr (std::is_lvalue_reference_v<WriteKeyType>)
        {
            // Treat lvalue as const&
            co_return co_await write(
                RANGES::single_view(std::addressof(key)) |
                    RANGES::views::transform([](auto const* ptr) { return *ptr; }),
                RANGES::single_view(std::forward<decltype(value)>(value)));
        }
        co_return co_await write(RANGES::single_view(std::forward<WriteKeyType>(key)),
            RANGES::single_view(std::forward<decltype(value)>(value)));
    }

    task::Task<void> removeOne(auto const& key)
    {
        co_return co_await remove(RANGES::single_view(std::addressof(key)) |
                                  RANGES::views::transform([](auto const* ptr) { return *ptr; }));
    }

private:
    friend Impl;
    auto& impl() { return static_cast<Impl&>(*this); }

    StorageBase() = default;
};

template <class Impl>
concept Storage = std::derived_from<Impl, StorageBase<Impl>>;

}  // namespace bcos::storage2