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
 * @file NodeConfig.h
 * @author: yujiechen
 * @date 2021-06-10
 */
#pragma once
#include "ConfigAggregate.h"
#include "bcos-framework/ledger/GenesisConfig.h"
#include "bcos-framework/ledger/LedgerConfig.h"
#include <bcos-crypto/interfaces/crypto/KeyFactory.h>
#include <bcos-framework/Common.h>
#include <bcos-framework/protocol/Protocol.h>
#include <util/tc_clientsocket.h>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <cstddef>

#define NodeConfig_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("NodeConfig")
namespace bcos::tool
{
class NodeConfig
{
public:
    constexpr static ssize_t DEFAULT_MIN_CONSENSUS_TIME_MS = 3000;
    constexpr static ssize_t DEFAULT_MIN_LEASE_TTL_SECONDS = 3;
    constexpr static ssize_t DEFAULT_MAX_SEAL_TIME_MS = 600000;
    constexpr static ssize_t DEFAULT_PIPELINE_SIZE = 50;

    using Ptr = std::shared_ptr<NodeConfig>;
    NodeConfig() : m_ledgerConfig(std::make_shared<bcos::ledger::LedgerConfig>()) {}

    NodeConfig(const NodeConfig&) = default;
    NodeConfig(NodeConfig&&) = default;
    NodeConfig& operator=(const NodeConfig&) = default;
    NodeConfig& operator=(NodeConfig&&) = default;
    explicit NodeConfig(bcos::crypto::KeyFactory::Ptr _keyFactory);
    virtual ~NodeConfig() = default;

    virtual void loadConfig(std::string const& _configPath, bool _enforceMemberID = true,
        bool enforceChainConfig = false, bool enforceGroupId = true)
    {
        boost::property_tree::ptree iniConfig;
        boost::property_tree::read_ini(_configPath, iniConfig);
        loadConfig(iniConfig, _enforceMemberID, enforceChainConfig, enforceGroupId);
    }
    virtual void loadGenesisConfig(std::string const& _genesisConfigPath)
    {
        boost::property_tree::ptree genesisConfig;
        boost::property_tree::read_ini(_genesisConfigPath, genesisConfig);
        loadGenesisConfig(genesisConfig);
    }

    virtual void loadConfigFromString(std::string const& _content)
    {
        boost::property_tree::ptree iniConfig;
        std::stringstream contentStream(_content);
        boost::property_tree::read_ini(contentStream, iniConfig);
        loadConfig(iniConfig);
    }

    virtual void loadGenesisConfigFromString(std::string const& _content)
    {
        boost::property_tree::ptree genesisConfig;
        std::stringstream contentStream(_content);
        boost::property_tree::read_ini(contentStream, genesisConfig);
        loadGenesisConfig(genesisConfig);
    }

    virtual void loadConfig(boost::property_tree::ptree const& _pt, bool _enforceMemberID = true,
        bool _enforceChainConfig = false, bool _enforceGroupId = true);
    virtual void loadGenesisConfig(boost::property_tree::ptree const& _genesisConfig);

    // the txpool configurations
    size_t txpoolLimit() const { return m_config.txPool().limit(); }
    size_t notifyWorkerNum() const { return m_config.txPool().notifyWorkerNum(); }
    size_t verifierWorkerNum() const { return m_config.txPool().verifierWorkerNum(); }
    int64_t txsExpirationTime() const { return m_config.txPool().txsExpirationTime(); }
    bool checkBlockLimit() const { return m_config.txPool().checkBlockLimit(); }

    bool smCryptoType() const { return m_config.chain().smCrypto(); }
    size_t blockLimit() const { return m_blockLimit; }

    size_t minSealTime() const { return m_config.sealer().minSealTime(); }
    bool allowFreeNodeSync() const { return m_config.sync().allowFreeNodeSync(); }
    size_t checkPointTimeoutInterval() const
    {
        return m_config.consensus().checkpointTimeoutInterval();
    }
    size_t pipelineSize() const { return m_config.consensus().pipelineSize(); }

