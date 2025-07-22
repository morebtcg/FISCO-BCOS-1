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
 * @file SealingManager.h
 * @author: yujiechen
 * @date: 2021-05-14
 */
#pragma once
#include "SealerConfig.h"
#include "bcos-framework/protocol/TransactionMetaData.h"
#include <bcos-utilities/CallbackCollectionHandler.h>
#include <bcos-utilities/ThreadPool.h>
#include <condition_variable>

namespace bcos::sealer
{
using TxsMetaDataQueue = std::deque<bcos::protocol::TransactionMetaData::Ptr>;

struct SealingVars
{
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;

    void get(std::invocable<const SealingVars&> auto getter) const
    {
        std::unique_lock lock(m_mutex);
        getter(*this);
    }
    void set(std::invocable<SealingVars&> auto setter)
    {
        std::unique_lock lock(m_mutex);
        setter(*this);
        m_condition.notify_one();
    }
    bool waitForShouldGenerateProposal();
    bool shouldGenerateProposal();

    // the invalid sealingNumber is -1
    ssize_t m_latestNumber{0};
    ssize_t m_sealingNumber{-1};
    ssize_t m_startSealingNumber{0};
    ssize_t m_endSealingNumber{0};
    uint64_t m_lastSealTime{0};
    // for sys block
    int64_t m_waitUntil{0};
};

class SealingManager : public std::enable_shared_from_this<SealingManager>
{
public:
    using Ptr = std::shared_ptr<SealingManager>;
    using ConstPtr = std::shared_ptr<SealingManager const>;

    explicit SealingManager(SealerConfig::Ptr _config);
    SealingManager(const SealingManager&) = delete;
    SealingManager(SealingManager&&) = delete;
    SealingManager& operator=(const SealingManager&) = delete;
    SealingManager& operator=(SealingManager&&) = delete;

    virtual ~SealingManager() noexcept = default;
    bool shouldGenerateProposal();

    std::pair<bool, bcos::protocol::Block::Ptr> generateProposal(
        std::function<uint16_t(bcos::protocol::Block::Ptr)>);

    // the consensus module notify the sealer to reset sealing when viewchange
    virtual void resetSealing();
    virtual void resetSealingInfo(
        ssize_t _startSealingNumber, ssize_t _endSealingNumber, size_t _maxTxsPerBlock);

    virtual void resetLatestNumber(int64_t _latestNumber);
    virtual void resetLatestHash(crypto::HashType _latestHash);
    virtual int64_t latestNumber() const;
    virtual crypto::HashType latestHash() const;

    enum class FetchResult : int8_t
    {
        SUCCESS,
        NOT_READY,
        NO_TRANSACTION,
    };
    virtual FetchResult fetchTransactions();

    template <class T>
    bcos::Handler<> onReady(T callback)
    {
        return m_onReady.add(std::move(callback));
    }
    virtual void notifyResetProposal(bcos::protocol::Block const& _block);

protected:
    virtual void appendTransactions(
        TxsMetaDataQueue& _txsQueue, bcos::protocol::Block const& _fetchedTxs);
    virtual bool reachMinSealTimeCondition();
    virtual void clearPendingTxs();
    virtual void notifyResetTxsFlag(
        const bcos::crypto::HashList& _txsHash, bool _flag, size_t _retryTime = 0);

    virtual int64_t txsSizeExpectedToFetch();
    virtual size_t pendingTxsSize();

private:
    SealerConfig::Ptr m_config;
    TxsMetaDataQueue m_pendingTxs;
    TxsMetaDataQueue m_pendingSysTxs;
    SharedMutex x_pendingTxs;

    SealingVars m_sealingVars;
    size_t m_maxTxsPerBlock = {0};

    bcos::CallbackCollectionHandler<> m_onReady;
    std::mutex m_fetchingTxsMutex;
    bcos::crypto::HashType m_latestHash;
};
}  // namespace bcos::sealer
