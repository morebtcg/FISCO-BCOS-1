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
 * @brief
 * @file LedgerConfig.h
 * @author: yujiechen
 * @date 2021-05-06
 */
#pragma once
#include "../consensus/ConsensusNode.h"
#include "../protocol/ProtocolTypeDef.h"
#include "Features.h"
#include "SystemConfigs.h"
#include <evmc/evmc.hpp>
#include <string>
#include <utility>

namespace bcos::ledger
{

// constexpr static uint64_t DEFAULT_GAS_LIMIT = 3000000000;
// constexpr static std::uint64_t DEFAULT_EPOCH_SEALER_NUM = 4;
// constexpr static std::uint64_t DEFAULT_EPOCH_BLOCK_NUM = 1000;
// constexpr static std::uint64_t DEFAULT_INTERNAL_NOTIFY_FLAG = 0;

class LedgerConfig
{
public:
    using Ptr = std::shared_ptr<LedgerConfig>;
    LedgerConfig() = default;
    LedgerConfig(const LedgerConfig&) = default;
    LedgerConfig(LedgerConfig&&) noexcept = default;
    LedgerConfig& operator=(const LedgerConfig&) = default;
    LedgerConfig& operator=(LedgerConfig&&) noexcept = default;
    virtual ~LedgerConfig() noexcept = default;

    virtual void setConsensusNodeList(bcos::consensus::ConsensusNodeList _consensusNodeList)
    {
        m_consensusNodeList = std::move(_consensusNodeList);
    }
    virtual void setObserverNodeList(bcos::consensus::ConsensusNodeList _observerNodeList)
    {
        m_observerNodeList = std::move(_observerNodeList);
    }
    virtual void setCandidateSealerNodeList(
        bcos::consensus::ConsensusNodeList candidateSealerNodeList)
    {
        m_candidateSealerNodeList = std::move(candidateSealerNodeList);
    }
    virtual void setHash(bcos::crypto::HashType const& _hash) { m_hash = _hash; }
    virtual void setBlockNumber(bcos::protocol::BlockNumber _blockNumber)
    {
        m_blockNumber = _blockNumber;
    }

    virtual void setBlockTxCountLimit(uint64_t _blockTxCountLimit)
    {
        m_systemConfigs.set(SystemConfig::tx_count_limit, std::to_string(_blockTxCountLimit));
    }

    virtual bcos::consensus::ConsensusNodeList const& consensusNodeList() const
    {
        return m_consensusNodeList;
    }

    virtual bcos::consensus::ConsensusNodeList& mutableConsensusNodeList()
    {
        return m_consensusNodeList;
    }

    virtual bcos::consensus::ConsensusNodeList const& observerNodeList() const
    {
        return m_observerNodeList;
    }
    virtual bcos::consensus::ConsensusNodeList const& candidateSealerNodeList() const
    {
        return m_candidateSealerNodeList;
    }
    bcos::crypto::HashType const& hash() const { return m_hash; }
    bcos::protocol::BlockNumber blockNumber() const { return m_blockNumber; }

    void setConsensusType(const std::string& _consensusType)
    {
        m_systemConfigs.set(SystemConfig::feature_rpbft, _consensusType);
    }
    std::string consensusType() const
    {
        if (auto value = m_systemConfigs.get(SystemConfig::feature_rpbft))
        {
            return *value;
        }
        return {"pbft"};
    }

    uint64_t blockTxCountLimit() const
    {
        if (auto value = m_systemConfigs.get(SystemConfig::tx_count_limit))
        {
            return boost::lexical_cast<uint64_t>(*value);
        }
        return 0;
    }

    bcos::consensus::ConsensusNodeList& mutableConsensusList() { return m_consensusNodeList; }
    bcos::consensus::ConsensusNodeList& mutableObserverList() { return m_observerNodeList; }
    bcos::consensus::ConsensusNodeList& mutableCandidateSealerNodeList()
    {
        return m_candidateSealerNodeList;
    }

    uint64_t leaderSwitchPeriod() const
    {
        if (auto value = m_systemConfigs.get(SystemConfig::consensus_leader_period))
        {
            return boost::lexical_cast<uint64_t>(*value);
        }
        return 0;
    }
    void setLeaderSwitchPeriod(uint64_t _leaderSwitchPeriod)
    {
        m_systemConfigs.set(
            SystemConfig::consensus_leader_period, std::to_string(_leaderSwitchPeriod));
    }

