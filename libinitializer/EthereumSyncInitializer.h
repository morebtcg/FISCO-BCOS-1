/**
 *  Copyright (C) 2026 FISCO BCOS.
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
 * @file EthereumSyncInitializer.h
 * @brief Ethereum L1 EL-mode self-sync driver: connect to RLPx bootnodes, download
 *        blocks with BlockExchange (PoS header validation), verify + commit each block
 *        with EthereumBlockVerifier (execute -> roots -> MPT state root -> ledger), in
 *        a dedicated background thread. Wired from AirNodeInitializer when
 *        [ethereum] mode=el; the config file is the source of truth.
 * @date 2026/8/19
 */
#pragma once

#include "libinitializer/Common.h"
#include "libinitializer/GlobalStateStorageInitializer.h"
#include "bcos-devp2p/eth/ForkId.h"
#include "bcos-devp2p/rlpx/Client.h"
#include "bcos-devp2p/sync/BlockExchange.h"
#include "bcos-devp2p/sync/Bootnodes.h"
#include "bcos-devp2p/sync/HeaderValidator.h"
#include "bcos-framework/ledger/LedgerInterface.h"
#include "bcos-framework/ledger/LedgerTypeDef.h"
#include "bcos-framework/protocol/BlockFactory.h"
#include "bcos-ledger/LedgerMethods.h"
#include "bcos-tool/NodeConfig.h"
#include "bcos-transaction-scheduler/EthereumBlockVerifier.h"
#include "bcos-transaction-scheduler/SchedulerSerialImpl.h"
#include "bcos-rlp-protocol/Web3Transaction.h"
#include "bcos-tars-protocol/protocol/TransactionImpl.h"  // complete type for shared_ptr upcast in decodeRaw()
#include "bcos-task/Wait.h"
#include "ethereum-executor/EthereumExecutor.h"
#include <boost/throw_exception.hpp>
#include <atomic>
#include <chrono>
#include <ctime>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace bcos::initializer
{

/// Self-contained Ethereum L1 EL-mode sync client. Owns a background thread that
/// repeatedly: (1) loads the bootnode list, (2) connects to each in turn, (3)
/// downloads blocks from the local head onward and verifies + commits each through
/// EthereumBlockVerifier, and (4) loops forever (catching transient network errors).
class EthereumSyncInitializer
{
public:
    // Sepolia's merge (terminal total difficulty) block — the only block-based EL
    // fork on Sepolia; all later forks are timestamp-based. Used by computeForkId().
    static constexpr uint64_t c_sepoliaMergeBlock = 1735371;
    // _globalStateStorage: production MultiLayerStorage (GlobalStateStorage).
    EthereumSyncInitializer(bcos::tool::NodeConfig::Ptr _nodeConfig,
        bcos::ledger::LedgerInterface::Ptr _ledger,
        bcos::protocol::BlockFactory::Ptr _blockFactory,
        std::shared_ptr<scheduler_v1::SchedulerSerialImpl> _scheduler,
        std::shared_ptr<executor_v1::eth::EthereumExecutor> _executor,
        GlobalStateStorageInitializer::Ptr _globalStateStorageInitializer,
        bcos::IOServicePool::Ptr _ioServicePool)
      : m_nodeConfig(std::move(_nodeConfig)),
        m_ledger(std::move(_ledger)),
        m_blockFactory(std::move(_blockFactory)),
        m_scheduler(std::move(_scheduler)),
        m_executor(std::move(_executor)),
        m_globalStateStorageInitializer(std::move(_globalStateStorageInitializer)),
        m_ioServicePool(std::move(_ioServicePool))
    {}

    ~EthereumSyncInitializer() { stop(); }

    EthereumSyncInitializer(EthereumSyncInitializer const&) = delete;
    EthereumSyncInitializer& operator=(EthereumSyncInitializer const&) = delete;

    /// Validate that the EL-mode prerequisites hold (executor v2, fork schedule, bootnode
    /// file readable, genesis anchor present). Throws InvalidConfig on failure.
    void validateConfig() const
    {
        if (!m_nodeConfig->ethereumELModeEnabled())
        {
            BOOST_THROW_EXCEPTION(bcos::tool::InvalidConfig() << bcos::errinfo_comment(
                                      "EthereumSyncInitializer: [ethereum].mode != el"));
        }
        if (m_nodeConfig->executorVersion() < ledger::ETHEREUM_EXECUTOR_VERSION)
        {
            BOOST_THROW_EXCEPTION(bcos::tool::InvalidConfig() << bcos::errinfo_comment(
                                      "Ethereum L1 EL mode requires executor.version >= 2 "
                                      "(the pure-Ethereum executor); set [executor] version=2 "
                                      "in config.genesis"));
        }
        if (!m_nodeConfig->genesisConfig().m_ethGenesisHeader.has_value())
        {
            BOOST_THROW_EXCEPTION(bcos::tool::InvalidConfig() << bcos::errinfo_comment(
                                      "Ethereum L1 EL mode requires an [eth_genesis_header] "
                                      "section in config.genesis (sync anchor)"));
        }
        // Bootnode file must exist and parse (validates the enode list eagerly).
        auto nodes = bcos::devp2p::sync::loadBootnodes(m_nodeConfig->ethereumBootnodesFile());
        if (nodes.empty())
        {
            BOOST_THROW_EXCEPTION(bcos::tool::InvalidConfig() << bcos::errinfo_comment(
                                      "Ethereum L1 EL mode: no bootnodes in " +
                                      m_nodeConfig->ethereumBootnodesFile()));
        }
    }

    /// Start the background sync loop. No-op if already started.
    void start()
    {
        if (m_running.exchange(true))
        {
            return;
        }
        INITIALIZER_LOG(INFO) << LOG_DESC("EL sync: starting self-sync loop")
                              << LOG_KV("bootnodes", m_nodeConfig->ethereumBootnodesFile())
                              << LOG_KV("listenPort", m_nodeConfig->ethereumListenPort())
                              << LOG_KV("maxBatch", m_nodeConfig->ethereumMaxBatchSize());
        m_thread = std::thread([this]() { syncLoop(); });
    }

    /// Stop the background thread and join it.
    void stop()
    {
        if (!m_running.exchange(false))
        {
            return;
        }
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    bool running() const { return m_running.load(); }

private:
    /// The chain-genesis anchor header, from [eth_genesis_header]. Used as the download
    /// anchor on a fresh node and pinned as the RLPx handshake genesisHash on every
    /// resume. Fork-gated fields copy through only when the genesis header carries them,
    /// so the anchor re-encodes to the byte-exact genesis RLP.
    bcos::protocol::EthBlockHeaderData genesisAnchorHeader() const
    {
        auto const& genesis = m_nodeConfig->genesisConfig().m_ethGenesisHeader.value();
        bcos::protocol::EthBlockHeaderData h;
        h.parentInfo.blockHash = genesis.m_parentHash;
        h.uncleHash = genesis.m_sha3Uncles;
        h.stateRoot = genesis.m_stateRoot;
        h.txsRoot = genesis.m_transactionsRoot;
        h.receiptsRoot = genesis.m_receiptsRoot;
        std::copy(genesis.m_logsBloom.begin(), genesis.m_logsBloom.end(), h.logsBloom.begin());
        h.difficulty = genesis.m_difficulty;
        h.gasLimit = genesis.m_gasLimit;
        h.gasUsed = genesis.m_gasUsed;
        h.number = 0;
        h.timestamp = genesis.m_timestamp;
        h.extraData = genesis.m_extraData;
        std::copy(genesis.m_mixHash.begin(), genesis.m_mixHash.end(), h.prevRandao.begin());
        std::copy(genesis.m_nonce.begin(), genesis.m_nonce.end(), h.nonce.begin());
        h.coinbase = genesis.m_miner;
        // Fork-gated fields: copy through only the ones the genesis header
        // actually carries (nullopt stays nullopt), so the anchor re-encodes
        // to the same byte-exact RLP as the committed genesis block.
        h.baseFee = genesis.m_baseFeePerGas;
        h.withdrawalsHash = genesis.m_withdrawalsRoot;
        h.blobGasUsed = genesis.m_blobGasUsed;
        h.excessBlobGas = genesis.m_excessBlobGas;
        h.parentBeaconRoot = genesis.m_parentBeaconBlockRoot;
        h.requestsHash = genesis.m_requestsHash;
        return h;
    }

    /// Resume point computed once per sync round: where to start downloading and
    /// which header anchors the download. A fresh ledger (only the genesis block)
    /// starts at 1 anchored on the genesis header; a ledger that already has blocks
    /// resumes from localHead + 1 anchored on the local head header (checkpoint
    /// resume). genesisHeader is always the chain genesis — it is what the RLPx
    /// handshake pins as the Status genesisHash, independent of the resume point.
    struct ResumePoint
    {
        uint64_t startNumber;
        bcos::protocol::EthBlockHeaderData anchor;        // local head (or genesis)
        bcos::protocol::EthBlockHeaderData prevHeader;    // parent for the first download
        bcos::protocol::EthBlockHeaderData genesisHeader; // chain genesis (handshake pin)
    };

    ResumePoint resumePoint() const
    {
        auto genesisHeader = genesisAnchorHeader();
        auto current = task::syncWait(ledger::getCurrentBlockNumber(*m_ledger));
        if (current <= 0)
        {
            // Fresh node: only the genesis block exists; start downloading at 1.
            return {1, genesisHeader, genesisHeader, genesisHeader};
        }
        // Resume from the local head: anchor = local head header, start at head + 1.
        auto headBlock = task::syncWait(
            ledger::getBlockData(*m_ledger, current, bcos::ledger::HEADER));
        if (!headBlock || !headBlock->blockHeader())
        {
            BOOST_THROW_EXCEPTION(std::runtime_error(
                "EL sync: cannot read local head block " + std::to_string(current) +
                " for resume"));
        }
        // Convert the stored Tars header back to the Ethereum header domain. The
        // EthBlockHeader constructor copies the fork-gated optionals only when the
        // stored header carries them, so the anchor re-encodes to the same RLP that
        // was committed — byte-exact resume.
        bcos::protocol::EthBlockHeader localHead(*headBlock->blockHeader());
        auto head = localHead.data();
        // The execution path (makeExecutionBlockHeader) stores the Tars timestamp in
        // FISCO milliseconds (the EVM divides by 1000), but the Ethereum header domain
        // — and therefore the resume anchor's RLP re-encoding — is seconds. The
        // EthBlockHeader constructor above already converts (m_data.timestamp =
        // ms / 1000), so `head.timestamp` is wire seconds here — do NOT divide again
        // or the anchor re-encodes to a wrong RLP and the resume parent-hash check
        // fails with "parent hash mismatch (fork or reorg)".
        INITIALIZER_LOG(INFO) << LOG_DESC("EL sync: resuming from local head")
                              << LOG_KV("headNumber", current)
                              << LOG_KV("headHash", anchorHeaderHash(head).hex().substr(0, 18))
                              << LOG_KV("resumeFrom", current + 1);
        return {
            static_cast<uint64_t>(current + 1), head, head, genesisHeader};
    }

    bcos::devp2p::sync::ChainConfig devp2pChainConfig() const
    {
        bcos::devp2p::sync::ChainConfig config;
        config.chainId = m_nodeConfig->ethereumChainId();
        config.londonTime = m_nodeConfig->ethereumForkLondonTime();
        config.shanghaiTime = m_nodeConfig->ethereumForkShanghaiTime();
        config.cancunTime = m_nodeConfig->ethereumForkCancunTime();
        config.pragueTime = m_nodeConfig->ethereumForkPragueTime();
        // Blocks before the merge are PoW (non-zero difficulty, ommers allowed);
        // from the merge block onward the chain is PoS.
        config.mergeBlock = c_sepoliaMergeBlock;
        return config;
    }

    /// EIP-2124 fork-id announced in the eth Status handshake. The checksum chains
    /// the genesis hash with every fork point that the LOCAL chain has already passed
    /// (crc32 over the 8-byte big-endian fork value, exactly geth's checksumUpdate),
    /// and next is the first fork point the local chain has NOT passed yet (or 0 if
    /// all known forks are active). This mirrors geth's forkid.NewID(config, genesis,
    /// localHead.Number, localHead.Time) — the fork-id reflects the local head, NOT
    /// wall-clock time. A fresh node (head = genesis) therefore announces
    /// {crc32(genesisHash), mergeBlock}, which every remote accepts via EIP-2124
    /// rule #2 (our checksum is the remote's genesis-sum and next matches its first
    /// fork) — unlike a wall-clock-derived all-forks checksum, which older remotes
    /// (e.g. geth 1.14.x, which only knows forks up to Cancun) reject outright.
    bcos::devp2p::eth::ForkId computeForkId(uint64_t _localHeadNumber, uint64_t _localHeadTime) const
    {
        auto const& genesis = m_nodeConfig->genesisConfig().m_ethGenesisHeader.value();
        uint32_t hash = bcos::devp2p::eth::crc32(
            bcos::bytesConstRef(genesis.m_hash.data(), genesis.m_hash.size()));
        // Sepolia's only block-based EL fork: the merge (terminal total difficulty)
        // block. (London is active at genesis and is skipped by geth's gatherForks.)
        if (c_sepoliaMergeBlock <= _localHeadNumber)
        {
            hash = bcos::devp2p::eth::forkIdAddForkPoint(hash, c_sepoliaMergeBlock);
        }
        else
        {
            // Merge not yet passed locally (fresh node): announce it as next.
            return {hash, c_sepoliaMergeBlock};
        }
        // Timestamp-based forks, chained in activation order; only chain in the ones
        // the local head has passed, and announce the first not-yet-passed one.
        auto addIfPassed = [&](uint64_t fork) {
            if (fork > 0 && fork <= _localHeadTime)
            {
                hash = bcos::devp2p::eth::forkIdAddForkPoint(hash, fork);
                return true;
            }
            return false;
        };
        for (uint64_t fork : {m_nodeConfig->ethereumForkShanghaiTime(),
                 m_nodeConfig->ethereumForkCancunTime(),
                 m_nodeConfig->ethereumForkPragueTime(),
                 m_nodeConfig->ethereumForkOsakaTime(),
                 m_nodeConfig->ethereumForkBpo1Time(),
                 m_nodeConfig->ethereumForkBpo2Time()})
        {
            if (!addIfPassed(fork))
            {
                if (fork > 0)
                {
                    return {hash, fork};
                }
            }
        }
        return {hash, 0};
    }

    scheduler_v1::EvmcForkTimestamps evmcForkSchedule() const
    {
        scheduler_v1::EvmcForkTimestamps forks;
        forks.londonTime = m_nodeConfig->ethereumForkLondonTime();
        // Paris (The Merge): timestamp from [fork_timestamps] paris_time. Chains
        // with a PoW phase (Sepolia) must set it (1661128380) so pre-merge blocks
        // run at LONDON (DIFFICULTY semantics); pure-PoS chains omit it (0 =
        // active from genesis).
        forks.parisTime = m_nodeConfig->ethereumForkParisTime();
        forks.shanghaiTime = m_nodeConfig->ethereumForkShanghaiTime();
        forks.cancunTime = m_nodeConfig->ethereumForkCancunTime();
        forks.pragueTime = m_nodeConfig->ethereumForkPragueTime();
        forks.osakaTime = m_nodeConfig->ethereumForkOsakaTime();
        return forks;
    }

    /// The shared raw->Transaction decoder (eth_sendRawTransaction / devp2p / verifier all
    /// use the same decodeWeb3RawTransaction path).
    bcos::protocol::Transaction::Ptr decodeRaw(bcos::bytes const& raw) const
    {
        auto cryptoSuite = m_blockFactory->cryptoSuite();
        return bcos::rpc::decodeWeb3RawTransaction(
            bcos::bytesConstRef(raw.data(), raw.size()), *cryptoSuite->hashImpl());
    }

    void syncLoop()
    {
        // The verifier runs on the shared v2 scheduler + EthereumExecutor.
        using Verifier = bcos::scheduler_v1::EthereumBlockVerifier<scheduler_v1::SchedulerSerialImpl,
            executor_v1::eth::EthereumExecutor>;
        Verifier verifier(*m_scheduler, *m_executor, *m_blockFactory);
        auto forks = evmcForkSchedule();
        auto chainId = m_nodeConfig->ethereumChainId();
        // v2 always computes the MPT state root itself; the injected calculator must never run.
        using ViewType = GlobalStateStorage::ViewType;
        typename Verifier::template StateRootCalculator<ViewType> stateRootCalc =
            [](ViewType&, uint32_t) -> task::Task<bcos::crypto::HashType> {
            BOOST_THROW_EXCEPTION(std::runtime_error{
                "legacy state-root fold must not run for executor v2 (Ethereum L1 EL mode)"});
        };

        // The node's own identity for the RLPx handshake. Deterministic from the configured
        // node key file when present; otherwise a fresh keypair (bootnodes authenticate us by
        // our public key, so a stable key is strongly recommended).
        bcos::devp2p::rlpx::EccKeyPair localKey;
        // TODO(el): load from m_nodeConfig->ethereumNodeKeyFile() when non-empty.

        while (m_running.load())
        {
            try
            {
                // Compute the resume point fresh on EVERY round (not once per process):
                // the previous round may have committed blocks, so the next round must
                // resume from the new local head instead of re-downloading what we
                // already have. The chain genesis is pinned separately for the RLPx
                // handshake — it must never change.
                auto resume = resumePoint();
                auto const& anchor = resume.anchor;
                auto const& genesisHeader = resume.genesisHeader;
                auto devp2pConfig = devp2pChainConfig();
                auto bootnodes =
                    bcos::devp2p::sync::loadBootnodes(m_nodeConfig->ethereumBootnodesFile());
                for (auto const& peer : bootnodes)
                {
                    if (!m_running.load())
                    {
                        return;
                    }
                    // A single unreachable bootnode must not stall the round: each peer's
                    // connect + download is fault-isolated so the loop moves on to the next
                    // bootnode (online fallback) and retries the whole list next round.
                    try
                    {
                        // Fill the Status/fork-id fields the peer expects. genesisHash is
                        // ALWAYS the chain genesis (the handshake rejects a peer on a
                        // different chain), headHash reflects the local resume anchor.
                        auto clientConfig = peer;
                        clientConfig.clientId = "FISCO-BCOS-EL/v0.1.0";
                        clientConfig.networkId = chainId;
                        clientConfig.genesisHash = anchorHeaderHash(genesisHeader);
                        clientConfig.headHash = anchorHeaderHash(anchor);
                        // totalDifficulty: minimal big-endian u256(0). An EMPTY byte
                        // string RLP-encodes as 0x80 (the canonical RLP integer 0); a
                        // single {0} would encode as 0x00 (non-canonical, rejected by
                        // strict peers). The peer does not use TD for fork choice here.
                        clientConfig.totalDifficulty = {};
                        // EIP-2124 fork-id: mirrors geth's forkid.NewID over the
                        // LOCAL head (genesis on a fresh node), so any Sepolia node
                        // (old or new) accepts us (see computeForkId).
                        clientConfig.forkId = computeForkId(
                            static_cast<uint64_t>(resume.anchor.number),
                            static_cast<uint64_t>(resume.anchor.timestamp));

                        bcos::devp2p::rlpx::RlpxClient client(localKey, clientConfig);
                        INITIALIZER_LOG(INFO)
                            << LOG_DESC("EL sync: connecting to bootnode")
                            << LOG_KV("host", peer.host) << LOG_KV("port", peer.port)
                            << LOG_KV("startNumber", resume.startNumber);
                        auto established = client.connect();
                        std::cerr << "[EL sync] handshake OK with " << peer.host
                                  << " (peerHead=" << established.peerStatus.headHash.hex().substr(0, 18)
                                  << ")" << std::endl;

                        // Download from the local head onward: startNumber = anchor + 1,
                        // anchor = local head header (genesis on a fresh node). Blocks are
                        // verified in strictly ascending order, which the verifier's
                        // incremental MPT requires — hence the resume point, never a jump.
                        bcos::devp2p::sync::BlockExchange exchange(
                            resume.startNumber, anchor, devp2pConfig,
                            m_nodeConfig->ethereumMaxBatchSize());
                        std::cerr << "[EL sync] starting download from "
                                  << resume.startNumber << " batch="
                                  << m_nodeConfig->ethereumMaxBatchSize() << std::endl;

                        auto prevHeader = resume.prevHeader;
                        exchange.downloadRange(established.session,
                            std::numeric_limits<uint64_t>::max(),
                            [&](bcos::devp2p::sync::Block const& block) {
                                if (!m_running.load())
                                {
                                    return;
                                }
                                auto result = task::syncWait(verifier.verifyAndCommit(
                                    m_globalStateStorageInitializer->storage(), *m_ledger,
                                    block.header, prevHeader, block.transactions,
                                    block.withdrawals, forks, chainId, block.uncles,
                                    c_sepoliaMergeBlock,
                                    [this](bcos::bytes const& raw) { return decodeRaw(raw); },
                                    stateRootCalc));
                                if (!result.valid)
                                {
                                    BOOST_THROW_EXCEPTION(std::runtime_error(
                                        "EL sync: block " +
                                        std::to_string(block.header.number) +
                                        " verification failed: " + result.error +
                                        " (computedStateRoot=" + result.stateRoot.hex() +
                                        " headerStateRoot=" + block.header.stateRoot.hex() +
                                        " coinbase=" + block.header.coinbase.hex() +
                                        " difficulty=" + block.header.difficulty.str() +
                                        " gasUsed=" + block.header.gasUsed.str() + ")"));
                                }
                                prevHeader = block.header;
                                INITIALIZER_LOG(INFO)
                                    << LOG_DESC("EL sync: committed block")
                                    << LOG_KV("number", block.header.number)
                                    << LOG_KV("hash", block.hash.hex().substr(0, 18))
                                    << LOG_KV("stateRoot",
                                        result.stateRoot.hex().substr(0, 18));
                            });
                    }
                    catch (std::exception const& e)
                    {
                        INITIALIZER_LOG(WARNING)
                            << LOG_DESC("EL sync: bootnode failed, trying next")
                            << LOG_KV("host", peer.host) << LOG_KV("port", peer.port)
                            << LOG_KV("error", e.what())
                            << LOG_KV("diag",
                                boost::current_exception_diagnostic_information());
                    }
                }
                // One full pass over the bootnode list: pause briefly before checking for
                // new blocks again (a successful download already advanced the local head;
                // the next round resumes from there).
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
            catch (std::exception const& e)
            {
                if (!m_running.load())
                {
                    return;
                }
                INITIALIZER_LOG(WARNING)
                    << LOG_DESC("EL sync: sync round failed, retrying")
                    << LOG_KV("error", e.what());
                // Back off briefly before retrying the next bootnode / round.
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        }
    }

    static bcos::h256 anchorHeaderHash(bcos::protocol::EthBlockHeaderData const& h)
    {
        bcos::bytes rlp;
        bcos::codec::rlp::encode(rlp, h);
        return bcos::crypto::keccak256Hash(bcos::bytesConstRef(rlp.data(), rlp.size()));
    }

    bcos::tool::NodeConfig::Ptr m_nodeConfig;
    bcos::ledger::LedgerInterface::Ptr m_ledger;
    bcos::protocol::BlockFactory::Ptr m_blockFactory;
    std::shared_ptr<scheduler_v1::SchedulerSerialImpl> m_scheduler;
    std::shared_ptr<executor_v1::eth::EthereumExecutor> m_executor;
    GlobalStateStorageInitializer::Ptr m_globalStateStorageInitializer;
    bcos::IOServicePool::Ptr m_ioServicePool;

    std::atomic_bool m_running{false};
    std::thread m_thread;
};

}  // namespace bcos::initializer
