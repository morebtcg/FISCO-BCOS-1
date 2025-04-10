#pragma once

#include "../protocol/BlockHeader.h"
#include "../protocol/TransactionReceipt.h"
#include "bcos-task/Task.h"

namespace bcos::scheduler_v1
{

inline constexpr struct ExecuteBlock
{
    task::Task<std::vector<std::shared_ptr<protocol::TransactionReceipt>>> operator()(
        auto& scheduler, auto& storage, auto& executor, protocol::BlockHeader const& blockHeader,
        ::ranges::input_range auto transactions, auto&&... args) const
    {
        co_return co_await tag_invoke(*this, scheduler, storage, executor, blockHeader,
            std::move(transactions), std::forward<decltype(args)>(args)...);
    }
} executeBlock{};

template <auto& Tag>
using tag_t = std::decay_t<decltype(Tag)>;

}  // namespace bcos::scheduler_v1