#pragma once

#include "CertConfig.h"
#include "ChainConfig.h"
#include "ConsensusConfig.h"
#include "ExecutorConfig.h"
#include "FailOverConfig.h"
#include "GatewayConfig.h"
#include "LedgerParamConfig.h"
#include "OthersConfig.h"
#include "RpcConfig.h"
#include "SealerConfig.h"
#include "SecurityConfig.h"
#include "ServiceConfig.h"
#include "StorageConfig.h"
#include "SyncConfig.h"
#include "TxPoolConfig.h"
#include "Web3RpcConfig.h"

namespace bcos::tool
{
class ConfigAggregate
{
public:
    RpcConfig const& rpc() const { return m_rpc; }
    RpcConfig& mutableRpc() { return m_rpc; }

    StorageConfig const& storage() const { return m_storage; }
    StorageConfig& mutableStorage() { return m_storage; }

    ServiceConfig const& service() const { return m_service; }
    ServiceConfig& mutableService() { return m_service; }

    Web3RpcConfig const& web3Rpc() const { return m_web3Rpc; }
    Web3RpcConfig& mutableWeb3Rpc() { return m_web3Rpc; }

    GatewayConfig const& gateway() const { return m_gateway; }
    GatewayConfig& mutableGateway() { return m_gateway; }

    CertConfig const& cert() const { return m_cert; }
    CertConfig& mutableCert() { return m_cert; }

    ChainConfig const& chain() const { return m_chain; }
    ChainConfig& mutableChain() { return m_chain; }

    TxPoolConfig const& txPool() const { return m_txPool; }
    TxPoolConfig& mutableTxPool() { return m_txPool; }

    FailOverConfig const& failOver() const { return m_failOver; }
    FailOverConfig& mutableFailOver() { return m_failOver; }

    ConsensusConfig const& consensus() const { return m_consensus; }
    ConsensusConfig& mutableConsensus() { return m_consensus; }

    ExecutorConfig const& executor() const { return m_executor; }
    ExecutorConfig& mutableExecutor() { return m_executor; }

    LedgerParamConfig const& ledgerParam() const { return m_ledgerParam; }
    LedgerParamConfig& mutableLedgerParam() { return m_ledgerParam; }

    OthersConfig const& others() const { return m_others; }
    OthersConfig& mutableOthers() { return m_others; }

    SyncConfig const& sync() const { return m_sync; }
    SyncConfig& mutableSync() { return m_sync; }

    SealerConfig const& sealer() const { return m_sealer; }
    SealerConfig& mutableSealer() { return m_sealer; }

    SecurityConfig const& security() const { return m_security; }
    SecurityConfig& mutableSecurity() { return m_security; }

private:
    RpcConfig m_rpc;
    StorageConfig m_storage;
    ServiceConfig m_service;
    Web3RpcConfig m_web3Rpc;
    GatewayConfig m_gateway;
    CertConfig m_cert;
    ChainConfig m_chain;
    TxPoolConfig m_txPool;
    FailOverConfig m_failOver;
    ConsensusConfig m_consensus;
    ExecutorConfig m_executor;
    LedgerParamConfig m_ledgerParam;
    OthersConfig m_others;
    SyncConfig m_sync;
    SealerConfig m_sealer;
    SecurityConfig m_security;
};
}  // namespace bcos::tool