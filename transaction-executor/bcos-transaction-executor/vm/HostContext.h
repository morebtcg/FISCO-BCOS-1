/*
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @brief host context
 * @file HostContext.h
 * @author: ancelmo
 * @date: 2022-12-24
 */

#pragma once

#include "../precompiled/AuthCheck.h"
#include "../precompiled/PrecompiledImpl.h"
#include "../precompiled/PrecompiledManager.h"
#include "EVMHostInterface.h"
#include "VMFactory.h"
#include "bcos-concepts/ByteBuffer.h"
#include "bcos-crypto/hasher/Hasher.h"
#include "bcos-executor/src/Common.h"
#include "bcos-framework/ledger/Account.h"
#include "bcos-framework/ledger/EVMAccount.h"
#include "bcos-framework/ledger/LedgerConfig.h"
#include "bcos-framework/ledger/LedgerTypeDef.h"
#include "bcos-framework/protocol/BlockHeader.h"
#include "bcos-framework/protocol/LogEntry.h"
#include "bcos-framework/protocol/Protocol.h"
#include "bcos-framework/storage2/MemoryStorage.h"
#include "bcos-framework/storage2/Storage.h"
#include "bcos-framework/transaction-executor/TransactionExecutor.h"
#include "bcos-transaction-executor/vm/VMInstance.h"
#include "bcos-utilities/Common.h"
#include <bcos-task/Wait.h>
#include <evmc/evmc.h>
#include <evmc/helpers.h>
#include <evmc/instructions.h>
#include <evmone/evmone.h>
#include <fmt/format.h>
#include <boost/multiprecision/cpp_int/import_export.hpp>
#include <boost/throw_exception.hpp>
#include <atomic>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string_view>

namespace bcos::transaction_executor
{

#define HOST_CONTEXT_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("HOST_CONTEXT")

// clang-format off
struct NotFoundCodeError : public bcos::Error {};
// clang-format on

evmc_bytes32 evm_hash_fn(const uint8_t* data, size_t size);
executor::VMSchedule const& vmSchedule();
static const auto mode = toRevision(vmSchedule());

template <class Storage>
class HostContext : public evmc_host_context
{
private:
    using Account = ledger::account::EVMAccount<Storage>;
    struct Executable
    {
        Executable(storage::Entry code)
          : m_code(std::make_optional(std::move(code))),
            m_vmInstance(VMFactory::create(VMKind::evmone,
                bytesConstRef((const uint8_t*)m_code->data(), m_code->size()), mode))
        {}
        Executable(bytesConstRef code) : m_vmInstance(VMFactory::create(VMKind::evmone, code, mode))
        {}

        std::optional<storage::Entry> m_code;
        VMInstance m_vmInstance;
    };

    Storage& m_rollbackableStorage;
    protocol::BlockHeader const& m_blockHeader;
    const evmc_address& m_origin;
    std::string_view m_abi;
    int m_contextID;
    int64_t& m_seq;
    PrecompiledManager const& m_precompiledManager;
    ledger::LedgerConfig const& m_ledgerConfig;
    crypto::Hash const& m_hashImpl;
    std::variant<const evmc_message*, evmc_message> m_message;
    Account m_myAccount;

    std::vector<protocol::LogEntry> m_logs;
    std::shared_ptr<Executable> m_executable;
    const bcos::transaction_executor::Precompiled* m_preparedPrecompiled{};

    auto buildLegacyExternalCaller()
    {
        return
            [this](const evmc_message& message) { return task::syncWait(externalCall(message)); };
    }

