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
         * @brief configuration for the node
         * @file NodeConfig.cpp
         * @author: yujiechen
         * @date 2021-06-10
         */
        #include "NodeConfig.h"
        #include "VersionConverter.h"
        #include "bcos-framework/bcos-framework/protocol/Protocol.h"
        #include "bcos-framework/consensus/ConsensusNode.h"
        #include "bcos-utilities/BoostLog.h"
        #include "bcos-utilities/Common.h"
        #include <bcos-framework/ledger/GenesisConfig.h>
        #include <bcos-framework/protocol/GlobalConfig.h>
        #include <json/forwards.h>
        #include <json/reader.h>
        #include <json/value.h>
        #include <servant/RemoteLogger.h>
        #include <util/tc_clientsocket.h>
        #include <boost/algorithm/string.hpp>
        #include <boost/algorithm/string/case_conv.hpp>
        #include <boost/algorithm/string/classification.hpp>
        #include <boost/algorithm/string/split.hpp>
        #include <boost/algorithm/string/trim.hpp>
        #include <boost/throw_exception.hpp>
        #include <utility>

        constexpr static auto MAX_BLOCK_LIMIT = 5000;

        using namespace bcos;
        using namespace bcos::crypto;
        using namespace bcos::tool;
        using namespace bcos::consensus;
        using namespace bcos::ledger;
        using namespace bcos::protocol;

        namespace
        {
        constexpr int MIN_VALID_PORT = 1025;
        constexpr int MAX_VALID_PORT = 65535;
        }  // namespace

        NodeConfig::NodeConfig(KeyFactory::Ptr _keyFactory)
          : m_keyFactory(std::move(_keyFactory)), m_ledgerConfig(std::make_shared<LedgerConfig>())
        {}

        void NodeConfig::loadConfig(boost::property_tree::ptree const& _pt, bool _enforceMemberID,
            bool _enforceChainConfig, bool _enforceGroupId)
        {
            // if version < 3.1.0, config.ini include chainConfig
            if (_enforceChainConfig || (m_genesisConfig.m_compatibilityVersion <
                                               (uint32_t)bcos::protocol::BlockVersion::V3_1_VERSION &&
                                           m_genesisConfig.m_compatibilityVersion >=
                                               (uint32_t)bcos::protocol::BlockVersion::MIN_VERSION))
            {
                loadChainConfig(_pt, _enforceGroupId);
            }
            loadCertConfig(_pt);
            loadRpcConfig(_pt);
            loadWeb3RpcConfig(_pt);
            loadGatewayConfig(_pt);
            loadSealerConfig(_pt);
            loadTxPoolConfig(_pt);
            // loadSecurityConfig before loadStorageSecurityConfig for deciding whether to use HSM
            loadSecurityConfig(_pt);
            loadStorageSecurityConfig(_pt);
            loadExecutorNormalConfig(_pt);

            loadFailOverConfig(_pt, _enforceMemberID);
            loadStorageConfig(_pt);
            loadConsensusConfig(_pt);
            loadSyncConfig(_pt);
            loadOthersConfig(_pt);
        }

        void NodeConfig::loadGenesisConfig(boost::property_tree::ptree const& _genesisConfig)
        {
            // if version >= 3.1.0, genesisBlock include chainConfig
            auto compatibilityVersion = _genesisConfig.get<std::string>(
                "version.compatibility_version", bcos::protocol::RC4_VERSION_STR);
            m_genesisConfig.m_compatibilityVersion = toVersionNumber(compatibilityVersion);
            if (m_genesisConfig.m_compatibilityVersion >=
                (uint32_t)bcos::protocol::BlockVersion::V3_1_VERSION)
            {
                loadChainConfig(_genesisConfig, true);
            }
            loadWeb3ChainConfig(_genesisConfig);
            loadGenesisFeatures(_genesisConfig);

            loadLedgerConfig(_genesisConfig);
            loadExecutorConfig(_genesisConfig);
        }

