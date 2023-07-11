/**
 *  Copyright (C) 2022 FISCO BCOS.
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
 * @file TxpoolSyncByTree.cpp
 * @author: kyonGuo
 * @date 2023/7/3
 */

#include "FakeTxsSyncMsg.h"
#include "bcos-crypto/interfaces/crypto/KeyPairInterface.h"
#include "bcos-crypto/signature/sm2/SM2Crypto.h"
#include "bcos-tars-protocol/protocol/TransactionImpl.h"
#include "test/unittests/txpool/TxPoolFixture.h"
#include <bcos-crypto/hash/Keccak256.h>
#include <bcos-crypto/hash/SM3.h>
#include <bcos-crypto/interfaces/crypto/CryptoSuite.h>
#include <bcos-crypto/signature/secp256k1/Secp256k1Crypto.h>
#include <bcos-framework/protocol/CommonError.h>
#include <bcos-tars-protocol/testutil/FakeTransaction.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/test/unit_test.hpp>
using namespace bcos;
using namespace bcos::sync;
using namespace bcos::crypto;
namespace bcos::test
{
BOOST_FIXTURE_TEST_SUITE(TxpoolSyncByTreeTest, TxPoolFixture)

BOOST_AUTO_TEST_CASE(testFreeNodeTreeSync)
{
    auto txpool = this->txpool();
    auto tx = fakeTransaction(this->m_cryptoSuite, std::to_string(utcTime()));
    bcos::task::wait([](decltype(txpool) txpool, decltype(tx) tx) -> bcos::task::Task<void> {
        bcos::bytes data;
        tx->encode(data);
        co_await txpool->broadcastTransactionBuffer(bcos::ref(data));
    }(txpool, tx));
}

BOOST_AUTO_TEST_CASE(testConsensusNodeTreeSync)
{
    this->appendSealer(this->m_nodeId);
    for (const auto& item : this->m_nodeIdList)
    {
        this->appendSealer(item);
    }
    NodeIDs nodeIds = m_nodeIdList;
    nodeIds.push_back(this->m_nodeId);
    auto& txpool = dynamic_cast<TxPool&>(*this->txpool());
    txpool.treeRouter()->updateConsensusNodeInfo(nodeIds);
    BCOS_LOG(TRACE) << LOG_DESC("updateRouter")
                    << LOG_KV("consIndex", txpool.treeRouter()->consIndex());

    auto tx = fakeTransaction(this->m_cryptoSuite, std::to_string(utcSteadyTime()));
    bcos::task::wait([](decltype(txpool) txpool, decltype(tx) tx) -> bcos::task::Task<void> {
        bcos::bytes data;
        tx->encode(data);
        co_await txpool.broadcastTransactionBuffer(bcos::ref(data));
    }(txpool, tx));
    // broadcast to 3 nodes
    for (const auto& item : this->m_nodeIdList | RANGES::views::take(3))
    {
        auto& nodeTxpool = dynamic_cast<TxPool&>(*m_fakeGateWay->m_nodeId2TxPool.at(item));
        auto size = nodeTxpool.txpoolStorage()->size();
        BOOST_CHECK(size == 1);
    }
    // FIXME: should solve broadcast storm
    for (const auto& item : this->m_nodeIdList | RANGES::views::drop(3))
    {
        auto& nodeTxpool = dynamic_cast<TxPool&>(*m_fakeGateWay->m_nodeId2TxPool.at(item));
        auto size = nodeTxpool.txpoolStorage()->size();
        BOOST_CHECK(size == 0);
    }
}

BOOST_AUTO_TEST_CASE(testConsensusNodeWithLowerVersionTreeSync)
{
    auto gInfo = std::make_shared<FakeGroupInfo>();
    for (int i = 0; i < 4; ++i)
    {
        auto protocol = std::make_shared<protocol::ProtocolInfo>();
        protocol->setVersion(i % 2 == 0 ? V1 : V2);
        gInfo->appendProtocol(protocol);
    }
    m_frontService->setGroupInfo(gInfo);

    this->appendSealer(this->m_nodeId);
    for (const auto& item : this->m_nodeIdList)
    {
        this->appendSealer(item);
    }
    NodeIDs nodeIds = m_nodeIdList;
    nodeIds.push_back(this->m_nodeId);
    auto& txpool = dynamic_cast<TxPool&>(*this->txpool());
    txpool.treeRouter()->updateConsensusNodeInfo(nodeIds);
    BCOS_LOG(TRACE) << LOG_DESC("updateRouter")
                    << LOG_KV("consIndex", txpool.treeRouter()->consIndex());

    auto tx = fakeTransaction(this->m_cryptoSuite, std::to_string(utcSteadyTimeUs()));
    bcos::task::wait([](decltype(txpool) txpool, decltype(tx) tx) -> bcos::task::Task<void> {
        bcos::bytes data;
        tx->encode(data);
        co_await txpool.broadcastTransactionBuffer(bcos::ref(data));
    }(txpool, tx));
    // broadcast to all nodes
    for (const auto& item : this->m_nodeIdList)
    {
        auto& nodeTxpool = dynamic_cast<TxPool&>(*m_fakeGateWay->m_nodeId2TxPool.at(item));
        auto size = nodeTxpool.txpoolStorage()->size();
        BOOST_CHECK(size == 1);
    }
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace bcos::test