    std::variant<const evmc_message*, evmc_message> getMessage(const evmc_message& inputMessage)
    {
        std::variant<const evmc_message*, evmc_message> message;
        switch (inputMessage.kind)
        {
        case EVMC_CREATE:
        {
            message.emplace<evmc_message>(inputMessage);
            auto& ref = std::get<evmc_message>(message);

            if (concepts::bytebuffer::equalTo(
                    inputMessage.code_address.bytes, executor::EMPTY_EVM_ADDRESS.bytes))
            {
                auto address = fmt::format(
                    FMT_COMPILE("{}_{}_{}"), m_blockHeader.number(), m_contextID, m_seq);
                auto hash = m_hashImpl.hash(address);
                std::copy_n(hash.data(), sizeof(ref.code_address.bytes), ref.code_address.bytes);
            }
            ref.recipient = ref.code_address;
            break;
        }
        case EVMC_CREATE2:
        {
            message.emplace<evmc_message>(inputMessage);
            auto& ref = std::get<evmc_message>(m_message);

            auto field1 = m_hashImpl.hash(bytes{0xff});
            auto field2 = bytesConstRef(ref.sender.bytes, sizeof(ref.sender.bytes));
            auto field3 = toBigEndian(fromEvmC(inputMessage.create2_salt));
            auto field4 = m_hashImpl.hash(bytesConstRef(ref.input_data, ref.input_size));
            auto hashView = RANGES::views::concat(field1, field2, field3, field4);

            std::copy_n(
                hashView.begin() + 12, sizeof(ref.code_address.bytes), ref.code_address.bytes);
            ref.recipient = ref.code_address;
            break;
        }
        default:
        {
            message.emplace<const evmc_message*>(std::addressof(inputMessage));
            break;
        }
        }
        return message;
    }

    evmc_message const& message() const&
    {
        return std::visit(
            bcos::overloaded{
                [this](const evmc_message* message) -> evmc_message const& { return *message; },
                [this](const evmc_message& message) -> evmc_message const& { return message; }},
            m_message);
    }

    auto getMyAccount() { return Account(m_rollbackableStorage, message().recipient); }

    constexpr static struct InnerConstructor
    {
    } innerConstructor{};

    HostContext(InnerConstructor /*unused*/, Storage& storage,
        const protocol::BlockHeader& blockHeader, const evmc_message& message,
        const evmc_address& origin, std::string_view abi, int contextID, int64_t& seq,
        PrecompiledManager const& precompiledManager, ledger::LedgerConfig const& ledgerConfig,
        crypto::Hash const& hashImpl, const evmc_host_interface* hostInterface)
      : evmc_host_context{.interface = hostInterface,
            .wasm_interface = nullptr,
            .hash_fn = evm_hash_fn,
            .isSMCrypto = (hashImpl.getHashImplType() == crypto::HashImplType::Sm3Hash),
            .version = 0,
            .metrics = std::addressof(executor::ethMetrics)},
        m_rollbackableStorage(storage),
        m_blockHeader(blockHeader),
        m_origin(origin),
        m_abi(abi),
        m_contextID(contextID),
        m_seq(seq),
        m_precompiledManager(precompiledManager),
        m_ledgerConfig(ledgerConfig),
        m_hashImpl(hashImpl),
        m_message(getMessage(message)),
        m_myAccount(getMyAccount())
    {}

public:
    HostContext(Storage& storage, protocol::BlockHeader const& blockHeader,
        const evmc_message& message, const evmc_address& origin, std::string_view abi,
        int contextID, int64_t& seq, PrecompiledManager const& precompiledManager,
        ledger::LedgerConfig const& ledgerConfig, crypto::Hash const& hashImpl, auto&& waitOperator)
      : HostContext(innerConstructor, storage, blockHeader, message, origin, abi, contextID, seq,
            precompiledManager, ledgerConfig, hashImpl,
            getHostInterface<HostContext>(std::forward<decltype(waitOperator)>(waitOperator)))
    {}

    ~HostContext() noexcept = default;
    HostContext(HostContext const&) = delete;
    HostContext& operator=(HostContext const&) = delete;
    HostContext(HostContext&&) = delete;
    HostContext& operator=(HostContext&&) = delete;

    task::Task<evmc_bytes32> get(const evmc_bytes32* key)
    {
        co_return co_await ledger::account::storage(m_myAccount, *key);
    }

