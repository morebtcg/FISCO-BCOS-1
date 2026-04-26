#include "LedgerParamConfig.h"
#include "Exceptions.h"
#include "VersionConverter.h"
#include <bcos-framework/ledger/LedgerTypeDef.h>
#include <bcos-framework/protocol/Protocol.h>
#include <bcos-utilities/DataConvertUtility.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/throw_exception.hpp>
#include <algorithm>

using namespace bcos::tool;

namespace
{
constexpr std::uint64_t DEFAULT_BLOCK_TX_COUNT_LIMIT = 1000;
constexpr std::uint64_t DEFAULT_CONSENSUS_LEADER_PERIOD = 1;
constexpr std::uint64_t DEFAULT_TX_GAS_LIMIT = 3000000000;

bcos::consensus::ConsensusNodeList parseConsensusNodeList(
    boost::property_tree::ptree const& config, bcos::crypto::KeyFactory::Ptr const& keyFactory)
{
    constexpr std::string_view SECTION_NAME = "consensus";
    constexpr std::string_view SUB_SECTION_NAME = "node.";

    if (!config.get_child_optional(std::string(SECTION_NAME)))
    {
        return {};
    }

    bcos::consensus::ConsensusNodeList nodeList;
    for (auto const& item : config.get_child(std::string(SECTION_NAME)))
    {
        if (!item.first.starts_with(SUB_SECTION_NAME))
        {
            continue;
        }

        std::string data = item.second.data();
        std::vector<std::string> nodeInfo;
        boost::split(nodeInfo, data, boost::is_any_of(":"));
        if (nodeInfo.empty())
        {
            BOOST_THROW_EXCEPTION(InvalidConfig() << bcos::errinfo_comment(
                                      "Uninitialized nodeInfo, key: " + item.first +
                                      ", value: " + data));
        }

        std::string nodeId = nodeInfo[0];
        boost::to_lower(nodeId);
        std::int64_t voteWeight = 1;
        std::int64_t termWeight = 0;
        if (nodeInfo.size() > 1)
        {
            auto& voteWeightInfo = nodeInfo[1];
            boost::trim(voteWeightInfo);
            voteWeight = boost::lexical_cast<std::int64_t>(voteWeightInfo);
        }
        if (nodeInfo.size() > 2)
        {
            auto& termWeightInfo = nodeInfo[2];
            boost::trim(termWeightInfo);
            termWeight = boost::lexical_cast<std::int64_t>(termWeightInfo);
        }
        if (voteWeight <= 0 || termWeight < 0)
        {
            BOOST_THROW_EXCEPTION(InvalidConfig() << bcos::errinfo_comment(
                                      "Please set weight for " + nodeId + " to positive!"));
        }

        bcos::consensus::ConsensusNode consensusNode{.nodeID =
                keyFactory->createKey(bcos::fromHex(nodeId)),
            .type = bcos::consensus::Type::consensus_sealer,
            .voteWeight = static_cast<std::uint64_t>(voteWeight),
            .termWeight = static_cast<std::uint64_t>(termWeight),
            .enableNumber = 0};
        nodeList.push_back(std::move(consensusNode));
    }

    std::sort(nodeList.begin(), nodeList.end());
    return nodeList;
}
}

void LedgerParamConfig::loadLedgerConfig(
    boost::property_tree::ptree const& config, bcos::crypto::KeyFactory::Ptr const& keyFactory)
{
    try
    {
        setConsensusType(config.get<std::string>("consensus.consensus_type", "pbft"));
    }
    catch (std::exception const&)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "consensus.consensus_type is null, please set it!"));
    }
    if (consensusType() != bcos::ledger::PBFT_CONSENSUS_TYPE &&
        consensusType() != bcos::ledger::RPBFT_CONSENSUS_TYPE)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "consensus.consensus_type is illegal, it must be pbft or rpbft!"));
    }

    auto blockTxCountLimit =
        config.get<std::uint64_t>("consensus.block_tx_count_limit", DEFAULT_BLOCK_TX_COUNT_LIMIT);
    if (blockTxCountLimit == 0)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "Please set consensus.block_tx_count_limit to positive!"));
    }
    setBlockTxCountLimit(blockTxCountLimit);

    auto txGasLimit = config.get<std::uint64_t>("tx.gas_limit", DEFAULT_TX_GAS_LIMIT);
    if (txGasLimit <= bcos::ledger::TX_GAS_LIMIT_MIN)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "Please set tx.gas_limit to more than " +
                                  std::to_string(bcos::ledger::TX_GAS_LIMIT_MIN) + " !"));
    }
    setTxGasLimit(txGasLimit);

    auto compatibilityVersion =
        config.get<std::string>("version.compatibility_version", bcos::protocol::RC4_VERSION_STR);
    setCompatibilityVersion(toVersionNumber(compatibilityVersion));

    auto consensusNodeList = parseConsensusNodeList(config, keyFactory);
    if (consensusNodeList.empty())
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment("Must set sealerList!"));
    }
    setConsensusNodeList(std::move(consensusNodeList));

    if (consensusType() == bcos::ledger::RPBFT_CONSENSUS_TYPE)
    {
        setEpochSealerNum(config.get<std::uint32_t>(
            "consensus.epoch_sealer_num", LedgerParamConfig::DEFAULT_EPOCH_SEALER_NUM));
        setEpochBlockNum(
            config.get<std::uint32_t>("consensus.epoch_block_num",
                LedgerParamConfig::DEFAULT_EPOCH_BLOCK_NUM));
    }

    auto consensusLeaderPeriod =
        config.get<std::uint64_t>("consensus.leader_period", DEFAULT_CONSENSUS_LEADER_PERIOD);
    if (consensusLeaderPeriod == 0)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "Please set consensus.leader_period to positive!"));
    }
    setLeaderSwitchPeriod(consensusLeaderPeriod);
}
