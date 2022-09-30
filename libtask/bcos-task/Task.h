#pragma once
#include "Coroutine.h"
#include <bcos-concepts/Exception.h>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/throw_exception.hpp>
#include <concepts>
#include <coroutine>
#include <exception>
#include <future>
#include <mutex>
#include <type_traits>
#include <variant>

namespace bcos::task
{

// clang-format off
struct NoReturnValue : public bcos::exception::Exception {};
// clang-format on

template <class TaskImpl, class Value>
class TaskBase
{
public:
    friend TaskImpl;

    using ReturnType = Value;

    struct PromiseVoid;
    struct PromiseValue;
    using promise_type = std::conditional_t<std::is_same_v<Value, void>, PromiseVoid, PromiseValue>;

    template <class PromiseImpl>
    struct PromiseBase
    {
        constexpr CO_STD::suspend_always initial_suspend() const noexcept { return {}; }
        constexpr auto final_suspend() const noexcept
        {
            struct FinalAwaitable
            {
                constexpr bool await_ready() const noexcept { return !m_continuationHandle; }
                auto await_suspend([[maybe_unused]] CO_STD::coroutine_handle<> handle) noexcept
                {
                    return m_continuationHandle;
                }
                constexpr void await_resume() noexcept {}

                std::coroutine_handle<> m_continuationHandle;
            };

            return FinalAwaitable{m_continuationHandle};
        }
        constexpr TaskImpl get_return_object()
        {
            return TaskImpl(CO_STD::coroutine_handle<promise_type>::from_promise(
                *static_cast<PromiseImpl*>(this)));
        }
        void unhandled_exception()
        {
            m_value.template emplace<std::exception_ptr>(std::current_exception());
        }

        CO_STD::coroutine_handle<> m_continuationHandle;
        std::conditional_t<std::is_void_v<Value>, std::variant<std::monostate, std::exception_ptr>,
            std::variant<std::monostate, Value, std::exception_ptr>>
            m_value;
    };
    struct PromiseVoid : public PromiseBase<PromiseVoid>
    {
        void return_void() {}
    };
    struct PromiseValue : public PromiseBase<PromiseValue>
    {
        void return_value(Value&& value)
        {
            PromiseBase<PromiseValue>::m_value.template emplace<Value>(std::forward<Value>(value));
        }
    };

    TaskBase(CO_STD::coroutine_handle<promise_type>&& handle) : m_handle(std::move(handle)) {}
    TaskBase(const TaskBase&) = delete;
    TaskBase(TaskBase&&) = default;
    TaskBase& operator=(const TaskBase&) = delete;
    TaskBase& operator=(TaskBase&&) = default;
    ~TaskBase() {}

    constexpr void run() && { m_handle.resume(); }

private:
    CO_STD::coroutine_handle<promise_type> m_handle;
};

enum class Type
{
    LAZY,
    EAGER
};

template <class Value, Type type = Type::LAZY>
class Task : public TaskBase<Task<Value, type>, Value>
{
public:
    using TaskBase<Task<Value>, Value>::TaskBase;
    using typename TaskBase<Task<Value, type>, Value>::ReturnType;
    using typename TaskBase<Task<Value, type>, Value>::promise_type;

    struct Awaitable
    {
        Awaitable(Task const& task) : m_handle(task.m_handle){};
        Awaitable(const Awaitable&) = delete;
        Awaitable(Awaitable&&) = default;
        Awaitable& operator=(const Awaitable&) = delete;
        Awaitable& operator=(Awaitable&&) = default;
        ~Awaitable() {}

        constexpr bool await_ready() const noexcept { return type == Type::EAGER; }

        template <class Promise>
        auto await_suspend(CO_STD::coroutine_handle<Promise> handle)
        {
            m_handle.promise().m_continuationHandle = std::move(handle);
            return m_handle;
        }
        constexpr Value await_resume()
        {
            auto& value = m_handle.promise().m_value;
            if (std::holds_alternative<std::exception_ptr>(value))
            {
                std::rethrow_exception(std::get<std::exception_ptr>(value));
            }

            if constexpr (!std::is_void_v<Value>)
            {
                if (!std::holds_alternative<Value>(value))
                {
                    BOOST_THROW_EXCEPTION(NoReturnValue{});
                }

                auto result = std::move(std::get<Value>(value));
                if (m_handle.promise().m_continuationHandle)
                {
                    m_handle.destroy();
                }
                return result;
            }
        }

        CO_STD::coroutine_handle<promise_type> m_handle;
    };
    Awaitable operator co_await() && { return Awaitable(*static_cast<Task*>(this)); }
    friend Awaitable;
};

}  // namespace bcos::task