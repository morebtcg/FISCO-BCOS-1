#include "LedgerMethods.h"
#include "bcos-framework/ledger/Features.h"
#include "bcos-framework/ledger/LedgerConfig.h"
#include <bcos-executor/src/Common.h>
#include <boost/exception/diagnostic_information.hpp>
#include <exception>
#include <functional>

bcos::task::Task<void> bcos::ledger::prewriteBlockToStorage(LedgerInterface& ledger,
    bcos::protocol::ConstTransactionsPtr transactions, bcos::protocol::Block::ConstPtr block,
    bool withTransactionsAndReceipts, storage::StorageInterface::Ptr storage)
{
    struct Awaitable
    {
        std::reference_wrapper<LedgerInterface> m_ledger;
        decltype(transactions) m_transactions;
        decltype(block) m_block;
        bool m_withTransactionsAndReceipts{};
        decltype(storage) m_storage;

        Error::Ptr m_error;

        constexpr static bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            m_ledger.get().asyncPrewriteBlock(
                m_storage, std::move(m_transactions), std::move(m_block),
                [this, handle](std::string, Error::Ptr error) {
                    if (error)
                    {
                        m_error = std::move(error);
                    }
                    handle.resume();
                },
                m_withTransactionsAndReceipts, std::nullopt);
        }
        void await_resume()
        {
            if (m_error)
            {
                BOOST_THROW_EXCEPTION(*m_error);
            }
        }
    };

    Awaitable awaitable{.m_ledger = ledger,
        .m_transactions = std::move(transactions),
        .m_block = std::move(block),
        .m_withTransactionsAndReceipts = withTransactionsAndReceipts,
        .m_storage = std::move(storage),
        .m_error = {}};
    co_await awaitable;
}

bcos::task::Task<void> bcos::ledger::tag_invoke(
    ledger::tag_t<storeTransactionsAndReceipts> /*unused*/, LedgerInterface& ledger,
    bcos::protocol::ConstTransactionsPtr blockTxs, bcos::protocol::Block::ConstPtr block)
{
    ledger.storeTransactionsAndReceipts(std::move(blockTxs), std::move(block));
    co_return;
}

void bcos::ledger::tag_invoke(ledger::tag_t<removeExpiredNonce> /*unused*/, LedgerInterface& ledger,
    protocol::BlockNumber expiredNumber)
{
    ledger.removeExpiredNonce(expiredNumber, false);
}

bcos::task::Task<bcos::protocol::Block::Ptr> bcos::ledger::tag_invoke(
    ledger::tag_t<getBlockData> /*unused*/, LedgerInterface& ledger,
    protocol::BlockNumber blockNumber, int32_t blockFlag)
{
    struct Awaitable
    {
        std::reference_wrapper<LedgerInterface> m_ledger;
        protocol::BlockNumber m_blockNumber;
        int32_t m_blockFlag;

        std::variant<Error::Ptr, bcos::protocol::Block::Ptr> m_result;

        constexpr static bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            m_ledger.get().asyncGetBlockDataByNumber(m_blockNumber, m_blockFlag,
                [this, handle](Error::Ptr error, bcos::protocol::Block::Ptr block) {
                    if (error)
                    {
                        m_result.emplace<Error::Ptr>(std::move(error));
                    }
                    else
                    {
                        m_result.emplace<bcos::protocol::Block::Ptr>(std::move(block));
                    }
                    handle.resume();
                });
        }
        bcos::protocol::Block::Ptr await_resume()
        {
            if (std::holds_alternative<Error::Ptr>(m_result))
            {
                BOOST_THROW_EXCEPTION(*std::get<Error::Ptr>(m_result));
            }
            return std::move(std::get<bcos::protocol::Block::Ptr>(m_result));
        }
    };
    Awaitable awaitable{
        .m_ledger = ledger, .m_blockNumber = blockNumber, .m_blockFlag = blockFlag, .m_result = {}};

    co_return co_await awaitable;
}