    uint64_t gasLimit() const
    {
        if (auto value = m_systemConfigs.get(SystemConfig::tx_gas_limit))
        {
            return boost::lexical_cast<uint64_t>(*value);
        }
        return 0;
    }
    void setGasLimit(uint64_t gasLimit)
    {
        m_systemConfigs.set(SystemConfig::tx_gas_limit, std::to_string(gasLimit));
    }

    std::string gasPrice() const
    {
        if (auto value = m_systemConfigs.get(SystemConfig::tx_gas_price))
        {
            return *value;
        }
        return {};
    }
    void setGasPrice(std::string gasPrice)
    {
        m_systemConfigs.set(SystemConfig::tx_gas_price, std::move(gasPrice));
    }

    // Not enforce to set this field, in memory data
    void setSealerId(int64_t _sealerId) { m_sealerId = _sealerId; }
    int64_t sealerId() const { return m_sealerId; }

    // Not enforce to set this field, in memory data
    void setTxsSize(int64_t _txsSize) { m_txsSize = _txsSize; }
    int64_t txsSize() const { return m_txsSize; }

    void setCompatibilityVersion(uint32_t _version)
    {
        m_systemConfigs.set(SystemConfig::compatibility_version, std::to_string(_version));
    }
    uint32_t compatibilityVersion() const
    {
        if (auto value = m_systemConfigs.get(SystemConfig::compatibility_version))
        {
            return boost::lexical_cast<uint32_t>(*value);
        }
        return 0;
    }

    void setAuthCheckStatus(uint32_t _authStatus)
    {
        m_systemConfigs.set(SystemConfig::auth_check_status, std::to_string(_authStatus));
    }
    uint32_t authCheckStatus() const
    {
        if (auto value = m_systemConfigs.get(SystemConfig::auth_check_status))
        {
            return boost::lexical_cast<uint32_t>(*value);
        }
        return 0;
    }

    void setEpochSealerNum(uint64_t epochSealerNum)
    {
        m_systemConfigs.set(
            SystemConfig::feature_rpbft_epoch_sealer_num, std::to_string(epochSealerNum));
    }
    uint64_t epochSealerNum() const
    {
        if (auto value = m_systemConfigs.get(SystemConfig::feature_rpbft_epoch_sealer_num))
        {
            return boost::lexical_cast<uint64_t>(*value);
        }
        return 0;
    }

    void setEpochBlockNum(uint64_t epochBlockNum)
    {
        m_systemConfigs.set(
            SystemConfig::feature_rpbft_epoch_block_num, std::to_string(epochBlockNum));
    }
    uint64_t epochBlockNum() const
    {
        if (auto value = m_systemConfigs.get(SystemConfig::feature_rpbft_epoch_block_num))
        {
            return boost::lexical_cast<uint64_t>(*value);
        }
        return 0;
    }

    void setNotifyRotateFlagInfo(const uint64_t _notifyRotateFlagInfo)
    {
        m_notifyRotateFlagInfo = _notifyRotateFlagInfo;
    }
    uint64_t notifyRotateFlagInfo() const { return m_notifyRotateFlagInfo; }

    Features const& features() const { return m_features; }
    void setFeatures(Features features) { m_features = features; }

    std::optional<std::string> chainId() const
    {
        if (auto value = m_systemConfigs.get(SystemConfig::web3_chain_id))
        {
            return *value;
        }
        return {};
    }
    void setChainId(std::string chainId)
    {
        m_systemConfigs.set(SystemConfig::web3_chain_id, std::move(chainId));
    }

    bcos::consensus::ConsensusNodeList m_consensusNodeList;
    bcos::consensus::ConsensusNodeList m_observerNodeList;
    bcos::consensus::ConsensusNodeList m_candidateSealerNodeList;

    SystemConfigs m_systemConfigs;
    Features m_features;
    bcos::crypto::HashType m_hash;
    bcos::protocol::BlockNumber m_blockNumber = 0;
    uint64_t m_notifyRotateFlagInfo{0};
    int64_t m_sealerId = -1;
    int64_t m_txsSize = -1;
};
}  // namespace bcos::ledger