    std::string const& storagePath() const { return m_config.storage().storagePath(); }
    std::string const& stateDBPath() const { return m_config.storage().stateDBPath(); }
    std::string const& blockDBPath() const { return m_config.storage().blockDBPath(); }
    std::string const& storageType() const { return m_config.storage().storageType(); }
    size_t keyPageSize() const { return m_config.storage().keyPageSize(); }
    int maxWriteBufferNumber() const { return m_config.storage().maxWriteBufferNumber(); }
    bool enableStatistics() const { return m_config.storage().enableStatistics(); }
    int maxBackgroundJobs() const { return m_config.storage().maxBackgroundJobs(); }
    size_t writeBufferSize() const { return m_config.storage().writeBufferSize(); }
    int minWriteBufferNumberToMerge() const
    {
        return m_config.storage().minWriteBufferNumberToMerge();
    }
    size_t blockCacheSize() const { return m_config.storage().blockCacheSize(); }
    bool enableRocksDBBlob() const { return m_config.storage().enableRocksDBBlob(); }
    std::vector<std::string> const& pdAddrs() const { return m_config.storage().pdAddrs(); }
    std::string const& pdCaPath() const { return m_config.storage().pdCaPath(); }
    std::string const& pdCertPath() const { return m_config.storage().pdCertPath(); }
    std::string const& pdKeyPath() const { return m_config.storage().pdKeyPath(); }
    std::string const& storageDBName() const { return m_config.storage().storageDBName(); }
    std::string const& stateDBName() const { return m_config.storage().stateDBName(); }
    bool enableArchive() const { return m_config.storage().enableArchive(); }
    bool syncArchivedBlocks() const { return m_config.storage().syncArchivedBlocks(); }
    bool enableSeparateBlockAndState() const
    {
        return m_config.storage().enableSeparateBlockAndState();
    }
    std::string const& archiveListenIP() const { return m_config.storage().archiveListenIP(); }
    uint16_t archiveListenPort() const { return m_config.storage().archiveListenPort(); }

    bcos::crypto::KeyFactory::Ptr keyFactory() { return m_keyFactory; }

    bcos::ledger::LedgerConfig::Ptr ledgerConfig() { return m_ledgerConfig; }

    std::string const& genesisData() const { return m_genesisData; }

    bool isWasm() const { return m_config.executor().isWasm(); }
    bool isAuthCheck() const { return m_config.executor().isAuthCheck(); }
    bool isSerialExecute() const { return m_config.executor().isSerialExecute(); }
    size_t vmCacheSize() const { return m_config.others().vmCacheSize(); }

    std::string const& authAdminAddress() const { return m_config.executor().authAdminAccount(); }

    std::string const& web3ChainID() const { return m_config.chain().web3ChainID(); }

    ConfigAggregate const& config() const { return m_config; }
    ServiceConfig& mutableServiceConfig() { return m_config.mutableService(); }
    RpcConfig const& rpcConfig() const { return m_config.rpc(); }
    StorageConfig const& storageConfig() const { return m_config.storage(); }
    ServiceConfig const& serviceConfig() const { return m_config.service(); }
    Web3RpcConfig const& web3RpcConfig() const { return m_config.web3Rpc(); }
    GatewayConfig const& gatewayConfig() const { return m_config.gateway(); }
    CertConfig const& certConfig() const { return m_config.cert(); }
    ChainConfig const& chainConfig() const { return m_config.chain(); }
    TxPoolConfig const& txPoolConfig() const { return m_config.txPool(); }
    FailOverConfig const& failOverConfig() const { return m_config.failOver(); }
    ConsensusConfig const& consensusConfig() const { return m_config.consensus(); }
    ExecutorConfig const& executorConfig() const { return m_config.executor(); }
    LedgerParamConfig const& ledgerParamConfig() const { return m_config.ledgerParam(); }
    OthersConfig const& othersConfig() const { return m_config.others(); }
    SyncConfig const& syncConfig() const { return m_config.sync(); }
    SealerConfig const& sealerConfig() const { return m_config.sealer(); }
    SecurityConfig const& securityConfig() const { return m_config.security(); }

    // the gateway configurations
    const std::string& p2pListenIP() const { return m_config.gateway().listenIP(); }
    uint16_t p2pListenPort() const { return m_config.gateway().listenPort(); }
    bool p2pSmSsl() const { return m_config.gateway().smSsl(); }
    const std::string& p2pNodeDir() const { return m_config.gateway().nodeDir(); }
    const std::string& p2pNodeFileName() const { return m_config.gateway().nodeFileName(); }

    // config for cert
    void setCertPath(const std::string& _certPath)
    {
        m_config.mutableCert().setCertPath(_certPath);
    }

    void setCaCert(const std::string& _caCert)
    {
        m_config.mutableCert().setCaCert(_caCert);
    }

    void setNodeCert(const std::string& _nodeCert)
    {
        m_config.mutableCert().setNodeCert(_nodeCert);
    }

    void setNodeKey(const std::string& _nodeKey)
    {
        m_config.mutableCert().setNodeKey(_nodeKey);
    }

    void setSmCaCert(const std::string& _smCaCert)
    {
        m_config.mutableCert().setSmCaCert(_smCaCert);
    }

