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
 * @file SealingManager.cpp
 * @author: yujiechen
 * @date: 2021-05-14
 */
#include "SealingManager.h"
#include "Common.h"
#include "Sealer.h"

using namespace bcos;
using namespace bcos::sealer;
using namespace bcos::crypto;
using namespace bcos::protocol;

void SealingManager::resetSealing()
{
    m_sealingVars.set([this](auto& vars) {
        SEAL_LOG(INFO) << LOG_DESC("resetSealing") << LOG_KV("startNum", vars.m_startSealingNumber)
                       << LOG_KV("endNum", vars.m_endSealingNumber)
                       << LOG_KV("sealingNum", vars.m_sealingNumber)
                       << LOG_KV("pendingTxs", pendingTxsSize());
        vars.m_sealingNumber = vars.m_endSealingNumber + 1;
    });
    clearPendingTxs();
}

void SealingManager::appendTransactions(
    TxsMetaDataQueue& _txsQueue, bcos::protocol::Block const& _fetchedTxs)
{
    WriteGuard lock(x_pendingTxs);
    // append the system transactions
    for (size_t i = 0; i < _fetchedTxs.transactionsMetaDataSize(); i++)
    {
        _txsQueue.emplace_back(
            std::const_pointer_cast<TransactionMetaData>(_fetchedTxs.transactionMetaData(i)));
    }
    m_onReady();
}

void SealingManager::clearPendingTxs()
{
    UpgradableGuard lock(x_pendingTxs);
    auto pendingTxsSize = m_pendingTxs.size() + m_pendingSysTxs.size();
    if (pendingTxsSize == 0)
    {
        return;
    }
    // return the txs back to the txpool
    SEAL_LOG(INFO) << LOG_DESC("clearPendingTxs: return back the unhandled transactions")
                   << LOG_KV("size", pendingTxsSize);
    auto unHandledTxs =
        ::ranges::views::concat(m_pendingTxs, m_pendingSysTxs) |
        ::ranges::views::transform([](auto& transaction) { return transaction->hash(); }) |
        ::ranges::to<std::vector>();
    try
    {
        notifyResetTxsFlag(unHandledTxs, false);
    }
    catch (std::exception const& e)
    {
        SEAL_LOG(WARNING) << LOG_DESC("clearPendingTxs: return back the unhandled txs exception")
                          << LOG_KV("message", boost::diagnostic_information(e));
    }
    UpgradeGuard ul(lock);
    m_pendingTxs.clear();
    m_pendingSysTxs.clear();
}

void SealingManager::notifyResetTxsFlag(const HashList& _txsHashList, bool _flag, size_t _retryTime)
{
    m_config->txpool()->asyncMarkTxs(_txsHashList, _flag, -1, HashType(),
        [this, _txsHashList, _flag, _retryTime](Error::Ptr _error) {
            if (_error == nullptr)
            {
                return;
            }
            SEAL_LOG(WARNING) << LOG_DESC("asyncMarkTxs failed, retry now");
            if (_retryTime >= 3)
            {
                return;
            }
            this->notifyResetTxsFlag(_txsHashList, _flag, _retryTime + 1);
        });
}

void SealingManager::notifyResetProposal(bcos::protocol::Block const& _block)
{
    auto txsHashList = ::ranges::to<std::vector>(_block.transactionHashes());
    notifyResetTxsFlag(txsHashList, false);
}

