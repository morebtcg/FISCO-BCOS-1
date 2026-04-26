#pragma once

#include <bcos-crypto/interfaces/crypto/KeyFactory.h>
#include <bcos-framework/consensus/ConsensusNode.h>
#include <boost/property_tree/ptree_fwd.hpp>
#include <cstdint>
#include <string>
#include <utility>

namespace bcos::tool
{
class LedgerParamConfig
{
public:
    static constexpr std::uint32_t DEFAULT_EPOCH_SEALER_NUM = 4;
    static constexpr std::uint32_t DEFAULT_EPOCH_BLOCK_NUM = 1000;

    std::string const& consensusType() const { return m_consensusType; }
    void setConsensusType(std::string consensusType) { m_consensusType = std::move(consensusType); }

    std::uint64_t blockTxCountLimit() const { return m_blockTxCountLimit; }
    void setBlockTxCountLimit(std::uint64_t blockTxCountLimit)
    {
        m_blockTxCountLimit = blockTxCountLimit;
    }

    std::uint64_t txGasLimit() const { return m_txGasLimit; }
    void setTxGasLimit(std::uint64_t txGasLimit) { m_txGasLimit = txGasLimit; }

    std::uint32_t compatibilityVersion() const { return m_compatibilityVersion; }
    void setCompatibilityVersion(std::uint32_t compatibilityVersion)
    {
        m_compatibilityVersion = compatibilityVersion;
    }

    bcos::consensus::ConsensusNodeList const& consensusNodeList() const
    {
        return m_consensusNodeList;
    }
    void setConsensusNodeList(bcos::consensus::ConsensusNodeList consensusNodeList)
    {
        m_consensusNodeList = std::move(consensusNodeList);
    }

    std::uint32_t epochSealerNum() const { return m_epochSealerNum; }
    void setEpochSealerNum(std::uint32_t epochSealerNum) { m_epochSealerNum = epochSealerNum; }

    std::uint32_t epochBlockNum() const { return m_epochBlockNum; }
    void setEpochBlockNum(std::uint32_t epochBlockNum) { m_epochBlockNum = epochBlockNum; }

    std::uint64_t leaderSwitchPeriod() const { return m_leaderSwitchPeriod; }
    void setLeaderSwitchPeriod(std::uint64_t leaderSwitchPeriod)
    {
        m_leaderSwitchPeriod = leaderSwitchPeriod;
    }

    void loadLedgerConfig(
        boost::property_tree::ptree const& config, bcos::crypto::KeyFactory::Ptr const& keyFactory);

private:
    std::string m_consensusType;
    std::uint64_t m_blockTxCountLimit = 0;
    std::uint64_t m_txGasLimit = 0;
    std::uint32_t m_compatibilityVersion = 0;
    bcos::consensus::ConsensusNodeList m_consensusNodeList;
    std::uint32_t m_epochSealerNum = DEFAULT_EPOCH_SEALER_NUM;
    std::uint32_t m_epochBlockNum = DEFAULT_EPOCH_BLOCK_NUM;
    std::uint64_t m_leaderSwitchPeriod = 1;
};
}  // namespace bcos::tool
