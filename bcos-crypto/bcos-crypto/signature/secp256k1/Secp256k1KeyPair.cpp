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
 * @brief implementation for secp256k1 KeyPair
 * @file Secp256k1KeyPair.cpp
 * @date 2021.03.05
 * @author yujiechen
 */
#include "Secp256k1KeyPair.h"
#include "Secp256k1Impl.h"
#include <bcos-crypto/hash/Keccak256.h>
#include <secp256k1.h>

bcos::crypto::PublicPtr bcos::crypto::secp256k1PriToPub(bcos::crypto::Secret const& _secret)
{
    auto publicKey = std::make_shared<KeyImpl>(SECP256K1_PUBLIC_LEN);
    bcos::crypto::key::secp256k1::SECP256K1SecretKeyRef ref(
        reinterpret_cast<const unsigned char*>(_secret.constData()), _secret.size());

    using key::secretKeyToPublicKey;
    auto pubkey = secretKeyToPublicKey(ref);
    assert(pubkey[0] == 0x4);

    std::copy(pubkey.begin() + 1, pubkey.end(), publicKey->mutableData());
    return publicKey;
}

bcos::crypto::Secp256k1KeyPair::Secp256k1KeyPair()
  : KeyPair(SECP256K1_PUBLIC_LEN, SECP256K1_PRIVATE_LEN, KeyPairType::Secp256K1)
{}

bcos::crypto::Secp256k1KeyPair::Secp256k1KeyPair(SecretPtr _secretKey) : Secp256k1KeyPair()
{
    m_publicKey = priToPub(*_secretKey);
    m_secretKey = std::move(_secretKey);
    m_type = KeyPairType::Secp256K1;
}