std::pair<bool, bcos::protocol::Block::Ptr> SealingManager::generateProposal(
    std::function<uint16_t(bcos::protocol::Block::Ptr)> _handleBlockHook)
{
    WriteGuard writeLock(x_pendingTxs);

    auto block = m_config->blockFactory()->createBlock();
    auto blockHeader = m_config->blockFactory()->blockHeaderFactory()->createBlockHeader();
    auto txsSize =
        std::min((size_t)m_maxTxsPerBlock, (m_pendingTxs.size() + m_pendingSysTxs.size()));
    auto systemTxsSize = std::min(txsSize, m_pendingSysTxs.size());
    m_sealingVars.set([&](auto& vars) {
        vars.m_sealingNumber = std::max(vars.m_sealingNumber, vars.m_latestNumber + 1);
        blockHeader->setNumber(vars.m_sealingNumber);

        // prioritize seal from the system txs list
        if (!m_pendingSysTxs.empty())
        {
            vars.m_waitUntil = vars.m_sealingNumber;
            SEAL_LOG(INFO) << LOG_DESC("seal the system transactions")
                           << LOG_KV("sealNextBlockUntil", vars.m_waitUntil)
                           << LOG_KV("curNum", vars.m_latestNumber);
        }
    });
    blockHeader->setTimestamp(m_config->nodeTimeMaintenance()->getAlignedTime());
    blockHeader->calculateHash(*m_config->blockFactory()->cryptoSuite()->hashImpl());
    block->setBlockHeader(blockHeader);

    bool containSysTxs = false;
    if (_handleBlockHook)
    {
        // put the generated transaction into the 0th position of the block transactions
        // Note: must set generatedTx into the first transaction for other transactions may change
        //       the _sys_config_ and _sys_consensus_
        //       here must use noteChange for this function will notify updating the txsCache
        auto handleRet = _handleBlockHook(block);
        if (handleRet == Sealer::SealBlockResult::SUCCESS)
        {
            if (block->transactionsMetaDataSize() > 0 || block->transactionsSize() > 0)
            {
                containSysTxs = true;
                if (txsSize == m_maxTxsPerBlock)
                {
                    txsSize--;
                }
            }
        }
        else if (handleRet == Sealer::SealBlockResult::WAIT_FOR_LATEST_BLOCK)
        {
            m_sealingVars.set([&](auto& vars) {
                SEAL_LOG(INFO) << LOG_DESC(
                                      "seal the rotate transactions, but not update latest block")
                               << LOG_KV("sealNextBlockUntil", vars.m_waitUntil)
                               << LOG_KV("curNum", vars.m_latestNumber);
                vars.m_waitUntil = vars.m_sealingNumber - 1;
            });
            return {false, nullptr};
        }
    }
    for (size_t i = 0; i < systemTxsSize; i++)
    {
        block->appendTransactionMetaData(std::move(m_pendingSysTxs.front()));
        m_pendingSysTxs.pop_front();
        containSysTxs = true;
    }
    for (size_t i = systemTxsSize; i < txsSize; i++)
    {
        block->appendTransactionMetaData(std::move(m_pendingTxs.front()));
        m_pendingTxs.pop_front();
    }

    m_sealingVars.set([&](auto& vars) {
        vars.m_sealingNumber++;
        vars.m_lastSealTime = utcSteadyTime();
    });
    // Note: When the last block(N) sealed by this node contains system transactions,
    //       if other nodes do not wait until block(N) is committed and directly seal block(N+1),
    //       will cause system exceptions.
    if (c_fileLogLevel == TRACE)
    {
        SEAL_LOG(TRACE) << LOG_DESC("generateProposal")
                        << LOG_KV("txsSize", block->transactionsMetaDataSize())
                        << LOG_KV("sysTxsSize", systemTxsSize)
                        << LOG_KV("containSysTxs", containSysTxs)
                        << LOG_KV("pendingSize", m_pendingTxs.size())
                        << LOG_KV("pendingSysSize", m_pendingSysTxs.size());
    }
    return {containSysTxs, block};
}

size_t SealingManager::pendingTxsSize()
{
    ReadGuard l(x_pendingTxs);
    return m_pendingSysTxs.size() + m_pendingTxs.size();
}

bool SealingManager::reachMinSealTimeCondition()
{
    bool result;
    m_sealingVars.get([&](auto& vars) {
        result = !((utcSteadyTime() - vars.m_lastSealTime) < m_config->minSealTime());
    });
    return result;
}

int64_t SealingManager::txsSizeExpectedToFetch()
{
    int64_t result;
    m_sealingVars.get([&](auto& vars) {
        auto txsSizeToFetch =
            (vars.m_endSealingNumber - vars.m_sealingNumber + 1) * m_maxTxsPerBlock;
        auto txsSize = pendingTxsSize();
        if (txsSizeToFetch <= txsSize)
        {
            result = 0;
        }
        else
        {
            result = txsSizeToFetch - txsSize;
        }
    });
    return result;
}

