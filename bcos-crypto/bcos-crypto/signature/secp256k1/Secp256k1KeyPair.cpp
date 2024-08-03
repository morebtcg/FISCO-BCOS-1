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
#include "bcos-crypto/signature/secp256k1/Secp256k1Crypto.h"
#include <bcos-crypto/hash/Keccak256.h>
#include <bcos-crypto/signature/Exceptions.h>
#include <bcos-crypto/signature/secp256k1/Secp256k1KeyPair.h>
#include <secp256k1.h>

bcos::crypto::PublicPtr bcos::crypto::secp256k1PriToPub(bcos::crypto::Secret const& _secret)
{
    auto publicKey = std::make_shared<KeyImpl>(SECP256K1_PUBLIC_LEN);

    secp256k1_pubkey pubkey;
    if (secp256k1_ec_pubkey_create(g_SECP256K1_CTX.get(), std::addressof(pubkey),
            reinterpret_cast<const unsigned char*>(_secret.constData())) == 0)
    {
        BOOST_THROW_EXCEPTION(
            PriToPublicKeyException() << errinfo_comment("secp256k1GenerateKeyPair exception"));
    }

    std::array<unsigned char, SECP256K1_UNCOMPRESS_PUBLICKEY_LEN> serializePubkey{};
    size_t outSize = SECP256K1_UNCOMPRESS_PUBLICKEY_LEN;
    if (secp256k1_ec_pubkey_serialize(g_SECP256K1_CTX.get(), serializePubkey.data(),
            std::addressof(outSize), std::addressof(pubkey), SECP256K1_EC_UNCOMPRESSED) == 0)
    {
        BOOST_THROW_EXCEPTION(
            PriToPublicKeyException() << errinfo_comment("secp256k1GenerateKeyPair exception"));
    }
    assert(serializePubkey[0] == 0x4);
    std::copy(serializePubkey.begin() + 1, serializePubkey.end(), publicKey->mutableData());
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

bcos::crypto::SECP256K1UncompressPublicKey bcos::crypto::tag_invoke(
    key::tag_t<key::secretKeyToPublicKey>, SECP256K1SecretKey const& secretKey)
{
    secp256k1_pubkey pubkey;
    if (secp256k1_ec_pubkey_create(
            g_SECP256K1_CTX.get(), std::addressof(pubkey), secretKey.data()) == 0)
    {
        BOOST_THROW_EXCEPTION(
            PriToPublicKeyException() << errinfo_comment("secp256k1GenerateKeyPair exception"));
    }

    bcos::crypto::SECP256K1UncompressPublicKey serializePubkey{};
    size_t outSize = SECP256K1_UNCOMPRESS_PUBLICKEY_LEN;
    if (secp256k1_ec_pubkey_serialize(g_SECP256K1_CTX.get(), serializePubkey.data(),
            std::addressof(outSize), std::addressof(pubkey), SECP256K1_EC_UNCOMPRESSED) == 0)
    {
        BOOST_THROW_EXCEPTION(
            PriToPublicKeyException() << errinfo_comment("secp256k1GenerateKeyPair exception"));
    }
    assert(serializePubkey[0] == 0x4);
    return serializePubkey;
}

bcos::crypto::SECP256K1SecretKey bcos::crypto::tag_invoke(
    keypair::tag_t<keypair::secretKey> /*unused*/, const Secp256k1KeyPair& keyPair)
{
    SECP256K1SecretKey secretKey;
    auto secretKeyPtr = keyPair.secretKey();
    std::copy(secretKeyPtr->constData(), secretKeyPtr->constData() + SECP256K1_PRIVATE_LEN,
        secretKey.begin());
    return secretKey;
}

bcos::crypto::SECP256K1UncompressPublicKey bcos::crypto::tag_invoke(
    keypair::tag_t<keypair::publicKey> /*unused*/, const Secp256k1KeyPair& keyPair)
{
    SECP256K1UncompressPublicKey decompressPublicKey;
    decompressPublicKey[0] = 0x4;

    auto publicKeyPtr = keyPair.publicKey();
    std::copy(keyPair.publicKey()->constData(),
        keyPair.publicKey()->constData() + SECP256K1_PUBLIC_LEN, decompressPublicKey.begin() + 1);
    return decompressPublicKey;
}
