#pragma once

#include "bcos-framework/ledger/LedgerConfig.h"
#include "bcos-framework/protocol/TransactionReceipt.h"
#include "bcos-framework/transaction-executor/TransactionExecutor.h"
#include "bcos-framework/transaction-scheduler/TransactionScheduler.h"
#include "bcos-task/Wait.h"
#include <oneapi/tbb/cache_aligned_allocator.h>
#include <oneapi/tbb/parallel_pipeline.h>

namespace bcos::transaction_scheduler
{

#define SERIAL_SCHEDULER_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("SERIAL_SCHEDULER")

class SchedulerSerialImpl
{
private:
    friend task::Task<std::vector<protocol::TransactionReceipt::Ptr>> tag_invoke(
        tag_t<executeBlock> /*unused*/, SchedulerSerialImpl& /*unused*/, auto& storage,
        auto& executor, protocol::BlockHeader const& blockHeader,
        RANGES::input_range auto const& transactions, ledger::LedgerConfig const& ledgerConfig)
    {
        using CoroType = std::invoke_result_t<transaction_executor::Execute3Step,
            decltype(executor), decltype(storage), decltype(blockHeader),
            RANGES::range_value_t<decltype(transactions)>, int, decltype(ledgerConfig),
            task::SyncWait>;
        struct Context
        {
            std::optional<CoroType> coro;
            typename CoroType::Iterator iterator;
            protocol::TransactionReceipt::Ptr receipt;
        };

        auto count = static_cast<int>(RANGES::size(transactions));
        std::vector<Context, tbb::cache_aligned_allocator<Context>> contexts(count);

        int index = 0;
        tbb::parallel_pipeline(count,
            tbb::make_filter<void, int>(tbb::filter_mode::serial_in_order,
                [&](tbb::flow_control& control) {
                    if (index == count)
                    {
                        control.stop();
                        return index;
                    }
                    auto& [coro, iterator, receipt] = contexts[index];
                    coro.emplace(transaction_executor::execute3Step(executor, storage, blockHeader,
                        transactions[index], index, ledgerConfig, task::syncWait));
                    iterator = coro->begin();

                    return index++;
                }) &
                tbb::make_filter<int, int>(tbb::filter_mode::serial_in_order,
                    [&](int index) {
                        auto& [coro, iterator, receipt] = contexts[index];
                        if (!receipt)
                        {
                            receipt = *(++iterator);
                        }
                        return index;
                    }) &
                tbb::make_filter<int, void>(tbb::filter_mode::serial_in_order, [&](int index) {
                    auto& [coro, iterator, receipt] = contexts[index];
                    if (!receipt)
                    {
                        receipt = *(++iterator);
                    }
                }));

        std::vector<protocol::TransactionReceipt::Ptr> receipts;
        RANGES::move(
            RANGES::views::transform(contexts, [](Context& context) { return context.receipt; }),
            RANGES::back_inserter(receipts));
        co_return receipts;
    }
};
}  // namespace bcos::transaction_scheduler