void NodeConfig::loadRpcConfig(boost::property_tree::ptree const& _pt)
{
    auto& rpcConfig = m_config.mutableRpc();
    auto needRetInput = rpcConfig.loadRpcConfig(_pt);
    g_BCOSConfig.setNeedRetInput(needRetInput);

    NodeConfig_LOG(INFO) << LOG_DESC("loadRpcConfig") << LOG_KV("listenIP", rpcConfig.listenIP())
                         << LOG_KV("listenPort", rpcConfig.listenPort())
                         << LOG_KV("smSsl", rpcConfig.smSsl())
                         << LOG_KV("disableSsl", rpcConfig.disableSsl())
                         << LOG_KV("needRetInput", needRetInput);
}

void NodeConfig::loadWeb3RpcConfig(boost::property_tree::ptree const& _pt)
{
    auto& web3RpcConfig = m_config.mutableWeb3Rpc();
    web3RpcConfig.loadWeb3RpcConfig(_pt);

    NodeConfig_LOG(INFO) << LOG_DESC("loadWeb3RpcConfig")
                         << LOG_KV("enableWeb3Rpc", web3RpcConfig.enable())
                         << LOG_KV("listenIP", web3RpcConfig.listenIP())
                         << LOG_KV("listenPort", web3RpcConfig.listenPort())
                         << LOG_KV("threadCount", web3RpcConfig.threadSize())
                         << LOG_KV("filterTimeout", web3RpcConfig.filterTimeout() / 1000)
                         << LOG_KV("maxProcessBlock", web3RpcConfig.maxProcessBlock())
                         << LOG_KV("batchRequestSizeLimit", web3RpcConfig.batchRequestSizeLimit())
                         << LOG_KV("enableCors", web3RpcConfig.enableCors())
                         << LOG_KV("corsAllowedOrigins", web3RpcConfig.corsAllowedOrigins())
                         << LOG_KV("corsAllowedMethods", web3RpcConfig.corsAllowedMethods())
                         << LOG_KV("corsAllowedHeaders", web3RpcConfig.corsAllowedHeaders())
                         << LOG_KV("corsMaxAge", web3RpcConfig.corsMaxAge())
                         << LOG_KV("corsAllowCredentials", web3RpcConfig.corsAllowCredentials())
                         << LOG_KV("syncTransaction", web3RpcConfig.syncTransaction());
}

void NodeConfig::loadGatewayConfig(boost::property_tree::ptree const& _pt)
{
    auto& gatewayConfig = m_config.mutableGateway();
    gatewayConfig.loadGatewayConfig(_pt);

    NodeConfig_LOG(INFO) << LOG_DESC("loadGatewayConfig")
                         << LOG_KV("listenIP", gatewayConfig.listenIP())
                         << LOG_KV("listenPort", gatewayConfig.listenPort())
                         << LOG_KV("smSsl", gatewayConfig.smSsl())
                         << LOG_KV("nodesFile", gatewayConfig.nodeFileName());
}

void NodeConfig::loadCertConfig(boost::property_tree::ptree const& _pt)
{
    auto& certConfig = m_config.mutableCert();
    certConfig.loadCertConfig(_pt);

    NodeConfig_LOG(INFO) << LOG_DESC("loadCertConfig")
                         << LOG_KV("ca_path", certConfig.certPath())
                         << LOG_KV("sm_ca_cert", certConfig.smCaCert())
                         << LOG_KV("sm_node_cert", certConfig.smNodeCert())
                         << LOG_KV("sm_node_key", certConfig.smNodeKey())
                         << LOG_KV("sm_ennode_cert", certConfig.enSmNodeCert())
                         << LOG_KV("sm_ennode_key", certConfig.enSmNodeKey());

    NodeConfig_LOG(INFO) << LOG_DESC("loadCertConfig")
                         << LOG_KV("ca_path", certConfig.certPath())
                         << LOG_KV("ca_cert", certConfig.caCert())
                         << LOG_KV("node_cert", certConfig.nodeCert())
                         << LOG_KV("node_key", certConfig.nodeKey());
}

