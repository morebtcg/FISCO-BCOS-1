#pragma once

#include "bcos-framework/storage2/MemoryStorage.h"
#include "bcos-table/src/StateStorage.h"
#include "bcos-transaction-executor/vm/VMFactory.h"
#include "precompiled/PrecompiledManager.h"
#include "transaction-executor/bcos-transaction-executor/RollbackableStorage.h"
#include "transaction-executor/bcos-transaction-executor/vm/VMInstance.h"
#include "vm/HostContext.h"
#include <bcos-framework/protocol/BlockHeader.h>
#include <bcos-framework/protocol/TransactionReceiptFactory.h>
#include <bcos-framework/storage2/Storage.h>
#include <bcos-framework/transaction-executor/TransactionExecutor.h>
#include <evmc/evmc.h>
#include <boost/algorithm/hex.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <gsl/util>
#include <iterator>
#include <type_traits>

namespace bcos::transaction_executor
{
#define TRANSACTION_EXECUTOR_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("TRANSACTION_EXECUTOR")

// clang-format off
struct InvalidArgumentsError: public bcos::Error {};
// clang-format on

class TransactionExecutorImpl
{
public:
    TransactionExecutorImpl(
        protocol::TransactionReceiptFactory const& receiptFactory, crypto::Hash::Ptr hashImpl)
      : m_receiptFactory(receiptFactory), m_precompiledManager(std::move(hashImpl))
    {}

private:
    VMFactory m_vmFactory;
    protocol::TransactionReceiptFactory const& m_receiptFactory;
    PrecompiledManager m_precompiledManager;

    template <class Storage, auto waitOperator>
    friend task::Task<protocol::TransactionReceipt::Ptr> tag_invoke(
        tag_t<executeTransaction> /*unused*/, TransactionExecutorImpl& executor, Storage& storage,
        protocol::BlockHeader const& blockHeader, protocol::Transaction const& transaction,
        int contextID, ledger::LedgerConfig const& ledgerConfig)
    {
        try
        {
            if (c_fileLogLevel <= LogLevel::TRACE)
            {
                TRANSACTION_EXECUTOR_LOG(TRACE)
                    << "Execte transaction: " << transaction.hash().hex();
            }

            Rollbackable<std::decay_t<decltype(storage)>> rollbackableStorage(storage);
            auto gasLimit = static_cast<int64_t>(std::get<0>(ledgerConfig.gasLimit()));

            auto toAddress = unhexAddress(transaction.to());
            evmc_message evmcMessage = {.kind = transaction.to().empty() ? EVMC_CREATE : EVMC_CALL,
                .flags = 0,
                .depth = 0,
                .gas = gasLimit,
                .recipient = toAddress,
                .destination_ptr = nullptr,
                .destination_len = 0,
                .sender = (!transaction.sender().empty() &&
                              transaction.sender().size() == sizeof(evmc_address)) ?
                              *(evmc_address*)transaction.sender().data() :
                              evmc_address{},
                .sender_ptr = nullptr,
                .sender_len = 0,
                .input_data = transaction.input().data(),
                .input_size = transaction.input().size(),
                .value = {},
                .create2_salt = {},
                .code_address = toAddress};

            if (blockHeader.number() == 0 &&
                transaction.to() == precompiled::AUTH_COMMITTEE_ADDRESS)
            {
                evmcMessage.kind = EVMC_CREATE;
            }

            int64_t seq = 0;
            HostContext<Storage, waitOperator> hostContext(executor.m_vmFactory,
                rollbackableStorage, blockHeader, evmcMessage, evmcMessage.sender,
                transaction.abi(), contextID, seq, executor.m_precompiledManager, ledgerConfig);
            auto evmcResult = co_await hostContext.execute();

            bcos::bytesConstRef output;
            std::string newContractAddress;
            if (!RANGES::equal(evmcResult.create_address.bytes, executor::EMPTY_EVM_ADDRESS.bytes))
            {
                newContractAddress.reserve(sizeof(evmcResult.create_address) * 2);
                boost::algorithm::hex_lower(evmcResult.create_address.bytes,
                    evmcResult.create_address.bytes + sizeof(evmcResult.create_address.bytes),
                    std::back_inserter(newContractAddress));
            }
            else
            {
                output = {evmcResult.output_data, evmcResult.output_size};
            }

            if (evmcResult.status_code != 0)
            {
                TRANSACTION_EXECUTOR_LOG(DEBUG) << "Transaction revert: " << evmcResult.status_code;
            }

            auto const& logEntries = hostContext.logs();
            auto receipt = executor.m_receiptFactory.createReceipt(gasLimit - evmcResult.gas_left,
                std::move(newContractAddress), logEntries, evmcResult.status_code, output,
                blockHeader.number());

            co_return receipt;
        }
        catch (std::exception& e)
        {
            TRANSACTION_EXECUTOR_LOG(DEBUG)
                << "Execute exception: " << boost::diagnostic_information(e);

            auto receipt = executor.m_receiptFactory.createReceipt(
                0, {}, {}, EVMC_INTERNAL_ERROR, {}, blockHeader.number());
            receipt->setMessage(boost::diagnostic_information(e));
            co_return receipt;
        }
    }
};

}  // namespace bcos::transaction_executor