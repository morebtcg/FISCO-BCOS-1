/**
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
 * @brief server for Scheduler
 * @file SchedulerServiceServer.cpp
 * @author: yujiechen
 * @date 2021-10-18
 */
#include "SchedulerServiceServer.h"
#include "bcos-tars-protocol/Common.h"
#include "bcos-tars-protocol/protocol/BlockHeaderImpl.h"
#include <bcos-tars-protocol/ErrorConverter.h>
#include <bcos-tars-protocol/client/ExecutorServiceClient.h>
#include <bcos-tars-protocol/protocol/BlockImpl.h>
#include <bcos-tars-protocol/protocol/TransactionImpl.h>
#include <bcos-tars-protocol/protocol/TransactionReceiptImpl.h>
#include <bcos-task/Wait.h>

using namespace tars;
using namespace bcostars;

bcostars::Error SchedulerServiceServer::call(
    const bcostars::Transaction& _tx, bcostars::TransactionReceipt&, tars::TarsCurrentPtr current)
{
    current->setResponse(false);
    auto bcosTransaction = std::make_shared<bcostars::protocol::TransactionImpl>(
        [m_tx = _tx]() mutable { return &m_tx; });
    bcos::task::wait([scheduler = m_scheduler, bcosTransaction = std::move(bcosTransaction),
                         current]() -> bcos::task::Task<void> {
        try
        {
            auto receipt = co_await scheduler->call(std::move(bcosTransaction));
            bcostars::TransactionReceipt tarsReceipt;
            if (receipt)
            {
                tarsReceipt =
                    std::dynamic_pointer_cast<bcostars::protocol::TransactionReceiptImpl>(receipt)
                        ->inner();
            }
            async_response_call(current, bcostars::Error(), tarsReceipt);
        }
        catch (bcos::Error& e)
        {
            async_response_call(current, toTarsError(std::make_shared<bcos::Error>(e)),
                bcostars::TransactionReceipt());
        }
    }());
    return bcostars::Error();
}

bcostars::Error SchedulerServiceServer::getCode(
    const std::string& contract, std::vector<tars::Char>& code, tars::TarsCurrentPtr current)
{
    current->setResponse(false);
    bcos::task::wait([scheduler = m_scheduler, contract, current]() -> bcos::task::Task<void> {
        try
        {
            auto code = co_await scheduler->getCode(contract);
            std::vector<tars::Char> outCode(code.begin(), code.end());
            async_response_getCode(current, bcostars::Error(), outCode);
        }
        catch (bcos::Error& e)
        {
            async_response_getCode(current, toTarsError(std::make_shared<bcos::Error>(e)), {});
        }
    }());

    return bcostars::Error();
}

bcostars::Error SchedulerServiceServer::getABI(
    const std::string& contract, std::string& abi, tars::TarsCurrentPtr current)
{
    current->setResponse(false);
    bcos::task::wait([scheduler = m_scheduler, contract, current]() -> bcos::task::Task<void> {
        try
        {
            auto abi = co_await scheduler->getABI(contract);
            async_response_getABI(current, bcostars::Error(), abi);
        }
        catch (bcos::Error& e)
        {
            async_response_getABI(current, toTarsError(std::make_shared<bcos::Error>(e)), {});
        }
    }());

    return bcostars::Error();
}

bcostars::Error SchedulerServiceServer::executeBlock(bcostars::Block const& _block,
    tars::Bool _verify, bcostars::BlockHeader&, tars::Bool&, tars::TarsCurrentPtr _current)
{
    _current->setResponse(false);
    auto bcosBlock = std::make_shared<bcostars::protocol::BlockImpl>(_block);
    bcos::task::wait([scheduler = m_scheduler, bcosBlock = std::move(bcosBlock), _verify,
                         _current]() -> bcos::task::Task<void> {
        try
        {
            auto [header, sysBlock] =
                co_await scheduler->executeBlock(std::move(bcosBlock), _verify);
            auto headerImpl =
                std::dynamic_pointer_cast<bcostars::protocol::BlockHeaderImpl>(header);
            async_response_executeBlock(_current, bcostars::Error(), headerImpl->inner(), sysBlock);
        }
        catch (bcos::Error& e)
        {
            async_response_executeBlock(
                _current, toTarsError(std::make_shared<bcos::Error>(e)), {}, false);
        }
    }());
    return bcostars::Error();
}


bcostars::Error SchedulerServiceServer::commitBlock(
    bcostars::BlockHeader const& _header, bcostars::LedgerConfig&, tars::TarsCurrentPtr _current)
{
    _current->setResponse(false);
    auto bcosHeader = std::make_shared<bcostars::protocol::BlockHeaderImpl>(
        [m_header = _header]() mutable { return &m_header; });
    bcos::task::wait([scheduler = m_scheduler, bcosHeader = std::move(bcosHeader),
                         _current]() -> bcos::task::Task<void> {
        try
        {
            auto ledgerConfig = co_await scheduler->commitBlock(std::move(bcosHeader));
            async_response_commitBlock(
                _current, bcostars::Error(), toTarsLedgerConfig(ledgerConfig));
        }
        catch (bcos::Error& e)
        {
            async_response_commitBlock(_current, toTarsError(std::make_shared<bcos::Error>(e)), {});
        }
    }());
    return bcostars::Error();
}

bcostars::Error SchedulerServiceServer::preExecuteBlock(
    const bcostars::Block& _block, tars::Bool _verify, tars::TarsCurrentPtr _current)
{
    _current->setResponse(false);
    auto bcosBlock = std::make_shared<bcostars::protocol::BlockImpl>(_block);
    bcos::task::wait([scheduler = m_scheduler, bcosBlock = std::move(bcosBlock), _verify,
                         _current]() -> bcos::task::Task<void> {
        try
        {
            co_await scheduler->preExecuteBlock(std::move(bcosBlock), _verify);
            async_response_preExecuteBlock(_current, bcostars::Error());
        }
        catch (bcos::Error& e)
        {
            async_response_preExecuteBlock(_current, toTarsError(std::make_shared<bcos::Error>(e)));
        }
    }());
    return bcostars::Error();
}
