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
 * @brief factory of vm
 * @file VMFactory.h
 * @author: ancelmo
 * @date: 2022-12-15
 */

#pragma once
#include "VMInstance.h"
#include "bcos-framework/storage2/MemoryStorage.h"
#include <evmone/evmone.h>
#include <boost/throw_exception.hpp>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace bcos::transaction_executor
{
enum class VMKind
{
    evmone,
};

// clang-format off
struct UnknownVMError : public bcos::Error {};
// clang-format on

class VMFactory
{
private:
    storage2::memory_storage::MemoryStorage<crypto::HashType,
        std::shared_ptr<evmone::baseline::CodeAnalysis const>,
        storage2::memory_storage::Attribute(
            storage2::memory_storage::CONCURRENT | storage2::memory_storage::MRU),
        std::hash<crypto::HashType>>
        m_evmoneCodeAnalysisCache;

public:
    /// Creates a VM instance of the global kind.
    static VMInstance create() { return create(VMKind::evmone); }

    /// Creates a VM instance of the kind provided.
    static VMInstance create(VMKind kind)
    {
        switch (kind)
        {
        case VMKind::evmone:
            return VMInstance{evmc_create_evmone()};
        default:
            BOOST_THROW_EXCEPTION(UnknownVMError{});
        }
    }

    task::Task<VMInstance> create(
        VMKind kind, const bcos::h256& codeHash, std::string_view code, evmc_revision mode)
    {
        switch (kind)
        {
        case VMKind::evmone:
        {
            auto codeAnalysis = co_await storage2::readOne(m_evmoneCodeAnalysisCache, codeHash);

            if (!codeAnalysis)
            {
                codeAnalysis.emplace(
                    std::make_shared<evmone::baseline::CodeAnalysis>(evmone::baseline::analyze(
                        mode, evmone::bytes_view((const uint8_t*)code.data(), code.size()))));
                co_await storage2::writeOne(m_evmoneCodeAnalysisCache, codeHash, *codeAnalysis);
            }

            co_return VMInstance{std::move(*codeAnalysis)};
        }
        default:
            BOOST_THROW_EXCEPTION(UnknownVMError{});
        }
    }
};
}  // namespace bcos::transaction_executor