    task::Task<void> set(const evmc_bytes32* key, const evmc_bytes32* value)
    {
        co_await ledger::account::setStorage(m_myAccount, *key, *value);
    }

    task::Task<std::optional<storage::Entry>> code(const evmc_address& address)
    {
        auto executable = co_await getExecutable(m_rollbackableStorage, address);
        if (executable && executable->m_code)
        {
            co_return executable->m_code;
        }
        co_return std::optional<storage::Entry>{};
    }

    task::Task<size_t> codeSizeAt(const evmc_address& address)
    {
        if (m_precompiledManager.getPrecompiled(address) != nullptr)
        {
            co_return 1;
        }

        if (auto codeEntry = co_await code(address))
        {
            co_return codeEntry->get().size();
        }
        co_return 0;
    }

    task::Task<h256> codeHashAt(const evmc_address& address)
    {
        Account account(m_rollbackableStorage, address);
        co_return co_await ledger::account::codeHash(account);
    }

    task::Task<bool> exists([[maybe_unused]] const evmc_address& address)
    {
        // TODO: impl the full suport for solidity
        co_return true;
    }

    /// Hash of a block if within the last 256 blocks, or h256() otherwise.
    task::Task<h256> blockHash(int64_t number) const
    {
        if (number >= blockNumber() || number < 0)
        {
            co_return h256{};
        }

        BOOST_THROW_EXCEPTION(std::runtime_error("Unsupported method!"));
        co_return h256{};
    }
    int64_t blockNumber() const { return m_blockHeader.number(); }
    uint32_t blockVersion() const { return m_blockHeader.version(); }
    int64_t timestamp() const { return m_blockHeader.timestamp(); }
    evmc_address const& origin() const { return m_origin; }
    int64_t blockGasLimit() const { return std::get<0>(m_ledgerConfig.gasLimit()); }

    /// Revert any changes made (by any of the other calls).
    void log(h256s topics, bytesConstRef data)
    {
        m_logs.emplace_back(bytes{}, std::move(topics), data.toBytes());
    }

    void suicide()
    {
        // suicide(m_myContractTable); // TODO: add suicide
    }

    task::Task<void> prepare()
    {
        assert(!concepts::bytebuffer::equalTo(
            message().code_address.bytes, executor::EMPTY_EVM_ADDRESS.bytes));
        assert(!concepts::bytebuffer::equalTo(
            message().recipient.bytes, executor::EMPTY_EVM_ADDRESS.bytes));
        if (message().kind == EVMC_CREATE || message().kind == EVMC_CREATE2)
        {
            prepareCreate();
        }
        else
        {
            co_await prepareCall();
        }
    }

    task::Task<EVMCResult> execute()
    {
        if (m_ledgerConfig.authCheckStatus() != 0U)
        {
            HOST_CONTEXT_LOG(DEBUG) << "Checking auth..." << m_ledgerConfig.authCheckStatus();
            auto [result, param] = checkAuth(m_rollbackableStorage, m_blockHeader, message(),
                m_origin, buildLegacyExternalCaller(), m_precompiledManager);
            if (!result)
            {
                // FIXME: build EVMCResult and return
            }
        }

        if (message().kind == EVMC_CREATE || message().kind == EVMC_CREATE2)
        {
            co_return co_await executeCreate();
        }
        else
        {
            co_return co_await executeCall();
        }
    }

    task::Task<EVMCResult> externalCall(const evmc_message& message)
    {
        if (c_fileLogLevel <= LogLevel::TRACE)
        {
            HOST_CONTEXT_LOG(TRACE) << "External call, kind: " << message.kind
                                    << " sender:" << address2HexString(message.sender)
                                    << " recipient:" << address2HexString(message.recipient);
        }
        ++m_seq;

        HostContext hostcontext(innerConstructor, m_rollbackableStorage, m_blockHeader, message,
            m_origin, {}, m_contextID, m_seq, m_precompiledManager, m_ledgerConfig, m_hashImpl,
            interface);

        co_await hostcontext.prepare();
        auto result = co_await hostcontext.execute();
        auto& logs = hostcontext.logs();
        if (result.status_code == EVMC_SUCCESS && !logs.empty())
        {
            m_logs.reserve(m_logs.size() + RANGES::size(logs));
            RANGES::move(logs, std::back_inserter(m_logs));
        }

        co_return result;
    }

