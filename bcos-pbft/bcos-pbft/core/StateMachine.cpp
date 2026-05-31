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
 * @brief state machine to execute the transactions
 * @file StateMachine.cpp
 * @author: yujiechen
 * @date 2021-05-18
 */
#include "StateMachine.h"
#include "Common.h"
#include <bcos-task/Wait.h>

using namespace bcos;
using namespace bcos::consensus;
using namespace bcos::protocol;
using namespace bcos::crypto;

void StateMachine::asyncApply(ssize_t _timeout, ProposalInterface::ConstPtr _lastAppliedProposal,
    ProposalInterface::Ptr _proposal, ProposalInterface::Ptr _executedProposal,
    std::function<void(int64_t)> _onExecuteFinished)
{
    auto self = weak_from_this();
    // Note: async here to increase performance
    m_worker->enqueue(
        [self, _timeout, _lastAppliedProposal, _proposal, _executedProposal, _onExecuteFinished]() {
            auto stateMachine = self.lock();
            if (!stateMachine)
            {
                return;
            }
            stateMachine->apply(
                _timeout, _lastAppliedProposal, _proposal, _executedProposal, _onExecuteFinished);
        });
}

void StateMachine::asyncPreApply(
    ProposalInterface::Ptr _proposal, std::function<void(bool)> _onPreApplyFinished)
{
    preApply(std::move(_proposal), std::move(_onPreApplyFinished));
}

void StateMachine::apply(ssize_t, ProposalInterface::ConstPtr _lastAppliedProposal,
    ProposalInterface::Ptr _proposal, ProposalInterface::Ptr _executedProposal,
    std::function<void(int64_t)> _onExecuteFinished)
{
    if (_proposal->index() <= _lastAppliedProposal->index())
    {
        CONSENSUS_LOG(WARNING) << LOG_DESC("asyncApply: the proposal has already been applied")
                               << LOG_KV("proposalIndex", _proposal->index())
                               << LOG_KV("lastAppliedProposal", _lastAppliedProposal->index());
        if (_onExecuteFinished)
        {
            _onExecuteFinished(-1);
        }
        return;
    }
    auto block = m_blockFactory->createBlock(_proposal->data());
    // invalid block
    auto blockHeader = block->blockHeader();
    if (!blockHeader)
    {
        if (_onExecuteFinished)
        {
            _onExecuteFinished(-1);
        }
        return;
    }
    // set the parentHash information
    if (_proposal->index() == _lastAppliedProposal->index() + 1)
    {
        ParentInfoList parentInfoList;
        ParentInfo parentInfo{.blockNumber = _lastAppliedProposal->index(),
            .blockHash = _lastAppliedProposal->hash()};
        parentInfoList.push_back(parentInfo);
        blockHeader->setParentInfo(parentInfoList);
        CONSENSUS_LOG(DEBUG) << LOG_DESC("setParentInfo for the proposal")
                             << LOG_KV("proposalIndex", _proposal->index())
                             << LOG_KV("lastAppliedProposal", _lastAppliedProposal->index())
                             << LOG_KV("parentHash", _lastAppliedProposal->hash().abridged());
    }
    else
    {
        CONSENSUS_LOG(FATAL) << LOG_DESC("invalid lastAppliedProposal")
                             << LOG_KV("lastAppliedIndex", _lastAppliedProposal->index())
                             << LOG_KV("proposal", _proposal->index());
    }
    blockHeader->calculateHash(*m_blockFactory->cryptoSuite()->hashImpl());

    // calls dispatcher to execute the block
    auto startT = utcTime();

    // capture values needed after co_await (block will be moved)
    auto expectedNumber = blockHeader->number();
    auto txsHashSize = block->transactionsHashSize();

    bcos::task::wait(
        [scheduler = m_scheduler, block = std::move(block), startT, expectedNumber, txsHashSize,
            _onExecuteFinished = std::move(_onExecuteFinished),
            _proposal = std::move(_proposal),
            _executedProposal = std::move(_executedProposal)]() -> bcos::task::Task<void> {
            try
            {
                auto [executedHeader, sysBlock] =
                    co_await scheduler->executeBlock(std::move(block), false);

                auto execT = (double)(utcTime() - startT) / (double)(txsHashSize);
                CONSENSUS_LOG(INFO)
                    << METRIC << LOG_DESC("asyncExecuteBlock success")
                    << LOG_KV("sysBlock", sysBlock)
                    << LOG_KV("number", executedHeader->number())
                    << LOG_KV("result", executedHeader->hash().abridged())
                    << LOG_KV("txsSize", txsHashSize)
                    << LOG_KV("txsRoot", executedHeader->txsRoot().abridged())
                    << LOG_KV("receiptsRoot", executedHeader->receiptsRoot().abridged())
                    << LOG_KV("stateRoot", executedHeader->stateRoot().abridged())
                    << LOG_KV("timeCost", (utcTime() - startT))
                    << LOG_KV("execPerTx", execT);

                if (executedHeader->number() != expectedNumber)
                {
                    CONSENSUS_LOG(WARNING)
                        << LOG_DESC("asyncExecuteBlock exception")
                        << LOG_KV("expectedNumber", expectedNumber)
                        << LOG_KV("number", executedHeader->number())
                        << LOG_KV("timeCost", (utcTime() - startT));
                    // FIB-112: must call _onExecuteFinished on every exit path
                    _onExecuteFinished(-1);
                    co_return;
                }

                _executedProposal->setIndex(executedHeader->number());
                _executedProposal->setHash(executedHeader->hash());

                bcos::bytes blockHeaderBuffer;
                executedHeader->encode(blockHeaderBuffer);
                _executedProposal->setData(std::move(blockHeaderBuffer));
                // the transactions hash list
                _executedProposal->setExtraData(_proposal->data());
                // The _onExecuteFinished callback itself does the asynchronous logic,
                // so there is no need to use m_worker to re-synchronize it here.
                _onExecuteFinished(0);
            }
            catch (bcos::Error& e)
            {
                CONSENSUS_LOG(WARNING)
                    << LOG_DESC("asyncExecuteBlock failed")
                    << LOG_KV("number", expectedNumber)
                    << LOG_KV("code", e.errorCode())
                    << LOG_KV("message", e.errorMessage());
                _onExecuteFinished(e.errorCode());
            }
        }());
}

