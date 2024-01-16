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
 * @file HostContext.cpp
 * @author: xingqiangbai
 * @date: 2021-05-24
 */

#include "HostContext.h"
#include "../Common.h"
#include "../executive/TransactionExecutive.h"
#include "EVMHostInterface.h"
#include "bcos-framework/bcos-framework/ledger/LedgerTypeDef.h"
#include "bcos-framework/storage/Table.h"
#include "bcos-table/src/StateStorage.h"
#include "evmc/evmc.hpp"
#include <bcos-framework/executor/ExecutionMessage.h>
#include <bcos-framework/protocol/Protocol.h>
#include <bcos-utilities/Common.h>
#include <evmc/evmc.h>
#include <evmc/helpers.h>
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/thread.hpp>
#include <boost/throw_exception.hpp>
#include <algorithm>
#include <exception>
#include <iterator>
#include <limits>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>


using namespace std;
using namespace bcos;
using namespace bcos::executor;
using namespace bcos::storage;
using namespace bcos::protocol;

namespace bcos::executor
{
namespace
{

evmc_bytes32 evm_hash_fn(const uint8_t* data, size_t size)
{
    return toEvmC(HostContext::hashImpl()->hash(bytesConstRef(data, size)));
}
}  // namespace

HostContext::HostContext(CallParameters::UniquePtr callParameters,
    std::shared_ptr<TransactionExecutive> executive, std::string tableName)
  : evmc_host_context(),
    m_callParameters(std::move(callParameters)),
    m_executive(std::move(executive)),
    m_tableName(std::move(tableName))
{
    interface = getHostInterface();
    wasm_interface = getWasmHostInterface();

    hash_fn = evm_hash_fn;
    version = m_executive->blockContext().blockVersion();
    isSMCrypto = false;

    if (hashImpl() && hashImpl()->getHashImplType() == crypto::HashImplType::Sm3Hash)
    {
        isSMCrypto = true;
    }
    metrics = &ethMetrics;
}

std::string HostContext::get(const std::string_view& _key)
{
    auto entry = m_executive->storage().getRow(m_tableName, _key);
    if (entry)
    {
        return std::string(entry->getField(0));
    }
    return {};
}

void HostContext::set(const std::string_view& _key, std::string _value)
{
    auto start = utcTimeUs();
    Entry entry;
    entry.importFields({std::move(_value)});

    m_executive->storage().setRow(m_tableName, _key, std::move(entry));
}


std::string addressBytesStr2String(std::string_view receiveAddressBytes)
{
    std::string strAddress;
    strAddress.reserve(receiveAddressBytes.size() * 2);
    boost::algorithm::hex_lower(
        receiveAddressBytes.begin(), receiveAddressBytes.end(), std::back_inserter(strAddress));

    return strAddress;
}

std::string evmAddress2String(const evmc_address& address)
{
    auto receiveAddressBytes = fromEvmC(address);
    return addressBytesStr2String(receiveAddressBytes);
}

evmc_result HostContext::externalRequest(const evmc_message* _msg)
{
    // Convert evmc_message to CallParameters
    auto request = std::make_unique<CallParameters>(CallParameters::MESSAGE);

    request->origin = origin();
    request->status = 0;
    request->value = fromEvmC(_msg->value);
    const auto& blockContext = m_executive->blockContext();
    if (blockContext.features().get(ledger::Features::Flag::feature_balance) &&
        evmAddress2String(_msg->sender) ==
            std::string(bcos::precompiled::EVM_BALANCE_SENDER_ADDRESS))
    {
        // comes from selfdestruct() getAccountBalance use EVM_BALANCE_SENDER_ADDRESS as msg->sender
        request->senderAddress = std::string(bcos::precompiled::EVM_BALANCE_SENDER_ADDRESS);
    }
    else
    {
        request->senderAddress = myAddress();
    }
    switch (_msg->kind)
    {
    case EVMC_CREATE2:
        request->createSalt = fromEvmC(_msg->create2_salt);
        if (features().get(
                ledger::Features::Flag::bugfix_evm_create2_delegatecall_staticcall_codecopy))
        {
            request->data.assign(_msg->input_data, _msg->input_data + _msg->input_size);
            request->create = true;
        }
        break;
    case EVMC_CALL:
        if (blockContext.isWasm())
        {
            request->receiveAddress.assign((char*)_msg->destination_ptr, _msg->destination_len);
        }
        else
        {
            request->receiveAddress = evmAddress2String(_msg->code_address);
        }

        request->codeAddress = request->receiveAddress;
        request->data.assign(_msg->input_data, _msg->input_data + _msg->input_size);
        break;
    case EVMC_DELEGATECALL:
    case EVMC_CALLCODE:
    {
        if (!blockContext.isWasm())
        {
            if (blockContext.blockVersion() >= (uint32_t)bcos::protocol::BlockVersion::V3_1_VERSION)
            {
                request->delegateCall = true;
                request->codeAddress = evmAddress2String(_msg->code_address);
                request->delegateCallSender = evmAddress2String(_msg->sender);
                request->receiveAddress = codeAddress();
                request->data.assign(_msg->input_data, _msg->input_data + _msg->input_size);
                break;
            }
        }

        // old logic
        evmc_result result;
        result.status_code = evmc_status_code(EVMC_INVALID_INSTRUCTION);
        result.release = nullptr;  // no output to release
        result.gas_left = 0;
        result.gas_refund = 0;

        // Bugfix:
        // To compat with lower version,
        // we must set output_size larger than 0 to trigger OUT_OF_GAS
        static auto response = std::make_unique<CallParameters>(CallParameters::MESSAGE);
        response->data = bytes(130 * 1024 * 1024);
        result.output_size = response->data.size();
        result.output_data = response->data.data();
        EXECUTOR_LOG(WARNING) << LOG_DESC(
                                     "delegatecall/callcode is unsupported in your compatibility "
                                     "version(3.0.0), please update it to 3.1.0(or after) using "
                                     "console. Otherwise it may "
                                     "lead to unpredictable behavior.")
                              << LOG_KV("version", blockContext.blockVersion());

        return result;
    }
    case EVMC_CREATE:
        request->data.assign(_msg->input_data, _msg->input_data + _msg->input_size);
        request->create = true;
        break;
    }
    if (versionCompareTo(blockContext.blockVersion(), BlockVersion::V3_1_VERSION) >= 0)
    {
        request->logEntries = std::move(m_callParameters->logEntries);
    }
    request->gas = _msg->gas;
    // if (built in precompiled) then execute locally

    if (m_executive->isStaticPrecompiled(request->receiveAddress))
    {
        return callBuiltInPrecompiled(request, false);
    }
    if (!blockContext.isWasm() && m_executive->isEthereumPrecompiled(request->receiveAddress))
    {
        return callBuiltInPrecompiled(request, true);
    }

    request->staticCall = m_callParameters->staticCall;

    if (features().get(ledger::Features::Flag::bugfix_evm_create2_delegatecall_staticcall_codecopy))
    {
        // EVM STATICCALL opcode support
        if (_msg->flags & EVMC_STATIC)
        {
            request->staticCall = true;
        }
    }

    auto response = m_executive->externalCall(std::move(request));

    // Convert CallParameters to evmc_resultx
    evmc_result result{.status_code = toEVMStatus(response, blockContext),
        .gas_left = response->gas,
        .gas_refund = 0,
        .output_data = response->data.data(),
        .output_size = response->data.size(),
        .release = nullptr,  // TODO: check if the response data need to release
        .create_address = toEvmC(boost::algorithm::unhex(response->newEVMContractAddress)),
        .padding = {}};

    if (features().get(ledger::Features::Flag::bugfix_event_log_order))
    {
        // put event log by stack(dfs) order
        m_callParameters->logEntries = std::move(response->logEntries);
    }
    // Put response to store in order to avoid data lost
    m_responseStore.emplace_back(std::move(response));

    return result;
}

evmc_status_code HostContext::toEVMStatus(std::unique_ptr<CallParameters> const& response,
    const bcos::executor::BlockContext& blockContext)
{
    if (blockContext.blockVersion() >= (uint32_t)(bcos::protocol::BlockVersion::V3_1_VERSION))
    {
        return evmc_status_code(response->evmStatus);
    }

    return evmc_status_code(response->status);
}

evmc_result HostContext::callBuiltInPrecompiled(
    std::unique_ptr<CallParameters> const& _request, bool _isEvmPrecompiled)
{
    auto callResults = std::make_unique<CallParameters>(CallParameters::FINISHED);
    evmc_result preResult{};
    int32_t resultCode = 0;
    bytes resultData;

    if (_isEvmPrecompiled)
    {
        auto gasUsed =
            m_executive->costOfPrecompiled(_request->receiveAddress, ref(_request->data));
        /// NOTE: this assignment is wrong, will cause out of gas, should not use evm precompiled
        /// before 3.1.0
        callResults->gas = gasUsed;
        if (versionCompareTo(version, BlockVersion::V3_1_VERSION) >= 0)
        {
            callResults->gas = _request->gas - gasUsed;
        }
        try
        {
            auto [success, output] = m_executive->executeOriginPrecompiled(
                _request->receiveAddress, ref(_request->data));
            resultCode =
                (int32_t)(success ? TransactionStatus::None : TransactionStatus::RevertInstruction);
            resultData.swap(output);
        }
        catch (...)
        {
            resultCode = (int32_t)TransactionStatus::RevertInstruction;
        }
    }
    else
    {
        try
        {
            auto precompiledCallParams =
                std::make_shared<precompiled::PrecompiledExecResult>(*_request);
            precompiledCallParams = m_executive->execPrecompiled(precompiledCallParams);
            callResults->gas = precompiledCallParams->m_gasLeft;
            resultCode = (int32_t)TransactionStatus::None;
            resultData = std::move(precompiledCallParams->m_execResult);
        }
        catch (protocol::PrecompiledError& e)
        {
            resultCode = (int32_t)TransactionStatus::PrecompiledError;
        }
        catch (std::exception& e)
        {
            resultCode = (int32_t)TransactionStatus::Unknown;
        }
    }

    if (resultCode != (int32_t)TransactionStatus::None)
    {
        callResults->type = CallParameters::REVERT;
        callResults->status = resultCode;
        preResult.status_code = EVMC_INTERNAL_ERROR;
        preResult.gas_left = 0;
        m_responseStore.emplace_back(std::move(callResults));
        return preResult;
    }

    preResult.gas_left = callResults->gas;
    preResult.gas_refund = 0;
    if (preResult.gas_left < 0)
    {
        callResults->type = CallParameters::REVERT;
        callResults->status = (int32_t)TransactionStatus::OutOfGas;
        preResult.status_code = EVMC_OUT_OF_GAS;
        preResult.gas_left = 0;
        return preResult;
    }
    callResults->status = (int32_t)TransactionStatus::None;
    callResults->data.swap(resultData);
    preResult.output_size = callResults->data.size();
    preResult.output_data = callResults->data.data();
    preResult.release = nullptr;
    m_responseStore.emplace_back(std::move(callResults));
    return preResult;
}

bool HostContext::setCode(bytes code)
{
    m_executive->setContractTableChanged();

    // set code will cause exception when exec revert
    // new logic
    if (blockVersion() >= uint32_t(bcos::protocol::BlockVersion::V3_1_VERSION))
    {
        auto contractTable = m_executive->storage().openTable(m_tableName);
        // set code hash in contract table
        auto codeHash = hashImpl()->hash(code);
        if (contractTable)
        {
            Entry codeHashEntry;
            codeHashEntry.importFields({codeHash.asBytes()});

            auto codeEntry = m_executive->storage().getRow(
                bcos::ledger::SYS_CODE_BINARY, codeHashEntry.getField(0));

            if (!codeEntry)
            {
                codeEntry = std::make_optional<Entry>();
                codeEntry->importFields({std::move(code)});

                // set code in code binary table
                m_executive->storage().setRow(bcos::ledger::SYS_CODE_BINARY,
                    codeHashEntry.getField(0), std::move(codeEntry.value()));
            }

            // dry code hash in account table
            m_executive->storage().setRow(m_tableName, ACCOUNT_CODE_HASH, std::move(codeHashEntry));
            return true;
        }
        return false;
    }
    // old logic
    auto contractTable = m_executive->storage().openTable(m_tableName);
    if (contractTable)
    {
        Entry codeHashEntry;
        auto codeHash = hashImpl()->hash(code);
        codeHashEntry.importFields({codeHash.asBytes()});
        m_executive->storage().setRow(m_tableName, ACCOUNT_CODE_HASH, std::move(codeHashEntry));

        Entry codeEntry;
        codeEntry.importFields({std::move(code)});
        m_executive->storage().setRow(m_tableName, ACCOUNT_CODE, std::move(codeEntry));
        return true;
    }
    return false;
}

void HostContext::setCodeAndAbi(bytes code, string abi)
{
    EXECUTOR_LOG(TRACE) << LOG_DESC("save code and abi") << LOG_KV("tableName", m_tableName)
                        << LOG_KV("codeSize", code.size()) << LOG_KV("abiSize", abi.size());
    if (setCode(std::move(code)))
    {
        // new logic
        if (blockVersion() >= uint32_t(bcos::protocol::BlockVersion::V3_1_VERSION))
        {
            // set abi in abi table
            auto codeEntry = m_executive->storage().getRow(m_tableName, ACCOUNT_CODE_HASH);
            auto codeHash = codeEntry->getField(0);

            if (c_fileLogLevel <= TRACE) [[unlikely]]
            {
                EXECUTOR_LOG(TRACE) << LOG_DESC("set abi") << LOG_KV("codeHash", toHex(codeHash))
                                    << LOG_KV("abiSize", abi.size());
            }

            auto abiEntry = m_executive->storage().getRow(bcos::ledger::SYS_CONTRACT_ABI, codeHash);

            if (!abiEntry)
            {
                abiEntry = std::make_optional<Entry>();
                abiEntry->importFields({std::move(abi)});

                m_executive->storage().setRow(
                    bcos::ledger::SYS_CONTRACT_ABI, codeHash, std::move(abiEntry.value()));
            }

            return;
        }

        // old logic
        Entry abiEntry;
        abiEntry.importFields({std::move(abi)});
        m_executive->storage().setRow(m_tableName, ACCOUNT_ABI, abiEntry);
    }
}

bcos::bytes HostContext::externalCodeRequest(const std::string_view& address)
{
    auto request = std::make_unique<CallParameters>(CallParameters::MESSAGE);
    request->gas = gas();
    request->senderAddress = myAddress();
    request->receiveAddress = myAddress();
    request->data = bcos::protocol::GET_CODE_INPUT_BYTES;
    request->origin = origin();
    request->status = 0;
    request->delegateCall = false;
    request->codeAddress = addressBytesStr2String(address);
    request->staticCall = staticCall();
    auto response = m_executive->externalCall(std::move(request));
    return std::move(response->data);
}

size_t HostContext::codeSizeAt(const std::string_view& address)
{
    if (m_executive->blockContext().blockVersion() >=
        (uint32_t)bcos::protocol::BlockVersion::V3_1_VERSION)
    {
        /*
         Note:
            evm precompiled(0x1 ~ 0x9): return 0 (Is the same as eth)
            FISCO BCOS precompiled: return 1

            Because evm precompiled is call by build-in opcode, no need to get code size before
         called, but FISCO BCOS precompiled is call like contract, so it need to get code size.
         */

        if (m_executive->isPrecompiled(addressBytesStr2String(address)))
        {
            // Only FISCO BCOS precompile: constant precompiled or build-in precompiled
            // evm precompiled address will go down to externalCodeRequest() and get empty code
            return 1;
        }
        auto code = externalCodeRequest(address);
        return code.size();  // OPCODE num is bytes.size
    }
    return 1;
}

h256 HostContext::codeHashAt(const std::string_view& address)
{
    if (m_executive->blockContext().blockVersion() >=
        (uint32_t)bcos::protocol::BlockVersion::V3_1_VERSION)
    {
        // precompiled return 0 hash;
        if (m_executive->isPrecompiled(addressBytesStr2String(address)))
        {
            return {0};
        }
        auto code = externalCodeRequest(address);

        if (code.empty())
        {
            if (features().get(ledger::Features::Flag::bugfix_precompiled_codehash))
            {
                return {0};
            }
        }

        auto hash = hashImpl()->hash(code).asBytes();
        return h256(hash);
    }
    return {0};
}

VMSchedule const& HostContext::vmSchedule() const
{
    return m_executive->vmSchedule();
}

evmc_bytes32 HostContext::store(const evmc_bytes32* key)
{
    evmc_bytes32 result;
    auto keyView = std::string_view((char*)key->bytes, sizeof(key->bytes));

    auto entry = m_executive->storage().getRow(m_tableName, keyView);
    if (entry)
    {
        auto field = entry->getField(0);
        std::uninitialized_copy_n(field.data(), sizeof(result), result.bytes);
    }
    else
    {
        std::uninitialized_fill_n(result.bytes, sizeof(result), 0);
    }
    return result;
}

void HostContext::setStore(const evmc_bytes32* key, const evmc_bytes32* value)
{
    auto keyView = std::string_view((char*)key->bytes, sizeof(key->bytes));
    bytes valueBytes(value->bytes, value->bytes + sizeof(value->bytes));

    Entry entry;
    entry.importFields({std::move(valueBytes)});
    m_executive->storage().setRow(m_tableName, keyView, std::move(entry));
}

void HostContext::log(h256s&& _topics, bytesConstRef _data)
{
    // if (m_isWasm || myAddress().empty())
    // {
    //     m_sub.logs->push_back(
    //         protocol::LogEntry(bytes(myAddress().data(), myAddress().data() +
    //         myAddress().size()),
    //             std::move(_topics), _data.toBytes()));
    // }
    // else
    // {
    //     // convert solidity address to hex string
    //     auto hexAddress = *toHexString(myAddress());
    //     boost::algorithm::to_lower(hexAddress);  // this is in case of toHexString be modified
    //     toChecksumAddress(hexAddress, hashImpl()->hash(hexAddress).hex());
    //     m_sub.logs->push_back(
    //         protocol::LogEntry(asBytes(hexAddress), std::move(_topics), _data.toBytes()));
    // }
    m_callParameters->logEntries.emplace_back(
        bytes(myAddress().data(), myAddress().data() + myAddress().size()), std::move(_topics),
        _data.toBytes());
}

h256 HostContext::blockHash(int64_t _number) const
{
    if (m_executive->blockContext().blockVersion() >=
        (uint32_t)bcos::protocol::BlockVersion::V3_1_VERSION)
    {
        if (_number >= blockNumber() || _number < 0)
        {
            return h256("");
        }
        else
        {
            return m_executive->blockContext().blockHash(_number);
        }
    }
    else
    {
        return m_executive->blockContext().hash();
    }
}

int64_t HostContext::blockNumber() const
{
    return m_executive->blockContext().number();
}

uint32_t HostContext::blockVersion() const
{
    return m_executive->blockContext().blockVersion();
}

uint64_t HostContext::timestamp() const
{
    return m_executive->blockContext().timestamp();
}

std::string_view HostContext::myAddress() const
{
    return m_executive->contractAddress();
}

std::optional<storage::Entry> HostContext::code()
{
    if (blockVersion() >= uint32_t(bcos::protocol::BlockVersion::V3_1_VERSION))
    {
        auto codehash = codeHash();

        auto key = std::string_view((char*)codehash.data(), codehash.size());
        auto entry = m_executive->storage().getRow(bcos::ledger::SYS_CODE_BINARY, key);
        if (entry && !entry->get().empty())
        {
            return entry;
        }
    }

    return m_executive->storage().getRow(m_tableName, ACCOUNT_CODE);
}

crypto::HashType HostContext::codeHash()
{
    auto entry = m_executive->storage().getRow(m_tableName, ACCOUNT_CODE_HASH);
    if (entry)
    {
        auto code = entry->getField(0);
        return crypto::HashType(code, crypto::HashType::StringDataType::FromBinary);
    }

    return {};
}

bool HostContext::isWasm()
{
    return m_executive->isWasm();
}

}  // namespace bcos::executor