    std::vector<protocol::LogEntry>& logs() & { return m_logs; }

private:
    task::Task<std::shared_ptr<Executable>> getExecutable(
        Storage& storage, const evmc_address& address)
    {
        static storage2::memory_storage::MemoryStorage<evmc_address, std::shared_ptr<Executable>,
            storage2::memory_storage::Attribute(
                storage2::memory_storage::MRU | storage2::memory_storage::CONCURRENT),
            std::hash<evmc_address>>
            cachedExecutables;

        auto executable = co_await storage2::readOne(cachedExecutables, address);
        if (executable)
        {
            co_return std::move(*executable);
        }

        Account account(m_rollbackableStorage, address);
        auto codeEntry = co_await ledger::account::code(account);
        if (!codeEntry)
        {
            co_return std::shared_ptr<Executable>{};
        }

        executable.emplace(std::make_shared<Executable>(Executable(std::move(*codeEntry))));
        co_await storage2::writeOne(cachedExecutables, address, *executable);
        co_return std::move(*executable);
    }

    void prepareCreate()
    {
        bytesConstRef createCode(message().input_data, message().input_size);
        m_executable = std::make_shared<Executable>(createCode);
    }

    task::Task<EVMCResult> executeCreate()
    {
        auto savepoint = m_rollbackableStorage.current();
        if (m_ledgerConfig.authCheckStatus() != 0U)
        {
            createAuthTable(m_rollbackableStorage, m_blockHeader, message(), m_origin,
                co_await ledger::account::path(m_myAccount), buildLegacyExternalCaller(),
                m_precompiledManager);
        }

        auto& ref = message();
        co_await ledger::account::create(m_myAccount);
        auto result = m_executable->m_vmInstance.execute(
            interface, this, mode, std::addressof(ref), message().input_data, message().input_size);
        if (result.status_code == 0)
        {
            auto code = bytesConstRef(result.output_data, result.output_size);
            auto codeHash = m_hashImpl.hash(code);
            co_await ledger::account::setCode(
                m_myAccount, code.toBytes(), std::string(m_abi), codeHash);

            result.gas_left -= result.output_size * vmSchedule().createDataGas;
            result.create_address = message().code_address;
        }
        else
        {
            co_await m_rollbackableStorage.rollback(savepoint);
        }

        co_return result;
    }

    task::Task<void> prepareCall()
    {
        if (auto const* precompiled = m_precompiledManager.getPrecompiled(message().code_address))
        {
            m_preparedPrecompiled = precompiled;
            co_return;
        }
    }

    task::Task<EVMCResult> executeCall()
    {
        auto savepoint = m_rollbackableStorage.current();
        std::optional<EVMCResult> result;
        if (m_preparedPrecompiled != nullptr) [[unlikely]]
        {
            result.emplace(transaction_executor::callPrecompiled(*m_preparedPrecompiled,
                m_rollbackableStorage, m_blockHeader, message(), m_origin,
                buildLegacyExternalCaller(), m_precompiledManager));
        }
        else
        {
            if (!m_executable)
            {
                m_executable =
                    co_await getExecutable(m_rollbackableStorage, message().code_address);
            }
            auto& ref = message();
            result.emplace(
                m_executable->m_vmInstance.execute(interface, this, mode, std::addressof(ref),
                    (const uint8_t*)m_executable->m_code->data(), m_executable->m_code->size()));
        }

        if (result->status_code != 0)
        {
            HOST_CONTEXT_LOG(DEBUG)
                << "Execute transaction failed, status: " << result->status_code;
            co_await m_rollbackableStorage.rollback(savepoint);
        }

        co_return std::move(*result);
    }
};

}  // namespace bcos::transaction_executor