void StateMachine::preApply(
    ProposalInterface::Ptr _proposal, std::function<void(bool)> _onPreApplyFinished)
{
    auto block = m_blockFactory->createBlock(_proposal->data());

    auto startT = utcTime();
    bcos::task::wait(
        [scheduler = m_scheduler, block, startT,
            _onPreApplyFinished = std::move(_onPreApplyFinished)]() -> bcos::task::Task<void> {
            try
            {
                co_await scheduler->preExecuteBlock(block, false);
                CONSENSUS_LOG(DEBUG)
                    << LOG_BADGE("prepareBlockExecutive") << LOG_DESC("preApply")
                    << LOG_KV("blockNumber", block->blockHeader()->number())
                    << LOG_KV("blockHeader.timestamps", block->blockHeader()->timestamp())
                    << LOG_KV("timeCost", (utcTime() - startT));
                _onPreApplyFinished(true);
            }
            catch (bcos::Error& e)
            {
                CONSENSUS_LOG(ERROR)
                    << LOG_BADGE("prepareBlockExecutive") << LOG_DESC("preApply failed!")
                    << LOG_KV("code", e.errorCode())
                    // FIB-113: removed duplicate LOG_KV("message", ...) entry
                    << LOG_KV("message", e.errorMessage())
                    << LOG_KV("blockNumber", block->blockHeader()->number())
                    << LOG_KV("blockHeader.timestamps", block->blockHeader()->timestamp())
                    << LOG_KV("timeCost", (utcTime() - startT));
                _onPreApplyFinished(false);
            }
        }());
}