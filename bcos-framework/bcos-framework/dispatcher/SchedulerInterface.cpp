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
 * @brief default Task-based implementations for SchedulerInterface
 * @file SchedulerInterface.cpp
 * @author: more
 * @date 2026-05-31
 */

#include "SchedulerInterface.h"
#include <boost/throw_exception.hpp>

using namespace bcos::scheduler;

// Default impl: bridges to 3-arg callback virtual method via Awaitable

bcos::task::Task<std::tuple<bcos::protocol::BlockHeader::Ptr, bool>>
SchedulerInterface::executeBlock(bcos::protocol::Block::Ptr block, bool verify)
{
    struct Awaitable
    {
        SchedulerInterface* self;
        bcos::protocol::Block::Ptr block;
        bool verify;
        bcos::Error::Ptr error = nullptr;
        bcos::protocol::BlockHeader::Ptr header = nullptr;
        bool sysBlock = false;

        constexpr bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            self->executeBlock(std::move(block), verify,
                [this, handle](bcos::Error::Ptr _error, bcos::protocol::BlockHeader::Ptr _header,
                    bool _sysBlock) {
                    if (_error)
                        error = std::move(_error);
                    else
                    {
                        header = std::move(_header);
                        sysBlock = _sysBlock;
                    }
                    handle.resume();
                });
        }
        auto await_resume()
        {
            if (error)
                BOOST_THROW_EXCEPTION(*error);
            return std::make_tuple(std::move(header), sysBlock);
        }
    };
    Awaitable a{this, std::move(block), verify};
    co_return co_await a;
}

bcos::task::Task<bcos::ledger::LedgerConfig::Ptr> SchedulerInterface::commitBlock(
    bcos::protocol::BlockHeader::Ptr header)
{
    struct Awaitable
    {
        SchedulerInterface* self;
        bcos::protocol::BlockHeader::Ptr header = nullptr;
        bcos::Error::Ptr error = nullptr;
        bcos::ledger::LedgerConfig::Ptr config = nullptr;

        constexpr bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            self->commitBlock(std::move(header),
                [this, handle](bcos::Error::Ptr _error, bcos::ledger::LedgerConfig::Ptr _config) {
                    if (_error)
                        error = std::move(_error);
                    else
                        config = std::move(_config);
                    handle.resume();
                });
        }
        auto await_resume()
        {
            if (error)
                BOOST_THROW_EXCEPTION(*error);
            return std::move(config);
        }
    };
    Awaitable a{this, std::move(header)};
    co_return co_await a;
}

bcos::task::Task<bcos::protocol::TransactionReceipt::Ptr> SchedulerInterface::call(
    bcos::protocol::Transaction::Ptr tx)
{
    struct Awaitable
    {
        SchedulerInterface* self;
        bcos::protocol::Transaction::Ptr tx;
        bcos::Error::Ptr error = nullptr;
        bcos::protocol::TransactionReceipt::Ptr receipt = nullptr;

        constexpr bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            self->call(std::move(tx), [this, handle](bcos::Error::Ptr _error,
                                          bcos::protocol::TransactionReceipt::Ptr _receipt) {
                if (_error)
                    error = std::move(_error);
                else
                    receipt = std::move(_receipt);
                handle.resume();
            });
        }
        auto await_resume()
        {
            if (error)
                BOOST_THROW_EXCEPTION(*error);
            return std::move(receipt);
        }
    };
    Awaitable a{this, std::move(tx)};
    co_return co_await a;
}

bcos::task::Task<bcos::bytes> SchedulerInterface::getCode(std::string_view contract)
{
    struct Awaitable
    {
        SchedulerInterface* self;
        std::string_view contract;
        bcos::Error::Ptr error = nullptr;
        bcos::bytes code = {};

        constexpr bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            self->getCode(contract, [this, handle](bcos::Error::Ptr _error, bcos::bytes _code) {
                if (_error)
                    error = std::move(_error);
                else
                    code = std::move(_code);
                handle.resume();
            });
        }
        auto await_resume()
        {
            if (error)
                BOOST_THROW_EXCEPTION(*error);
            return std::move(code);
        }
    };
    Awaitable a{this, contract};
    co_return co_await a;
}

bcos::task::Task<std::string> SchedulerInterface::getABI(std::string_view contract)
{
    struct Awaitable
    {
        SchedulerInterface* self;
        std::string_view contract;
        bcos::Error::Ptr error = nullptr;
        std::string abi = {};

        constexpr bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            self->getABI(contract, [this, handle](bcos::Error::Ptr _error, std::string _abi) {
                if (_error)
                    error = std::move(_error);
                else
                    abi = std::move(_abi);
                handle.resume();
            });
        }
        auto await_resume()
        {
            if (error)
                BOOST_THROW_EXCEPTION(*error);
            return std::move(abi);
        }
    };
    Awaitable a{this, contract};
    co_return co_await a;
}

bcos::task::Task<bcos::protocol::Session::ConstPtr> SchedulerInterface::status()
{
    struct Awaitable
    {
        SchedulerInterface* self;
        bcos::Error::Ptr error = nullptr;
        bcos::protocol::Session::ConstPtr session = nullptr;

        constexpr bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            self->status([this, handle](
                             bcos::Error::Ptr _error, bcos::protocol::Session::ConstPtr _session) {
                if (_error)
                    error = std::move(_error);
                else
                    session = std::move(_session);
                handle.resume();
            });
        }
        auto await_resume()
        {
            if (error)
                BOOST_THROW_EXCEPTION(*error);
            return std::move(session);
        }
    };
    Awaitable a{this};
    co_return co_await a;
}

bcos::task::Task<void> SchedulerInterface::reset()
{
    struct Awaitable
    {
        SchedulerInterface* self;
        bcos::Error::Ptr error = nullptr;

        constexpr bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            self->reset([this, handle](bcos::Error::Ptr _error) {
                if (_error)
                    error = std::move(_error);
                handle.resume();
            });
        }
        void await_resume()
        {
            if (error)
                BOOST_THROW_EXCEPTION(*error);
        }
    };
    Awaitable a{this};
    co_await a;
}

bcos::task::Task<void> SchedulerInterface::preExecuteBlock(
    bcos::protocol::Block::Ptr block, bool verify)
{
    struct Awaitable
    {
        SchedulerInterface* self;
        bcos::protocol::Block::Ptr block;
        bool verify;
        bcos::Error::Ptr error = nullptr;

        constexpr bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            self->preExecuteBlock(
                std::move(block), verify, [this, handle](bcos::Error::Ptr _error) {
                    if (_error)
                        error = std::move(_error);
                    handle.resume();
                });
        }
        void await_resume()
        {
            if (error)
                BOOST_THROW_EXCEPTION(*error);
        }
    };
    Awaitable a{this, std::move(block), verify};
    co_await a;
}