bcos::sealer::SealingManager::FetchResult SealingManager::fetchTransactions()
{
    // fetching transactions currently
    auto lock = std::unique_lock{m_fetchingTxsMutex, std::try_to_lock};
    if (!lock.owns_lock())
    {
        return FetchResult::NOT_READY;
    }

    ssize_t startSealingNumber = 0;
    ssize_t endSealingNumber = 0;
    ssize_t sealingNumber = 0;

    m_sealingVars.get([&](auto& vars) {
        sealingNumber = vars.m_sealingNumber;
        startSealingNumber = vars.m_startSealingNumber;
        endSealingNumber = vars.m_endSealingNumber;
    });
    // no need to sealing
    if (sealingNumber < startSealingNumber || sealingNumber > endSealingNumber)
    {
        return FetchResult::NOT_READY;
    }

    auto txsToFetch = txsSizeExpectedToFetch();
    if (txsToFetch == 0)
    {
        return FetchResult::NOT_READY;
    }

    try
    {
        auto [_txsHashList, _sysTxsList] = m_config->txpool()->sealTxs(txsToFetch, nullptr);
        if (_txsHashList->transactionsHashSize() == 0 && _sysTxsList->transactionsHashSize() == 0)
        {
            return FetchResult::NO_TRANSACTION;
        }

        bool abort = true;
        if ((sealingNumber >= startSealingNumber) && (sealingNumber <= endSealingNumber))
        {
            appendTransactions(m_pendingTxs, *_txsHashList);
            appendTransactions(m_pendingSysTxs, *_sysTxsList);
            abort = false;
        }
        else
        {
            SEAL_LOG(INFO) << LOG_DESC("fetchTransactions finish: abort the expired txs")
                           << LOG_KV("txsSize", _txsHashList->transactionsMetaDataSize())
                           << LOG_KV("sysTxsSize", _sysTxsList->transactionsMetaDataSize());
            // Note: should reset the aborted txs
            notifyResetProposal(*_txsHashList);
            notifyResetProposal(*_sysTxsList);
        }

        m_onReady();
        SEAL_LOG(DEBUG) << LOG_DESC("fetchTransactions finish")
                        << LOG_KV("txsSize", _txsHashList->transactionsMetaDataSize())
                        << LOG_KV("sysTxsSize", _sysTxsList->transactionsMetaDataSize())
                        << LOG_KV("pendingSize", m_pendingTxs.size())
                        << LOG_KV("pendingSysSize", m_pendingSysTxs.size())
                        << LOG_KV("startSealingNumber", startSealingNumber)
                        << LOG_KV("endSealingNumber", endSealingNumber)
                        << LOG_KV("sealingNumber", sealingNumber) << LOG_KV("abort", abort);
    }
    catch (std::exception const& e)
    {
        SEAL_LOG(WARNING) << LOG_DESC("fetchTransactions: onRecv sealed txs failed")
                          << LOG_KV("message", boost::diagnostic_information(e));
    }
    return FetchResult::SUCCESS;
}

bcos::sealer::SealingManager::SealingManager(SealerConfig::Ptr _config)
  : m_config(std::move(_config))
{}

void bcos::sealer::SealingManager::resetSealingInfo(
    ssize_t _startSealingNumber, ssize_t _endSealingNumber, size_t _maxTxsPerBlock)
{
    if (_startSealingNumber > _endSealingNumber)
    {
        return;
    }
    // non-continuous sealing request
    m_sealingVars.set([&](auto& vars) {
        if (vars.m_sealingNumber > vars.m_endSealingNumber ||
            _startSealingNumber != (vars.m_endSealingNumber + 1))
        {
            clearPendingTxs();
            vars.m_startSealingNumber = _startSealingNumber;
            vars.m_sealingNumber = _startSealingNumber;
            vars.m_lastSealTime = utcSteadyTime();
            // for sys block
            if (vars.m_waitUntil >= vars.m_startSealingNumber)
            {
                SEAL_LOG(INFO) << LOG_DESC("resetSealingInfo: reset waitUntil for reseal");
                vars.m_waitUntil = vars.m_startSealingNumber - 1;
            }
        }
        vars.m_endSealingNumber = _endSealingNumber;

        m_maxTxsPerBlock = _maxTxsPerBlock;
        m_onReady();
        SEAL_LOG(INFO) << LOG_DESC("resetSealingInfo") << LOG_KV("start", vars.m_startSealingNumber)
                       << LOG_KV("end", vars.m_endSealingNumber)
                       << LOG_KV("sealingNumber", vars.m_sealingNumber)
                       << LOG_KV("waitUntil", vars.m_waitUntil);
    });
}

void bcos::sealer::SealingManager::resetLatestNumber(int64_t _latestNumber)
{
    m_sealingVars.set([&](auto& vars) { vars.m_latestNumber = _latestNumber; });
}

void bcos::sealer::SealingManager::resetLatestHash(crypto::HashType _latestHash)
{
    m_latestHash = _latestHash;
}

int64_t bcos::sealer::SealingManager::latestNumber() const
{
    int64_t latestNumber = 0;
    m_sealingVars.get([&](auto& vars) { latestNumber = vars.m_latestNumber; });
    return latestNumber;
}

bcos::crypto::HashType bcos::sealer::SealingManager::latestHash() const
{
    return m_latestHash;
}

bool bcos::sealer::SealingVars::waitForShouldGenerateProposal()
{
    std::unique_lock lock(m_mutex);
    bool result = false;
    m_condition.wait_for(lock, std::chrono::seconds(1), [&]() {
        result = shouldGenerateProposal();
        return result;
    });
    return result;
}

bool bcos::sealer::SealingVars::shouldGenerateProposal()
{
    std::unique_lock lock(m_mutex);
    if (m_sealingNumber < m_startSealingNumber || m_sealingNumber > m_endSealingNumber)
    {
        return false;
    }
    // should wait the given block submit to the ledger
    if (m_latestNumber < m_waitUntil)
    {
        return false;
    }

    return true;
    // check the txs size
    // auto txsSize = pendingTxsSize();
    // return (txsSize >= m_generateProposalVars.m_maxTxsPerBlock) ||
    // reachMinSealTimeCondition();
}

bool bcos::sealer::SealingManager::shouldGenerateProposal()
{
    return m_sealingVars.waitForShouldGenerateProposal();
}
