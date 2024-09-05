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
 * @brief the information of the consensus node
 * @file ConsensusNodeInterface.h
 * @author: yujiechen
 * @date 2021-04-09
 */
#pragma once
#include "bcos-utilities/ThreeWay4Apple.h"
#include <bcos-crypto/interfaces/crypto/KeyInterface.h>
#include <bcos-framework/Common.h>
#include <compare>

namespace bcos::consensus
{

constexpr static uint64_t defaultVoteWeight = 100;
constexpr static uint64_t defaultTermWeight = 1;

class ConsensusNodeInterface
{
public:
    enum Type
    {
        SEALER = 0,
        CANDIDATE_SEALER,
        OBSERVER
    };

    using Ptr = std::shared_ptr<ConsensusNodeInterface>;
    ConsensusNodeInterface() = default;
    ConsensusNodeInterface(const ConsensusNodeInterface&) = default;
    ConsensusNodeInterface(ConsensusNodeInterface&&) noexcept = default;
    ConsensusNodeInterface& operator=(const ConsensusNodeInterface&) = default;
    ConsensusNodeInterface& operator=(ConsensusNodeInterface&&) noexcept = default;
    virtual ~ConsensusNodeInterface() noexcept = default;

    // the nodeID of the consensus node
    virtual bcos::crypto::PublicPtr nodeID() const = 0;
    virtual Type type() const = 0;
    virtual uint64_t voteWeight() const { return defaultVoteWeight; }
    virtual uint64_t termWeight() const { return defaultTermWeight; }
};

class ConsensusNodeFactory
{
public:
    ConsensusNodeFactory(const ConsensusNodeFactory&) = default;
    ConsensusNodeFactory(ConsensusNodeFactory&&) noexcept = default;
    ConsensusNodeFactory& operator=(const ConsensusNodeFactory&) = default;
    ConsensusNodeFactory& operator=(ConsensusNodeFactory&&) noexcept = default;
    virtual ~ConsensusNodeFactory() noexcept = default;
    virtual ConsensusNodeInterface::Ptr create() = 0;
};

inline std::strong_ordering operator<=>(
    ConsensusNodeInterface const& lhs, ConsensusNodeInterface const& rhs)
{
    if (auto cmp = lhs.nodeID()->data() <=> rhs.nodeID()->data(); !std::is_eq(cmp))
    {
        return cmp;
    }
    if (auto cmp = lhs.type() <=> rhs.type(); !std::is_eq(cmp))
    {
        return cmp;
    }
    if (auto cmp = lhs.voteWeight() <=> rhs.voteWeight(); !std::is_eq(cmp))
    {
        return cmp;
    }
    return lhs.termWeight() <=> rhs.termWeight();
}
inline std::strong_ordering operator<=>(
    ConsensusNodeInterface::Ptr const& lhs, ConsensusNodeInterface::Ptr const& rhs)
{
    return *lhs <=> *rhs;
}

using ConsensusNodeList = std::vector<ConsensusNodeInterface::Ptr>;
using ConsensusNodeListPtr = std::shared_ptr<ConsensusNodeList>;
using ConsensusNodeSet = std::set<ConsensusNodeInterface::Ptr>;
using ConsensusNodeSetPtr = std::shared_ptr<ConsensusNodeSet>;
inline std::string decsConsensusNodeList(ConsensusNodeList const& _nodeList)
{
    std::ostringstream stringstream;
    for (const auto& node : _nodeList)
    {
        stringstream << LOG_KV(node->nodeID()->shortHex(), std::to_string(node->voteWeight()));
    }
    return stringstream.str();
}
}  // namespace bcos::consensus
