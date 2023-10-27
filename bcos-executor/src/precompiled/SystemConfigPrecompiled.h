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
 * @file SystemConfigPrecompiled.h
 * @author: kyonRay
 * @date 2021-05-26
 */

#pragma once
#include "../vm/Precompiled.h"
#include "bcos-executor/src/precompiled/common/Common.h"
#include "bcos-framework/protocol/ProtocolTypeDef.h"
#include <bcos-framework/ledger/Features.h>
#include <bcos-framework/ledger/LedgerTypeDef.h>
#include <set>

namespace bcos::precompiled
{
class SystemConfigPrecompiled : public bcos::precompiled::Precompiled
{
public:
    using Ptr = std::shared_ptr<SystemConfigPrecompiled>;

    SystemConfigPrecompiled(crypto::Hash::Ptr hashImpl);
    SystemConfigPrecompiled(const SystemConfigPrecompiled&) = default;
    SystemConfigPrecompiled& operator=(const SystemConfigPrecompiled&) = delete;
    SystemConfigPrecompiled(SystemConfigPrecompiled&&) = default;
    SystemConfigPrecompiled& operator=(SystemConfigPrecompiled&&) = delete;
    ~SystemConfigPrecompiled() override = default;

    std::shared_ptr<PrecompiledExecResult> call(
        std::shared_ptr<executor::TransactionExecutive> _executive,
        PrecompiledExecResult::Ptr _callParameters) override;
    static std::pair<std::string, protocol::BlockNumber> getSysConfigByKey(
        const std::shared_ptr<executor::TransactionExecutive>& _executive, const std::string& _key);

private:
    int64_t validate(
        std::string_view key, std::string_view value, protocol::BlockVersion blockVersion);
    static bool shouldUpgradeChain(std::string_view key, protocol::BlockVersion fromVersion,
        protocol::BlockVersion toVersion) noexcept;
    static void upgradeChain(const std::shared_ptr<executor::TransactionExecutive>& _executive,
        const PrecompiledExecResult::Ptr& _callParameters, CodecWrapper const& codec,
        protocol::BlockVersion toVersion);

    std::map<std::string, std::function<int64_t(std::string, protocol::BlockVersion)>>
        m_valueConverter;
    std::unordered_map<std::string, std::function<void(int64_t, protocol::BlockVersion)>>
        m_sysValueCmp;
};

}  // namespace bcos::precompiled