    void setSmNodeCert(const std::string& _smNodeCert)
    {
        m_config.mutableCert().setSmNodeCert(_smNodeCert);
    }

    void setSmNodeKey(const std::string& _smNodeKey)
    {
        m_config.mutableCert().setSmNodeKey(_smNodeKey);
    }

    void setEnSmNodeCert(const std::string& _enSmNodeCert)
    {
        m_config.mutableCert().setEnSmNodeCert(_enSmNodeCert);
    }

    void setEnSmNodeKey(const std::string& _enSmNodeKey)
    {
        m_config.mutableCert().setEnSmNodeKey(_enSmNodeKey);
    }

    bool enableLRUCacheStorage() const { return m_config.storage().enableLRUCacheStorage(); }
    ssize_t cacheSize() const { return m_config.storage().cacheSize(); }

    std::string const& memberID() const { return m_config.failOver().memberID(); }
    unsigned leaseTTL() const { return m_config.failOver().leaseTTL(); }
    bool enableFailOver() const { return m_config.failOver().enable(); }
    std::string const& failOverClusterUrl() const { return m_config.failOver().clusterUrl(); }

    bool enableSendBlockStatusByTree() const
    {
        return m_config.sync().enableSendBlockStatusByTree();
    }
    bool enableSendTxByTree() const { return m_config.sync().enableSendTxByTree(); }
    std::int64_t treeWidth() const { return m_config.sync().treeWidth(); }

    int sendTxTimeout() const { return m_config.others().sendTxTimeout(); }

    bool withoutTarsFramework() const { return m_config.service().withoutTarsFramework(); }
    void setWithoutTarsFramework(bool _withoutTarsFramework)
    {
        m_config.mutableService().setWithoutTarsFramework(_withoutTarsFramework);
    }

    using BaselineSchedulerConfig = bcos::tool::BaselineSchedulerConfig;
    BaselineSchedulerConfig const& baselineSchedulerConfig() const
    {
        return m_config.others().baselineSchedulerConfig();
    }

    using TarsRPCConfig = bcos::tool::TarsRPCConfig;
    TarsRPCConfig const& tarsRPCConfig() const { return m_config.others().tarsRPCConfig(); }

    ledger::GenesisConfig const& genesisConfig() const;

    static bool isValidPort(int port);

    bool enableTxsFromFreeNode() const { return m_config.txPool().enableTxsFromFreeNode(); }
    int executorVersion() const;

protected:
    virtual void loadChainConfig(boost::property_tree::ptree const& _pt, bool _enforceGroupId);
    virtual void loadWeb3ChainConfig(boost::property_tree::ptree const& _pt);
    virtual void loadRpcConfig(boost::property_tree::ptree const& _pt);
    virtual void loadWeb3RpcConfig(boost::property_tree::ptree const& _pt);
    virtual void loadGatewayConfig(boost::property_tree::ptree const& _pt);
    virtual void loadCertConfig(boost::property_tree::ptree const& _pt);
    virtual void loadTxPoolConfig(boost::property_tree::ptree const& _pt);
    virtual void loadSecurityConfig(boost::property_tree::ptree const& _pt);
    virtual void loadSealerConfig(boost::property_tree::ptree const& _pt);
    virtual void loadStorageSecurityConfig(boost::property_tree::ptree const& _pt);
    virtual void loadSyncConfig(boost::property_tree::ptree const& _pt);

    virtual void loadStorageConfig(boost::property_tree::ptree const& _pt);
    virtual void loadConsensusConfig(boost::property_tree::ptree const& _pt);

    virtual void loadFailOverConfig(
        boost::property_tree::ptree const& _pt, bool _enforceMemberID = true);
    virtual void loadOthersConfig(boost::property_tree::ptree const& _pt);

    virtual void loadLedgerConfig(boost::property_tree::ptree const& _genesisConfig);

    // load config.genesis
    void loadExecutorConfig(boost::property_tree::ptree const& _pt);

    // load config.ini
    void loadExecutorNormalConfig(boost::property_tree::ptree const& _pt);

private:
    void loadGenesisFeatures(boost::property_tree::ptree const& ptree);

    bcos::crypto::KeyFactory::Ptr m_keyFactory;
    // chain configuration
    size_t m_blockLimit{};

    // for security
    // ledger configuration
    bcos::ledger::LedgerConfig::Ptr m_ledgerConfig;
    std::string m_genesisData;

    // Genesis config
    ledger::GenesisConfig m_genesisConfig;

    ConfigAggregate m_config;
};

std::string generateGenesisData(
    ledger::GenesisConfig const& genesisConfig, ledger::LedgerConfig const& ledgerConfig);

}  // namespace bcos::tool