// load the txpool related params
void NodeConfig::loadTxPoolConfig(boost::property_tree::ptree const& _pt)
{
    auto& txPoolConfig = m_config.mutableTxPool();
    txPoolConfig.loadTxPoolConfig(
        _pt, m_config.sealer().minSealTime(), DEFAULT_MIN_CONSENSUS_TIME_MS);

    NodeConfig_LOG(INFO) << LOG_DESC("loadTxPoolConfig")
                         << LOG_KV("txpoolLimit", txPoolConfig.limit())
                         << LOG_KV("notifierWorkers", txPoolConfig.notifyWorkerNum())
                         << LOG_KV("verifierWorkers", txPoolConfig.verifierWorkerNum())
                         << LOG_KV("checkBlockLimit", txPoolConfig.checkBlockLimit())
                         << LOG_KV("txsExpirationTime(ms)", txPoolConfig.txsExpirationTime())
                         << LOG_KV("enableTxsFromFreeNode", txPoolConfig.enableTxsFromFreeNode());
}

void NodeConfig::loadChainConfig(boost::property_tree::ptree const& _pt, bool _enforceGroupId)
{
    auto& chainConfig = m_config.mutableChain();
    chainConfig.loadChainConfig(_pt, _enforceGroupId, MAX_BLOCK_LIMIT);

    m_genesisConfig.m_smCrypto = chainConfig.smCrypto();
    m_genesisConfig.m_groupID = chainConfig.groupID();
    m_genesisConfig.m_chainID = chainConfig.chainID();
    m_blockLimit = chainConfig.blockLimit();

    NodeConfig_LOG(INFO) << METRIC << LOG_DESC("loadChainConfig")
                         << LOG_KV("smCrypto", chainConfig.smCrypto())
                         << LOG_KV("chainId", chainConfig.chainID())
                         << LOG_KV("groupId", chainConfig.groupID())
                         << LOG_KV("blockLimit", m_blockLimit);
}

void NodeConfig::NodeConfig::loadWeb3ChainConfig(boost::property_tree::ptree const& _pt)
{
    auto& chainConfig = m_config.mutableChain();
    chainConfig.loadWeb3ChainConfig(_pt);
    m_genesisConfig.m_web3ChainID = chainConfig.web3ChainID();

    NodeConfig_LOG(INFO) << LOG_DESC("loadWeb3ChainConfig")
                         << LOG_KV("web3ChainID", chainConfig.web3ChainID());
}

void NodeConfig::loadSecurityConfig(boost::property_tree::ptree const& _pt)
{
    m_config.mutableSecurity().loadSecurityConfig(_pt);
}

void NodeConfig::loadSealerConfig(boost::property_tree::ptree const& _pt)
{
    auto& sealerConfig = m_config.mutableSealer();
    sealerConfig.loadSealerConfig(_pt);
    auto minSealTime = sealerConfig.minSealTime();
    if (minSealTime <= 0 || minSealTime > DEFAULT_MAX_SEAL_TIME_MS)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "Please set consensus.min_seal_time between 1 and 600000!"));
    }
    NodeConfig_LOG(INFO) << LOG_DESC("loadSealerConfig") << LOG_KV("minSealTime", minSealTime);
}

void NodeConfig::loadStorageSecurityConfig(boost::property_tree::ptree const& _pt)
{
    m_config.mutableSecurity().loadStorageSecurityConfig(_pt);
}

void NodeConfig::loadSyncConfig(const boost::property_tree::ptree& _pt)
{
    auto& syncConfig = m_config.mutableSync();
    syncConfig.loadSyncConfig(_pt);
    auto treeWidth = syncConfig.treeWidth();
    if (treeWidth == 0 || treeWidth > UINT16_MAX)
    {
        BOOST_THROW_EXCEPTION(
            InvalidConfig() << errinfo_comment("Please set sync.tree_width in 1~65535"));
    }

    NodeConfig_LOG(INFO) << LOG_DESC("loadSyncConfig")
                         << LOG_KV("sync_block_by_tree", syncConfig.enableSendBlockStatusByTree())
                         << LOG_KV("send_txs_by_tree", syncConfig.enableSendTxByTree())
                         << LOG_KV("tree_width", treeWidth);
}