bcos::task::Task<bcos::ledger::TransactionCount> bcos::ledger::tag_invoke(
    ledger::tag_t<getTransactionCount> /*unused*/, LedgerInterface& ledger)
{
    struct Awaitable
    {
        std::reference_wrapper<LedgerInterface> m_ledger;
        std::variant<Error::Ptr, TransactionCount> m_result;

        constexpr static bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            m_ledger.get().asyncGetTotalTransactionCount(
                [this, handle](Error::Ptr error, int64_t total, int64_t failed,
                    bcos::protocol::BlockNumber blockNumber) {
                    if (error)
                    {
                        m_result.emplace<Error::Ptr>(std::move(error));
                    }
                    else
                    {
                        m_result.emplace<TransactionCount>(TransactionCount{
                            .total = total,
                            .failed = failed,
                            .blockNumber = blockNumber,
                        });
                    }
                    handle.resume();
                });
        }
        TransactionCount await_resume()
        {
            if (std::holds_alternative<Error::Ptr>(m_result))
            {
                BOOST_THROW_EXCEPTION(*std::get<Error::Ptr>(m_result));
            }
            return std::get<TransactionCount>(m_result);
        }
    };

    Awaitable awaitable{.m_ledger = ledger, .m_result = {}};
    co_return co_await awaitable;
}
bcos::task::Task<bcos::protocol::BlockNumber> bcos::ledger::tag_invoke(
    ledger::tag_t<getCurrentBlockNumber> /*unused*/, LedgerInterface& ledger)
{
    struct Awaitable
    {
        std::reference_wrapper<LedgerInterface> m_ledger;
        std::variant<Error::Ptr, protocol::BlockNumber> m_result;

        constexpr static bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            m_ledger.get().asyncGetBlockNumber(
                [this, handle](Error::Ptr error, bcos::protocol::BlockNumber blockNumber) {
                    if (error)
                    {
                        m_result.emplace<Error::Ptr>(std::move(error));
                    }
                    else
                    {
                        m_result.emplace<protocol::BlockNumber>(blockNumber);
                    }
                    handle.resume();
                });
        }
        protocol::BlockNumber await_resume()
        {
            if (std::holds_alternative<Error::Ptr>(m_result))
            {
                BOOST_THROW_EXCEPTION(*std::get<Error::Ptr>(m_result));
            }
            return std::get<protocol::BlockNumber>(m_result);
        }
    };

    Awaitable awaitable{.m_ledger = ledger, .m_result = {}};
    co_return co_await awaitable;
}
bcos::task::Task<bcos::crypto::HashType> bcos::ledger::tag_invoke(
    ledger::tag_t<getBlockHash> /*unused*/, LedgerInterface& ledger,
    protocol::BlockNumber blockNumber)
{
    struct Awaitable
    {
        LedgerInterface& m_ledger;
        protocol::BlockNumber m_blockNumber;

        std::variant<Error::Ptr, crypto::HashType> m_result;

        constexpr static bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            m_ledger.asyncGetBlockHashByNumber(
                m_blockNumber, [this, handle](Error::Ptr error, crypto::HashType hash) {
                    if (error)
                    {
                        m_result.emplace<Error::Ptr>(std::move(error));
                    }
                    else
                    {
                        m_result.emplace<crypto::HashType>(hash);
                    }
                    handle.resume();
                });
        }
        crypto::HashType await_resume()
        {
            if (std::holds_alternative<Error::Ptr>(m_result))
            {
                BOOST_THROW_EXCEPTION(*std::get<Error::Ptr>(m_result));
            }
            return std::get<crypto::HashType>(m_result);
        }
    };

    Awaitable awaitable{.m_ledger = ledger, .m_blockNumber = blockNumber, .m_result = {}};
    co_return co_await awaitable;
}

