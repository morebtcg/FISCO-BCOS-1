#pragma once

#include "MultiLayerStorage.h"
#include "ReadWriteSetStorage.h"
#include "bcos-framework/ledger/LedgerConfig.h"
#include "bcos-framework/protocol/Transaction.h"
#include "bcos-framework/protocol/TransactionReceipt.h"
#include "bcos-framework/protocol/TransactionReceiptFactory.h"
#include "bcos-framework/storage2/MemoryStorage.h"
#include "bcos-framework/storage2/Storage.h"
#include "bcos-framework/transaction-executor/TransactionExecutor.h"
#include "bcos-framework/transaction-scheduler/TransactionScheduler.h"
#include "bcos-tars-protocol/protocol/TransactionReceiptImpl.h"
#include "bcos-task/TBBWait.h"
#include "bcos-task/Wait.h"
#include "bcos-utilities/Exceptions.h"
#include "bcos-utilities/ITTAPI.h"
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_pipeline.h>
#include <tbb/cache_aligned_allocator.h>
#include <tbb/task_arena.h>
#include <boost/exception/detail/exception_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <atomic>
#include <cstddef>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace bcos::transaction_scheduler
{

#define PARALLEL_SCHEDULER_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("PARALLEL_SCHEDULER")

class SchedulerParallelImpl
{
    template <class Storage>
    struct StorageTrait
    {
        using LocalStorage = MultiLayerStorage<typename Storage::MutableStorage, void, Storage>;
        using LocalStorageView =
            std::invoke_result_t<decltype(&LocalStorage::fork), LocalStorage, bool>;
        using LocalReadWriteSetStorage =
            ReadWriteSetStorage<LocalStorageView, transaction_executor::StateKey>;
    };
    constexpr static auto TRANSACTION_GRAIN_SIZE = 32L;

    template <class Storage, class Executor, class ContextRange>
    class ChunkStatus
    {
    private:
        auto forkAndMutable(auto& storage)
        {
            storage.newMutable();
            auto view = storage.fork(true);
            return view;
        }

        int64_t m_chunkIndex = 0;
        boost::atomic_flag const& m_hasRAW;
        ContextRange m_contextRange;
        Executor& m_executor;
        typename StorageTrait<Storage>::LocalStorage m_localStorage;
        typename StorageTrait<Storage>::LocalStorageView m_localStorageView;
        typename StorageTrait<Storage>::LocalReadWriteSetStorage m_localReadWriteSetStorage;

    public:
        ChunkStatus(int64_t chunkIndex, boost::atomic_flag const& hasRAW, ContextRange contextRange,
            Executor& executor, auto& storage)
          : m_chunkIndex(chunkIndex),
            m_hasRAW(hasRAW),
            m_contextRange(std::move(contextRange)),
            m_executor(executor),
            m_localStorage(storage),
            m_localStorageView(forkAndMutable(m_localStorage)),
            m_localReadWriteSetStorage(m_localStorageView)
        {}

        int64_t chunkIndex() { return m_chunkIndex; }
        auto count() { return RANGES::size(m_contextRange); }
        auto& localStorage() & { return m_localStorage; }
        auto& readWriteSetStorage() & { return m_localReadWriteSetStorage; }

        void executeStep1(
            protocol::BlockHeader const& blockHeader, ledger::LedgerConfig const& ledgerConfig)
        {
            ittapi::Report report(ittapi::ITT_DOMAINS::instance().PARALLEL_SCHEDULER,
                ittapi::ITT_DOMAINS::instance().EXECUTE_CHUNK1);
            for (auto& context : m_contextRange)
            {
                if (m_hasRAW.test())
                {
                    break;
                }

                context.readWriteSetStorage = std::addressof(m_localReadWriteSetStorage);
                if (!context.coro)
                {
                    context.coro.emplace(transaction_executor::execute3Step(m_executor,
                        *context.readWriteSetStorage, blockHeader, context.transaction,
                        context.contextID, ledgerConfig, task::tbb::syncWait, context.retryFlag,
                        std::addressof(context.readWriteSetStorage)));
                    context.iterator = context.coro->begin();
                }
            }
        }

        void executeStep2()
        {
            ittapi::Report report(ittapi::ITT_DOMAINS::instance().PARALLEL_SCHEDULER,
                ittapi::ITT_DOMAINS::instance().EXECUTE_CHUNK2);
            for (auto& context : m_contextRange)
            {
                if (m_hasRAW.test())
                {
                    break;
                }
                if (context.iterator != context.coro->end())
                {
                    ++context.iterator;
                }
            }
        }

        void executeStep3()
        {
            ittapi::Report report(ittapi::ITT_DOMAINS::instance().PARALLEL_SCHEDULER,
                ittapi::ITT_DOMAINS::instance().EXECUTE_CHUNK3);
            for (auto& context : m_contextRange)
            {
                context.retryFlag = false;
                if (m_hasRAW.test())
                {
                    break;
                }
                if (context.iterator != context.coro->end())
                {
                    ++context.iterator;
                    auto receipt = *(context.iterator);
                    context.receipt = receipt;
                }
            }
        }
    };

public:
    SchedulerParallelImpl(const SchedulerParallelImpl&) = delete;
    SchedulerParallelImpl(SchedulerParallelImpl&&) noexcept = default;
    SchedulerParallelImpl& operator=(const SchedulerParallelImpl&) = delete;
    SchedulerParallelImpl& operator=(SchedulerParallelImpl&&) noexcept = default;

    SchedulerParallelImpl() = default;
    ~SchedulerParallelImpl() noexcept = default;

private:
    static task::Task<void> mergeLastStorage(
        SchedulerParallelImpl& scheduler, auto& storage, auto&& lastStorage)
    {
        ittapi::Report mergeReport(ittapi::ITT_DOMAINS::instance().PARALLEL_SCHEDULER,
            ittapi::ITT_DOMAINS::instance().MERGE_LAST_CHUNK);
        PARALLEL_SCHEDULER_LOG(DEBUG) << "Final merge lastStorage";
        co_await storage2::merge(storage, std::forward<decltype(lastStorage)>(lastStorage));
    }

    static void executeSinglePass(SchedulerParallelImpl& scheduler, auto& storage, auto& executor,
        protocol::BlockHeader const& blockHeader, ledger::LedgerConfig const& ledgerConfig,
        size_t offset, RANGES::random_access_range auto& contexts)
    {
        ittapi::Report report(ittapi::ITT_DOMAINS::instance().PARALLEL_SCHEDULER,
            ittapi::ITT_DOMAINS::instance().SINGLE_PASS);

        auto count = RANGES::size(contexts);
        ReadWriteSetStorage<decltype(storage), transaction_executor::StateKey> writeSet(storage);

        using Chunk = SchedulerParallelImpl::ChunkStatus<std::decay_t<decltype(storage)>,
            std::decay_t<decltype(executor)>,
            decltype(RANGES::subrange<RANGES::iterator_t<decltype(contexts)>>(contexts))>;
        using ChunkStorage = typename std::decay_t<decltype(storage)>::MutableStorage;
        PARALLEL_SCHEDULER_LOG(DEBUG)
            << "Start new chunk executing... " << offset << " | " << RANGES::size(contexts);

        boost::atomic_flag hasRAW;
        ChunkStorage lastStorage;

        static tbb::task_arena arena(8);
        arena.execute([&]() {
            auto contextChunks =
                RANGES::views::drop(contexts, offset) |
                RANGES::views::chunk(std::max<size_t>(
                    (size_t)((count - offset) / tbb::this_task_arena::max_concurrency()),
                    (size_t)TRANSACTION_GRAIN_SIZE));

            RANGES::range_size_t<decltype(contextChunks)> chunkIndex = 0;
            // 五级流水线：分片准备、并行执行、检测RAW冲突&合并读写集、生成回执、合并storage
            // Five-level pipeline: shard preparation, parallel execution, detection of RAW
            // conflicts & merging read/write sets, generating receipts, and merging storage
            tbb::parallel_pipeline(tbb::this_task_arena::max_concurrency(),
                tbb::make_filter<void, std::unique_ptr<Chunk>>(tbb::filter_mode::serial_in_order,
                    [&](tbb::flow_control& control) -> std::unique_ptr<Chunk> {
                        if (chunkIndex >= RANGES::size(contextChunks) || hasRAW.test())
                        {
                            control.stop();
                            return {};
                        }

                        ittapi::Report report(ittapi::ITT_DOMAINS::instance().PARALLEL_SCHEDULER,
                            ittapi::ITT_DOMAINS::instance().STEP_1);
                        PARALLEL_SCHEDULER_LOG(DEBUG) << "Chunk: " << chunkIndex;
                        auto chunk = std::make_unique<Chunk>(
                            chunkIndex, hasRAW, contextChunks[chunkIndex], executor, storage);
                        ++chunkIndex;
                        return chunk;
                    }) &
                    tbb::make_filter<std::unique_ptr<Chunk>, std::unique_ptr<Chunk>>(
                        tbb::filter_mode::parallel,
                        [&](std::unique_ptr<Chunk> chunk) -> std::unique_ptr<Chunk> {
                            ittapi::Report report(
                                ittapi::ITT_DOMAINS::instance().PARALLEL_SCHEDULER,
                                ittapi::ITT_DOMAINS::instance().STEP_2);
                            if (chunk && !hasRAW.test())
                            {
                                chunk->executeStep1(blockHeader, ledgerConfig);
                                chunk->executeStep2();
                            }

                            return chunk;
                        }) &
                    tbb::make_filter<std::unique_ptr<Chunk>, std::unique_ptr<Chunk>>(
                        tbb::filter_mode::serial_in_order,
                        [&](std::unique_ptr<Chunk> chunk) -> std::unique_ptr<Chunk> {
                            ittapi::Report report1(
                                ittapi::ITT_DOMAINS::instance().PARALLEL_SCHEDULER,
                                ittapi::ITT_DOMAINS::instance().STEP_3);
                            if (hasRAW.test())
                            {
                                return {};
                            }

                            auto index = chunk->chunkIndex();
                            if (index > 0)
                            {
                                ittapi::Report report2(
                                    ittapi::ITT_DOMAINS::instance().PARALLEL_SCHEDULER,
                                    ittapi::ITT_DOMAINS::instance().DETECT_RAW);
                                if (writeSet.hasRAWIntersection(chunk->readWriteSetStorage()))
                                {
                                    hasRAW.test_and_set();
                                    PARALLEL_SCHEDULER_LOG(DEBUG)
                                        << "Detected RAW Intersection:" << index;
                                    return {};
                                }
                            }

                            PARALLEL_SCHEDULER_LOG(DEBUG)
                                << "Merging rwset... " << index << " | " << chunk->count();
                            ittapi::Report report3(
                                ittapi::ITT_DOMAINS::instance().PARALLEL_SCHEDULER,
                                ittapi::ITT_DOMAINS::instance().MERGE_RWSET);
                            writeSet.mergeWriteSet(chunk->readWriteSetStorage());
                            return chunk;
                        }) &
                    tbb::make_filter<std::unique_ptr<Chunk>, std::unique_ptr<Chunk>>(
                        tbb::filter_mode::parallel,
                        [&](std::unique_ptr<Chunk> chunk) -> std::unique_ptr<Chunk> {
                            ittapi::Report report(
                                ittapi::ITT_DOMAINS::instance().PARALLEL_SCHEDULER,
                                ittapi::ITT_DOMAINS::instance().STEP_4);
                            if (chunk)
                            {
                                // ittapi::Report report(
                                //     ittapi::ITT_DOMAINS::instance().PARALLEL_SCHEDULER,
                                //     ittapi::ITT_DOMAINS::instance().RELEASE_CONFLICT);
                                chunk->executeStep3();
                            }

                            return chunk;
                        }) &
                    tbb::make_filter<std::unique_ptr<Chunk>, void>(
                        tbb::filter_mode::serial_in_order, [&](std::unique_ptr<Chunk> chunk) {
                            ittapi::Report report1(
                                ittapi::ITT_DOMAINS::instance().PARALLEL_SCHEDULER,
                                ittapi::ITT_DOMAINS::instance().STEP_5);
                            if (chunk)
                            {
                                offset += (size_t)chunk->count();
                                ittapi::Report report2(
                                    ittapi::ITT_DOMAINS::instance().PARALLEL_SCHEDULER,
                                    ittapi::ITT_DOMAINS::instance().MERGE_CHUNK);
                                task::tbb::syncWait(storage2::merge(lastStorage,
                                    std::move(chunk->localStorage().mutableStorage())));
                            }
                        }));
        });


        task::tbb::syncWait(mergeLastStorage(scheduler, storage, std::move(lastStorage)));
        if (offset < count)
        {
            executeSinglePass(
                scheduler, storage, executor, blockHeader, ledgerConfig, offset, contexts);
        }
    }

    template <class CoroType, class Storage>
    struct ExecutionContext
    {
        int contextID;
        const protocol::Transaction& transaction;
        protocol::TransactionReceipt::Ptr& receipt;
        typename StorageTrait<Storage>::LocalReadWriteSetStorage* readWriteSetStorage;
        std::optional<CoroType> coro;
        typename CoroType::Iterator iterator;
        bool retryFlag;
    };

    friend task::Task<std::vector<protocol::TransactionReceipt::Ptr>> tag_invoke(
        tag_t<executeBlock> /*unused*/, SchedulerParallelImpl& scheduler, auto& storage,
        auto& executor, protocol::BlockHeader const& blockHeader,
        RANGES::random_access_range auto const& transactions,
        ledger::LedgerConfig const& ledgerConfig)
    {
        ittapi::Report report(ittapi::ITT_DOMAINS::instance().PARALLEL_SCHEDULER,
            ittapi::ITT_DOMAINS::instance().PARALLEL_EXECUTE);
        std::vector<protocol::TransactionReceipt::Ptr> receipts(RANGES::size(transactions));

        using Storage = std::decay_t<decltype(storage)>;
        using CoroType = std::invoke_result_t<transaction_executor::Execute3Step,
            decltype(executor),
            std::add_lvalue_reference_t<typename StorageTrait<Storage>::LocalReadWriteSetStorage>,
            protocol::BlockHeader const&, protocol::Transaction const&, int,
            ledger::LedgerConfig const&, task::tbb::SyncWait, const bool&,
            std::add_pointer_t<
                std::add_pointer_t<typename StorageTrait<Storage>::LocalReadWriteSetStorage>>>;
        std::vector<ExecutionContext<CoroType, Storage>,
            tbb::cache_aligned_allocator<ExecutionContext<CoroType, Storage>>>
            contexts;
        contexts.reserve(RANGES::size(transactions));
        for (auto index : RANGES::views::iota(0, (int)RANGES::size(transactions)))
        {
            contexts.emplace_back(ExecutionContext<CoroType, Storage>{.contextID = index,
                .transaction = transactions[index],
                .receipt = receipts[index],
                .readWriteSetStorage = {},
                .coro = {},
                .iterator = {},
                .retryFlag = true});
        }

        executeSinglePass(scheduler, storage, executor, blockHeader, ledgerConfig, 0, contexts);
        co_return receipts;
    }
};
}  // namespace bcos::transaction_scheduler