void NodeConfig::loadStorageConfig(boost::property_tree::ptree const& _pt)
{
    auto& storageConfig = m_config.mutableStorage();
    storageConfig.loadStorageConfig(_pt, m_genesisConfig.m_groupID);
    g_BCOSConfig.setStorageType(storageConfig.storageType());
    auto pdAddrsValue = _pt.get<std::string>("storage.pd_addrs", "127.0.0.1:2379");
    NodeConfig_LOG(INFO) << LOG_DESC("loadStorageConfig")
                         << LOG_KV("storagePath", storageConfig.storagePath())
                         << LOG_KV("KeyPage", storageConfig.keyPageSize())
                         << LOG_KV("storageType", storageConfig.storageType())
                         << LOG_KV("pdAddrs", pdAddrsValue)
                         << LOG_KV("pdCaPath", storageConfig.pdCaPath())
                         << LOG_KV("enableArchive", storageConfig.enableArchive())
                         << LOG_KV("enableSeparateBlockAndState",
                                storageConfig.enableSeparateBlockAndState())
                         << LOG_KV("archiveListenIP", storageConfig.archiveListenIP())
                         << LOG_KV("archiveListenPort", storageConfig.archiveListenPort())
                         << LOG_KV("enable_rocksdb_blob", storageConfig.enableRocksDBBlob())
                         << LOG_KV("enableLRUCacheStorage", storageConfig.enableLRUCacheStorage());
}

// Note: In components that do not require failover, do not need to set member_id
void NodeConfig::loadFailOverConfig(boost::property_tree::ptree const& _pt, bool _enforceMemberID)
{
    auto& failOverConfig = m_config.mutableFailOver();
    failOverConfig.loadFailOverConfig(_pt, _enforceMemberID, DEFAULT_MIN_LEASE_TTL_SECONDS);
    if (!failOverConfig.enable())
    {
        return;
    }

    NodeConfig_LOG(INFO) << LOG_DESC("loadFailOverConfig")
                         << LOG_KV("failOverClusterUrl", failOverConfig.clusterUrl())
                         << LOG_KV("memberID",
                                failOverConfig.memberID().empty() ? "not-set" : failOverConfig.memberID())
                         << LOG_KV("leaseTTL", failOverConfig.leaseTTL())
                         << LOG_KV("enableFailOver", failOverConfig.enable());
}

void NodeConfig::loadOthersConfig(boost::property_tree::ptree const& _pt)
{
    auto& othersConfig = m_config.mutableOthers();
    othersConfig.loadOthersConfig(_pt);

    NodeConfig_LOG(INFO) << LOG_DESC("loadOthersConfig")
                         << LOG_KV("sendTxTimeout", othersConfig.sendTxTimeout())
                         << LOG_KV("vmCacheSize", othersConfig.vmCacheSize())
                         << LOG_KV("checkTransactionSignature", othersConfig.checkTransactionSignature())
                         << LOG_KV("checkParallelConflict", othersConfig.checkParallelConflict())
                         << LOG_KV("singlePointConsensus", othersConfig.singlePointConsensus())
                         << LOG_KV("forceSender", toHex(othersConfig.forceSender()));
}

void NodeConfig::loadConsensusConfig(boost::property_tree::ptree const& _pt)
{
    auto& consensusConfig = m_config.mutableConsensus();
    consensusConfig.loadConsensusConfig(
        _pt, DEFAULT_MIN_CONSENSUS_TIME_MS, DEFAULT_PIPELINE_SIZE);

    NodeConfig_LOG(INFO) << LOG_DESC("loadConsensusConfig")
                         << LOG_KV("checkPointTimeoutInterval",
                                consensusConfig.checkpointTimeoutInterval())
                         << LOG_KV("pipeline_size", consensusConfig.pipelineSize());
}