bcos::task::Task<bcos::protocol::BlockNumber> bcos::ledger::tag_invoke(
    bcos::ledger::tag_t<bcos::ledger::getBlockNumber> /*unused*/,
    bcos::ledger::LedgerInterface& ledger, bcos::crypto::HashType hash)
{
    struct Awaitable
    {
        bcos::ledger::LedgerInterface& m_ledger;
        bcos::crypto::HashType m_hash;

        std::variant<bcos::Error::Ptr, bcos::protocol::BlockNumber> m_result;

        constexpr static bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            m_ledger.asyncGetBlockNumberByHash(
                m_hash, [this, handle](bcos::Error::Ptr error, bcos::protocol::BlockNumber number) {
                    if (error)
                    {
                        m_result.emplace<bcos::Error::Ptr>(std::move(error));
                    }
                    else
                    {
                        m_result.emplace<bcos::protocol::BlockNumber>(number);
                    }
                    handle.resume();
                });
        }
        bcos::protocol::BlockNumber await_resume()
        {
            if (std::holds_alternative<bcos::Error::Ptr>(m_result))
            {
                BOOST_THROW_EXCEPTION(*std::get<bcos::Error::Ptr>(m_result));
            }
            return std::get<bcos::protocol::BlockNumber>(m_result);
        }
    };

    Awaitable awaitable{.m_ledger = ledger, .m_hash = hash, .m_result = {}};
    co_return co_await awaitable;
}

bcos::task::Task<std::optional<bcos::ledger::SystemConfigEntry>> bcos::ledger::tag_invoke(
    ledger::tag_t<getSystemConfig> /*unused*/, LedgerInterface& ledger, std::string_view key)
{
    struct Awaitable
    {
        LedgerInterface& m_ledger;
        std::string_view m_key;
        std::variant<Error::Ptr, std::optional<SystemConfigEntry>> m_result;

        constexpr static bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            m_ledger.asyncGetSystemConfigByKey(
                m_key, [this, handle](Error::Ptr error, std::string value,
                           bcos::protocol::BlockNumber blockNumber) {
                    if (error)
                    {
                        if (error->errorCode() == LedgerError::EmptyEntry ||
                            error->errorCode() == LedgerError::ErrorArgument ||
                            error->errorCode() == LedgerError::GetStorageError)
                        {
                            m_result.emplace<std::optional<SystemConfigEntry>>();
                        }
                        else
                        {
                            m_result.emplace<Error::Ptr>(std::move(error));
                        }
                    }
                    else
                    {
                        m_result.emplace<std::optional<SystemConfigEntry>>(
                            std::in_place, std::move(value), blockNumber);
                    }
                    handle.resume();
                });
        }
        std::optional<SystemConfigEntry> await_resume()
        {
            if (std::holds_alternative<Error::Ptr>(m_result))
            {
                BOOST_THROW_EXCEPTION(*std::get<Error::Ptr>(m_result));
            }
            return std::move(std::get<std::optional<SystemConfigEntry>>(m_result));
        }
    };

    Awaitable awaitable{.m_ledger = ledger, .m_key = key, .m_result = {}};
    co_return co_await awaitable;
}

bcos::task::Task<bcos::consensus::ConsensusNodeList> bcos::ledger::tag_invoke(
    ledger::tag_t<getNodeList> /*unused*/, LedgerInterface& ledger, std::string_view type)
{
    struct Awaitable
    {
        LedgerInterface& m_ledger;
        std::string_view m_type;
        std::variant<Error::Ptr, consensus::ConsensusNodeList> m_result;

        constexpr static bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            m_ledger.asyncGetNodeListByType(m_type,
                [this, handle](Error::Ptr error, consensus::ConsensusNodeList consensusNodeList) {
                    if (error)
                    {
                        m_result.emplace<Error::Ptr>(std::move(error));
                    }
                    else
                    {
                        m_result.emplace<consensus::ConsensusNodeList>(
                            std::move(consensusNodeList));
                    }
                    handle.resume();
                });
        }
        consensus::ConsensusNodeList await_resume()
        {
            if (std::holds_alternative<Error::Ptr>(m_result))
            {
                BOOST_THROW_EXCEPTION(*std::get<Error::Ptr>(m_result));
            }
            return std::move(std::get<consensus::ConsensusNodeList>(m_result));
        }
    };

    Awaitable awaitable{.m_ledger = ledger, .m_type = type, .m_result = {}};
    co_return co_await awaitable;
}

