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
 * @brief implementation of TxValidator
 * @file TxValidator.cpp
 * @author: yujiechen
 * @date 2021-05-11
 */
#include "TxValidator.h"
#include "bcos-task/Wait.h"

using namespace bcos;
using namespace bcos::protocol;
using namespace bcos::txpool;

TransactionStatus TxValidator::verify(const bcos::protocol::Transaction& _tx)
{
    if (_tx.invalid()) [[unlikely]]
    {
        return TransactionStatus::InvalidSignature;
    }
    if (_tx.type() == static_cast<uint8_t>(TransactionType::BCOSTransaction))
    {
        // check groupId and chainId
        if (_tx.groupId() != m_groupId) [[unlikely]]
        {
            return TransactionStatus::InvalidGroupId;
        }
        if (_tx.chainId() != m_chainId) [[unlikely]]
        {
            return TransactionStatus::InvalidChainId;
        }
    }

    // should check the transaction signature first, because the sender of transaction will be force
    // remove in front module check signature
    try
    {
        _tx.verify(*m_cryptoSuite->hashImpl(), *m_cryptoSuite->signatureImpl());
    }
    catch (...)
    {
        return TransactionStatus::InvalidSignature;
    }

    // should check the transaction signature first, because sender is empty
    if (const auto status = checkTransaction(_tx); status != TransactionStatus::None)
    {
        return status;
    }

    if (isSystemTransaction(_tx))
    {
        _tx.setSystemTx(true);
    }
    m_txPoolNonceChecker->insert(std::string(_tx.nonce()));
    if (_tx.type() == static_cast<uint8_t>(TransactionType::Web3Transaction))
    {
        task::syncWait(m_web3NonceChecker->insertMemoryNonce(
            std::string(_tx.sender()), std::string(_tx.nonce())));
    }
    return TransactionStatus::None;
}

bcos::protocol::TransactionStatus TxValidator::checkTransaction(
    const bcos::protocol::Transaction& _tx, bool onlyCheckLedgerNonce)
{
    if (_tx.type() == static_cast<uint8_t>(TransactionType::Web3Transaction))
    {
        return checkWeb3Nonce(_tx, onlyCheckLedgerNonce);
    }
    // compare with nonces cached in memory, only check nonce in txpool
    if (!onlyCheckLedgerNonce)
    {
        if (auto status = checkTxpoolNonce(_tx); status != TransactionStatus::None)
        {
            return status;
        }
    }
    // check ledger nonce and block limit
    auto status = checkLedgerNonceAndBlockLimit(_tx);
    return status;
}


TransactionStatus TxValidator::checkLedgerNonceAndBlockLimit(const bcos::protocol::Transaction& _tx)
{
    // compare with nonces stored on-chain, and check block limit inside
    auto status = m_ledgerNonceChecker->checkNonce(_tx);
    if (status != TransactionStatus::None)
    {
        return status;
    }
    if (isSystemTransaction(_tx))
    {
        _tx.setSystemTx(true);
    }
    return TransactionStatus::None;
}

TransactionStatus TxValidator::checkTxpoolNonce(const bcos::protocol::Transaction& _tx)
{
    return m_txPoolNonceChecker->checkNonce(_tx);
}

bcos::protocol::TransactionStatus TxValidator::checkWeb3Nonce(
    const bcos::protocol::Transaction& _tx, bool onlyCheckLedgerNonce)
{
    if (_tx.type() != static_cast<uint8_t>(TransactionType::Web3Transaction)) [[likely]]
    {
        return TransactionStatus::None;
    }
    return task::syncWait(m_web3NonceChecker->checkWeb3Nonce(_tx, onlyCheckLedgerNonce));
}
bool bcos::txpool::TxValidator::isSystemTransaction(const bcos::protocol::Transaction& _tx)
{
    return precompiled::contains(bcos::precompiled::c_systemTxsAddress, _tx.to());
}
bcos::txpool::TxValidator::TxValidator(NonceCheckerInterface::Ptr _txPoolNonceChecker,
    Web3NonceChecker::Ptr _web3NonceChecker, bcos::crypto::CryptoSuite::Ptr _cryptoSuite,
    std::string _groupId, std::string _chainId)
  : m_txPoolNonceChecker(std::move(_txPoolNonceChecker)),
    m_web3NonceChecker(std::move(_web3NonceChecker)),
    m_cryptoSuite(std::move(_cryptoSuite)),
    m_groupId(std::move(_groupId)),
    m_chainId(std::move(_chainId))
{}
bcos::txpool::Web3NonceChecker::Ptr bcos::txpool::TxValidator::web3NonceChecker()
{
    return m_web3NonceChecker;
}
bcos::txpool::LedgerNonceChecker::Ptr bcos::txpool::TxValidator::ledgerNonceChecker()
{
    return m_ledgerNonceChecker;
}
void bcos::txpool::TxValidator::setLedgerNonceChecker(LedgerNonceChecker::Ptr _ledgerNonceChecker)
{
    m_ledgerNonceChecker = std::move(_ledgerNonceChecker);
}