void NodeConfig::loadLedgerConfig(boost::property_tree::ptree const& _genesisConfig)
{
    auto& ledgerParamConfig = m_config.mutableLedgerParam();
    ledgerParamConfig.loadLedgerConfig(_genesisConfig, m_keyFactory);

    m_genesisConfig.m_consensusType = ledgerParamConfig.consensusType();
    m_genesisConfig.m_txCountLimit = ledgerParamConfig.blockTxCountLimit();
    m_genesisConfig.m_txGasLimit = ledgerParamConfig.txGasLimit();
    m_genesisConfig.m_compatibilityVersion = ledgerParamConfig.compatibilityVersion();
    m_genesisConfig.m_epochSealerNum = ledgerParamConfig.epochSealerNum();
    m_genesisConfig.m_epochBlockNum = ledgerParamConfig.epochBlockNum();

    m_ledgerConfig->setBlockTxCountLimit(ledgerParamConfig.blockTxCountLimit());
    m_ledgerConfig->setConsensusNodeList(ledgerParamConfig.consensusNodeList());
    m_ledgerConfig->setLeaderSwitchPeriod(ledgerParamConfig.leaderSwitchPeriod());

    NodeConfig_LOG(INFO)
        << LOG_DESC("loadLedgerConfig") << LOG_KV("consensus_type", m_genesisConfig.m_consensusType)
        << LOG_KV("block_tx_count_limit", m_ledgerConfig->blockTxCountLimit())
        << LOG_KV("gas_limit", m_genesisConfig.m_txGasLimit)
        << LOG_KV("leader_period", m_ledgerConfig->leaderSwitchPeriod())
        << LOG_KV("minSealTime", m_config.sealer().minSealTime())
        << LOG_KV("compatibilityVersion",
               (bcos::protocol::BlockVersion)m_genesisConfig.m_compatibilityVersion);
}

void NodeConfig::loadExecutorConfig(boost::property_tree::ptree const& _pt)
{
    auto& executorConfig = m_config.mutableExecutor();
    executorConfig.loadGenesisExecutorConfig(_pt, m_genesisConfig.m_compatibilityVersion);

    m_genesisConfig.m_isWasm = executorConfig.isWasm();
    m_genesisConfig.m_isAuthCheck = executorConfig.isAuthCheck();
    m_genesisConfig.m_isSerialExecute = executorConfig.isSerialExecute();
    m_genesisConfig.m_executorVersion = executorConfig.executorVersion();
    m_genesisConfig.m_authAdminAccount = executorConfig.authAdminAccount();

    NodeConfig_LOG(INFO) << METRIC << LOG_DESC("loadExecutorConfig")
                         << LOG_KV("isWasm", executorConfig.isWasm())
                         << LOG_KV("isAuthCheck", executorConfig.isAuthCheck())
                         << LOG_KV("authAdminAccount", executorConfig.authAdminAccount())
                         << LOG_KV("isSerialExecute", executorConfig.isSerialExecute());
}

// load config.ini
void NodeConfig::loadExecutorNormalConfig(boost::property_tree::ptree const& _pt)
{
    auto& executorConfig = m_config.mutableExecutor();
    executorConfig.loadExecutorNormalConfig(_pt);
    g_BCOSConfig.setEnableDAG(executorConfig.enableDag());
    NodeConfig_LOG(INFO) << METRIC << LOG_DESC("loadExecutorNormalConfig: config.ini")
                         << LOG_KV("enableDag", executorConfig.enableDag());
}

bool NodeConfig::isValidPort(int port)
{
    return port >= MIN_VALID_PORT && port <= MAX_VALID_PORT;
}

void bcos::tool::NodeConfig::loadGenesisFeatures(boost::property_tree::ptree const& ptree)
{
    if (auto node = ptree.get_child_optional("features"))
    {
        for (const auto& it : *node)
        {
            auto flag = it.first;
            auto enableNumber = it.second.get_value<bool>();
            m_genesisConfig.m_features.emplace_back(
                ledger::FeatureSet{.flag = ledger::Features::string2Flag(flag),
                    .enable = static_cast<int>(enableNumber)});
        }
    }
}