bcos::task::Task<void> bcos::ledger::tag_invoke(
    ledger::tag_t<getLedgerConfig> /*unused*/, LedgerInterface& ledger, LedgerConfig& ledgerConfig)
{
    auto nodeList = co_await getNodeList(ledger, {});
    ledgerConfig.setConsensusNodeList(::ranges::views::filter(nodeList, [](auto& node) {
        return node.type == consensus::Type::consensus_sealer;
    }) | ::ranges::to<std::vector>());
    ledgerConfig.setObserverNodeList(::ranges::views::filter(nodeList, [](auto& node) {
        return node.type == consensus::Type::consensus_observer;
    }) | ::ranges::to<std::vector>());

    auto blockNumber = co_await getCurrentBlockNumber(ledger);
    ledgerConfig.setBlockNumber(blockNumber);
    ledgerConfig.setHash(co_await getBlockHash(ledger, blockNumber));
    ledgerConfig.setFeatures(co_await getFeatures(ledger));
    co_await ledger::readFromStorage(
        ledgerConfig.m_systemConfigs, *ledger.stateStorage(), blockNumber);
}

bcos::task::Task<bcos::ledger::Features> bcos::ledger::tag_invoke(
    ledger::tag_t<getFeatures> /*unused*/, LedgerInterface& ledger)
{
    auto blockNumber = co_await getCurrentBlockNumber(ledger);
    Features features;
    for (auto key : bcos::ledger::Features::featureKeys())
    {
        try
        {
            auto value = co_await getSystemConfig(ledger, key);
            if (!value)
            {
                LEDGER2_LOG(DEBUG) << "Not found system config: " << key;
                continue;
            }

            if (blockNumber + 1 >= std::get<1>(*value))
            {
                features.set(key);
            }
        }
        catch (std::exception& e)
        {
            LEDGER2_LOG(DEBUG) << "Not found system config: " << key;
        }
    }

    co_return features;
}

bcos::task::Task<bcos::protocol::TransactionReceipt::ConstPtr> bcos::ledger::tag_invoke(
    ledger::tag_t<getReceipt> /*unused*/, LedgerInterface& ledger, crypto::HashType const& txHash)
{
    struct Awaitable
    {
        bcos::ledger::LedgerInterface& m_ledger;
        bcos::crypto::HashType m_hash;

        std::variant<bcos::Error::Ptr, bcos::protocol::TransactionReceipt::ConstPtr> m_result;

        constexpr static bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            m_ledger.asyncGetTransactionReceiptByHash(m_hash, false,
                [this, handle](bcos::Error::Ptr error,
                    const bcos::protocol::TransactionReceipt::ConstPtr& receipt, MerkleProofPtr) {
                    if (error)
                    {
                        m_result.emplace<bcos::Error::Ptr>(std::move(error));
                    }
                    else
                    {
                        m_result.emplace<bcos::protocol::TransactionReceipt::ConstPtr>(receipt);
                    }
                    handle.resume();
                });
        }
        bcos::protocol::TransactionReceipt::ConstPtr await_resume()
        {
            if (std::holds_alternative<bcos::Error::Ptr>(m_result))
            {
                BOOST_THROW_EXCEPTION(*std::get<bcos::Error::Ptr>(m_result));
            }
            return std::get<bcos::protocol::TransactionReceipt::ConstPtr>(m_result);
        }
    };

    Awaitable awaitable{.m_ledger = ledger, .m_hash = txHash, .m_result = {}};
    co_return co_await awaitable;
}

