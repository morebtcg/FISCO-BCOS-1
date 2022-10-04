#pragma once
#include "Task.h"
#include <boost/exception/diagnostic_information.hpp>
#include <exception>
#include <new>
#include <thread>
#include <type_traits>
#include <variant>

namespace bcos::task
{

template <class Task, class Callback>
void wait(Task task, Callback callback)
{
    struct FinalAwaitable
    {
        constexpr bool await_ready() const noexcept { return false; }
        void await_suspend(CO_STD::coroutine_handle<> handle) noexcept { handle.destroy(); }
        constexpr void await_resume() noexcept {}
    };

    auto waitTask = [](Task task, Callback callback) -> task::Task<void> {
        using TaskType = std::remove_cvref_t<Task>;
        try
        {
            if constexpr (std::is_void_v<typename TaskType::ReturnType>)
            {
                co_await task;
                callback();
            }
            else
            {
                callback(co_await task);
            }
        }
        catch (...)
        {
            callback(std::current_exception());
        }

        co_await FinalAwaitable();
    };
    waitTask(std::move(task), std::move(callback)).run();
}

template <class Task>
void wait(Task task)
{
    wait(std::move(task), []([[maybe_unused]] std::exception_ptr error = nullptr) {});
}

template <class Task>
auto syncWait(Task task)
{
    std::promise<typename Task::ReturnType> promise;
    auto future = promise.get_future();

    if constexpr (std::is_void_v<typename Task::ReturnType>)
    {
        wait(std::move(task), [&promise](std::exception_ptr error = nullptr) {
            if (error)
            {
                promise.set_exception(error);
            }
            else
            {
                promise.set_value();
            }
        });
        future.get();
    }
    else
    {
        wait(std::move(task), [&promise](auto&& value) mutable -> void {
            using ValueType = std::remove_cvref_t<decltype(value)>;
            if constexpr (std::is_same_v<ValueType, std::exception_ptr>)
            {
                promise.set_exception(value);
            }
            else
            {
                promise.set_value(std::forward<decltype(value)>(value));
            }
        });
        return future.get();
    }
}

template <class Task>
auto operator~(Task task)
{
    return syncWait(std::move(task));
}

}  // namespace bcos::task