std::string bcos::tool::generateGenesisData(
    ledger::GenesisConfig const& genesisConfig, ledger::LedgerConfig const& ledgerConfig)
{
    if (genesisConfig.m_compatibilityVersion >=
        (uint32_t)bcos::protocol::BlockVersion::V3_1_VERSION)
    {
        std::stringstream ss;
        ss << "[chain]" << '\n'
           << "sm_crypto:" << genesisConfig.m_smCrypto << '\n'
           << "chainID: " << genesisConfig.m_chainID << '\n'
              << "groupID: " << genesisConfig.m_groupID << '\n'
              << "[consensus]" << '\n'
           << "consensus_type: " << genesisConfig.m_consensusType << '\n'
           << "block_tx_count_limit:" << genesisConfig.m_txCountLimit << '\n'
           << "leader_period:" << genesisConfig.m_leaderSwitchPeriod << '\n'
           << "[version]" << '\n'
           << "compatibility_version:"
           << bcos::protocol::BlockVersion(genesisConfig.m_compatibilityVersion) << '\n'
           << "[tx]" << '\n'
           << "gaslimit:" << genesisConfig.m_txGasLimit << '\n'
           << "[executor]" << '\n'
           << "iswasm: " << genesisConfig.m_isWasm << '\n'
           << "isAuthCheck:" << genesisConfig.m_isAuthCheck << '\n'
           << "authAdminAccount:" << genesisConfig.m_authAdminAccount << '\n'
           << "isSerialExecute:" << genesisConfig.m_isSerialExecute << '\n';
        if (genesisConfig.m_compatibilityVersion >=
            (uint32_t)bcos::protocol::BlockVersion::V3_5_VERSION)
        {
            ss << "epochSealerNum:" << genesisConfig.m_epochSealerNum << '\n'
               << "epochBlockNum:" << genesisConfig.m_epochBlockNum << '\n';
        }
        if (!genesisConfig.m_features.empty())  // TODO: Need version check?
        {
            ss << "[features]" << '\n';
            for (const auto& feature : genesisConfig.m_features)
            {
                ss << feature.flag << ":" << feature.enable << '\n';
            }
        }

        size_t j = 0;
        for (const auto& node : ledgerConfig.consensusNodeList())
        {
            ss << "node." + boost::lexical_cast<std::string>(j) + ":" +
                      toHex(node.nodeID->data()) + "," + std::to_string(node.voteWeight) +
                      "\n";
            ++j;
        }
        std::string genesisdata = ss.str();
        NodeConfig_LOG(INFO) << LOG_BADGE("generateGenesisData")
                             << LOG_KV("genesisData", genesisdata);

        return genesisdata;
    }

    std::stringstream executorStream;
    executorStream << genesisConfig.m_isWasm << "-" << genesisConfig.m_isAuthCheck << "-"
                   << genesisConfig.m_authAdminAccount << "-" << genesisConfig.m_isSerialExecute;

    std::stringstream ss;
    ss << ledgerConfig.blockTxCountLimit() << "-" << ledgerConfig.leaderSwitchPeriod() << "-"
       << genesisConfig.m_txGasLimit << "-"
       << protocol::BlockVersion(genesisConfig.m_compatibilityVersion) << "-"
       << executorStream.str();
    for (const auto& node : ledgerConfig.consensusNodeList())
    {
        ss << toHex(node.nodeID->data()) << "," << node.voteWeight << ";";
    }
    auto genesisdata = ss.str();
    NodeConfig_LOG(INFO) << LOG_BADGE("generateGenesisData") << LOG_KV("genesisData", genesisdata);

    return genesisdata;
}
bcos::ledger::GenesisConfig const& bcos::tool::NodeConfig::genesisConfig() const
{
    return m_genesisConfig;
}
int bcos::tool::NodeConfig::executorVersion() const
{
    return m_config.executor().executorVersion();
}