bcos::task::Task<bcos::protocol::TransactionsConstPtr> bcos::ledger::tag_invoke(
    ledger::tag_t<getTransactions> /*unused*/, LedgerInterface& ledger, crypto::HashListPtr hashes)
{
    struct Awaitable
    {
        bcos::ledger::LedgerInterface& m_ledger;
        bcos::crypto::HashListPtr m_hashes;

        std::variant<bcos::Error::Ptr, bcos::protocol::TransactionsConstPtr> m_result;

        constexpr static bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            m_ledger.asyncGetBatchTxsByHashList(
                std::move(m_hashes), false, [this, handle](auto&& error, auto&& txs, auto&&) {
                    if (error)
                    {
                        m_result.emplace<bcos::Error::Ptr>(std::forward<decltype(error)>(error));
                    }
                    else
                    {
                        m_result.emplace<bcos::protocol::TransactionsConstPtr>(txs);
                    }
                    handle.resume();
                });
        }
        bcos::protocol::TransactionsConstPtr await_resume()
        {
            if (std::holds_alternative<bcos::Error::Ptr>(m_result))
            {
                BOOST_THROW_EXCEPTION(*std::get<bcos::Error::Ptr>(m_result));
            }
            return std::get<bcos::protocol::TransactionsConstPtr>(m_result);
        }
    };

    Awaitable awaitable{.m_ledger = ledger, .m_hashes = std::move(hashes), .m_result = {}};
    co_return co_await awaitable;
}

bcos::task::Task<std::optional<bcos::storage::Entry>> bcos::ledger::tag_invoke(
    ledger::tag_t<getStorageAt> /*unused*/, LedgerInterface& ledger, std::string_view address,
    std::string_view key, bcos::protocol::BlockNumber number)
{
    co_return co_await ledger.getStorageAt(address, key, number);
}

bcos::task::Task<
    std::shared_ptr<std::map<bcos::protocol::BlockNumber, bcos::protocol::NonceListPtr>>>
bcos::ledger::tag_invoke(ledger::tag_t<getNonceList> /*unused*/, LedgerInterface& ledger,
    bcos::protocol::BlockNumber startNumber, int64_t offset)
{
    struct Awaitable
    {
        bcos::ledger::LedgerInterface& m_ledger;
        bcos::protocol::BlockNumber m_startNumber;
        int64_t m_offset;

        std::variant<bcos::Error::Ptr,
            std::shared_ptr<std::map<protocol::BlockNumber, protocol::NonceListPtr>>>
            m_result;

        constexpr static bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            m_ledger.asyncGetNonceList(m_startNumber, m_offset,
                [this, handle](auto&& error,
                    std::shared_ptr<std::map<protocol::BlockNumber, protocol::NonceListPtr>>
                        nonceList) {
                    if (error)
                    {
                        m_result.emplace<bcos::Error::Ptr>(std::forward<decltype(error)>(error));
                    }
                    else
                    {
                        m_result.emplace<decltype(nonceList)>(std::move(nonceList));
                    }
                    handle.resume();
                });
        }
        std::shared_ptr<std::map<protocol::BlockNumber, protocol::NonceListPtr>> await_resume()
        {
            if (std::holds_alternative<bcos::Error::Ptr>(m_result))
            {
                BOOST_THROW_EXCEPTION(*std::get<bcos::Error::Ptr>(m_result));
            }
            return std::get<
                std::shared_ptr<std::map<protocol::BlockNumber, protocol::NonceListPtr>>>(m_result);
        }
    };

    Awaitable awaitable{
        .m_ledger = ledger, .m_startNumber = startNumber, .m_offset = offset, .m_result = {}};
    co_return co_await awaitable;
}
