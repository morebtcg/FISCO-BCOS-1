#pragma once
#include "Coroutine.h"
#include "bcos-concepts/Exception.h"
#include <boost/exception/diagnostic_information.hpp>
#include <boost/throw_exception.hpp>
#include <concepts>
#include <exception>
#include <memory>
#include <type_traits>
#include <variant>

namespace bcos::task
{

struct NoReturnValue : public bcos::error::Exception
{
};

template <class Value>
    requires(!std::is_reference_v<Value>)
class [[nodiscard]] Task
{
public:
    struct PromiseVoid;
    struct PromiseValue;
    using promise_type = std::conditional_t<std::is_same_v<Value, void>, PromiseVoid, PromiseValue>;
    using VariantType =
        std::conditional_t<std::is_void_v<Value>, std::variant<std::monostate, std::exception_ptr>,
            std::variant<std::monostate, Value, std::exception_ptr>>;

    struct Awaitable
    {
        Awaitable(Task const& task) : m_handle(task.m_handle){};
        Awaitable(const Awaitable&) = delete;
        Awaitable(Awaitable&&) noexcept = default;
        Awaitable& operator=(const Awaitable&) = delete;
        Awaitable& operator=(Awaitable&&) noexcept = default;
        ~Awaitable() noexcept = default;

        bool await_ready() const noexcept { return !m_handle || m_handle.done(); }

        template <class Promise>
        CO_STD::coroutine_handle<> await_suspend(CO_STD::coroutine_handle<Promise> handle)
        {
            m_handle.promise().m_continuationHandle = handle;
            m_handle.promise().m_awaitable = this;

            return m_handle;
        }
        Value await_resume()
        {
            if (std::holds_alternative<std::exception_ptr>(m_value))
            {
                std::rethrow_exception(std::get<std::exception_ptr>(m_value));
            }

            if constexpr (!std::is_void_v<Value>)
            {
                if (!std::holds_alternative<Value>(m_value))
                {
                    BOOST_THROW_EXCEPTION(NoReturnValue{});
                }

                return std::move(std::get<Value>(m_value));
            }
        }

        CO_STD::coroutine_handle<promise_type> m_handle;
        VariantType m_value;
    };
    Awaitable operator co_await() { return Awaitable(*static_cast<Task*>(this)); }

    template <class PromiseImpl>
    struct PromiseBase
    {
        constexpr static CO_STD::suspend_always initial_suspend() noexcept { return {}; }
        constexpr static auto final_suspend() noexcept
        {
            struct FinalAwaitable
            {
                constexpr static bool await_ready() noexcept { return false; }
                constexpr static CO_STD::coroutine_handle<> await_suspend(
                    CO_STD::coroutine_handle<PromiseImpl> handle) noexcept
                {
                    auto continuationHandle = handle.promise().m_continuationHandle;
                    handle.destroy();
                    return continuationHandle ? continuationHandle : CO_STD::noop_coroutine();
                }
                constexpr static void await_resume() noexcept {}
            };
            return FinalAwaitable{};
        }
        Task get_return_object()
        {
            auto handle = CO_STD::coroutine_handle<promise_type>::from_promise(
                *static_cast<PromiseImpl*>(this));
            return Task(handle);
        }
        void unhandled_exception()
        {
            if (m_awaitable)
            {
                m_awaitable->m_value.template emplace<std::exception_ptr>(std::current_exception());
            }
            else
            {
                std::rethrow_exception(std::current_exception());
            }
        }

        CO_STD::coroutine_handle<> m_continuationHandle;
        Awaitable* m_awaitable = nullptr;
    };
    struct PromiseVoid : public PromiseBase<PromiseVoid>
    {
        constexpr static void return_void() noexcept {}
    };
    struct PromiseValue : public PromiseBase<PromiseValue>
    {
        void return_value(auto&& value)
        {
            if (PromiseBase<PromiseValue>::m_awaitable)
            {
                PromiseBase<PromiseValue>::m_awaitable->m_value.template emplace<Value>(
                    std::forward<decltype(value)>(value));
            }
        }
    };

    explicit Task(CO_STD::coroutine_handle<promise_type> handle) : m_handle(handle) {}
    Task(const Task&) = default;
    Task(Task&& task) noexcept : m_handle(task.m_handle) { task.m_handle = nullptr; }
    Task& operator=(const Task&) = default;
    Task& operator=(Task&& task) noexcept
    {
        m_handle = task.m_handle;
        task.m_handle = nullptr;
    }
    ~Task() noexcept = default;
    void start() { m_handle.resume(); }

private:
    CO_STD::coroutine_handle<promise_type> m_handle;
};

}  // namespace bcos::task