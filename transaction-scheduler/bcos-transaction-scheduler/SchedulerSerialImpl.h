#pragma once

#include "bcos-framework/ledger/LedgerConfig.h"
#include "bcos-framework/protocol/TransactionReceipt.h"
#include "bcos-framework/transaction-executor/TransactionExecutor.h"
#include "bcos-framework/transaction-scheduler/TransactionScheduler.h"
#include "bcos-task/TBBWait.h"
#include <oneapi/tbb/cache_aligned_allocator.h>
#include <oneapi/tbb/parallel_pipeline.h>
#include <oneapi/tbb/task_arena.h>
#include <atomic>

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
        ittapi::Report report(ittapi::ITT_DOMAINS::instance().SERIAL_SCHEDULER,
            ittapi::ITT_DOMAINS::instance().SERIAL_EXECUTE);

        using CoroType = std::invoke_result_t<transaction_executor::Execute3Step,
            decltype(executor), decltype(storage), decltype(blockHeader),
            RANGES::range_value_t<decltype(transactions)>, int, decltype(ledgerConfig),
            task::tbb::SyncWait>;
        struct Context
        {
            std::optional<CoroType> coro;
            typename CoroType::Iterator iterator;
            protocol::TransactionReceipt::Ptr receipt;
        };

        auto count = static_cast<int64_t>(RANGES::size(transactions));
        std::vector<Context, tbb::cache_aligned_allocator<Context>> contexts(count);

        auto chunks = RANGES::views::iota(0L, count) | RANGES::views::chunk(100);
        auto chunkCount = static_cast<int64_t>(RANGES::size(chunks));

        int64_t startIndex = 0;
        std::atomic_int64_t endIndex = 0;
        // 三级流水线，3个线程
        // Three-stage pipeline, with 3 threads
        tbb::task_arena arena(3);
        arena.execute([&]() {
            tbb::parallel_pipeline(chunkCount,
                tbb::make_filter<void,
                    int>(tbb::filter_mode::serial_in_order, [&](tbb::flow_control& control) {
                    if (startIndex == chunkCount)
                    {
                        control.stop();
                        return startIndex;
                    }
                    return startIndex++;
                }) & tbb::make_filter<int, int>(tbb::filter_mode::parallel, [&](int index) {
                    ittapi::Report report(ittapi::ITT_DOMAINS::instance().SERIAL_SCHEDULER,
                        ittapi::ITT_DOMAINS::instance().STEP_1);
                    for (auto i : chunks[index])
                    {
                        // 如果此处找不到合约代码，有可能下一级流水线会部署，立即中断流水线
                        // If the contract code cannot be found here, it is possible that the next
                        // level of the pipeline will be deployed, and the pipeline will be
                        // interrupted immediately
                        auto& [coro, iterator, receipt] = contexts[i];
                        coro.emplace(transaction_executor::execute3Step(executor, storage,
                            blockHeader, transactions[i], i, ledgerConfig, task::tbb::syncWait));
                        iterator = coro->begin();
                        receipt = *iterator;
                    }
                    return index;
                }) & tbb::make_filter<int, int>(tbb::filter_mode::serial_in_order, [&](int index) {
                    ittapi::Report report(ittapi::ITT_DOMAINS::instance().SERIAL_SCHEDULER,
                        ittapi::ITT_DOMAINS::instance().STEP_2);
                    for (auto i : chunks[index])
                    {
                        auto& [coro, iterator, receipt] = contexts[i];
                        if (!receipt)
                        {
                            receipt = *(++iterator);
                        }
                    }
                    return index;
                }) & tbb::make_filter<int, void>(tbb::filter_mode::parallel, [&](int index) {
                    ittapi::Report report(ittapi::ITT_DOMAINS::instance().SERIAL_SCHEDULER,
                        ittapi::ITT_DOMAINS::instance().STEP_3);
                    for (auto i : chunks[index])
                    {
                        auto& [coro, iterator, receipt] = contexts[i];
                        if (!receipt)
                        {
                            receipt = *(++iterator);
                        }
                        coro.reset();
                    }
                }));
        });

        std::vector<protocol::TransactionReceipt::Ptr> receipts;
        RANGES::move(RANGES::views::transform(
                         contexts, [](Context & context) -> auto& { return context.receipt; }),
            RANGES::back_inserter(receipts));
        co_return receipts;
    }
};
}  // namespace bcos::transaction_scheduler