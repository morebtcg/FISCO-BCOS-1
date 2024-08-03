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
 * @file Secp256k1KeyPair.h
 * @date 2021.03.05
 * @author yujiechen
 */
#pragma once
#include "../../interfaces/crypto/Key.h"
#include "../../interfaces/crypto/KeyPair.h"
#include <bcos-crypto/interfaces/crypto/Signature.h>
#include <bcos-crypto/signature/key/KeyPair.h>
#include <secp256k1.h>

namespace bcos::crypto
{
constexpr static int SECP256K1_PUBLIC_LEN = 64;
constexpr static int SECP256K1_PRIVATE_LEN = 32;

static const std::unique_ptr<secp256k1_context,
    decltype([](auto* ptr) { secp256k1_context_destroy(ptr); })>
    g_SECP256K1_CTX{secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY)};

PublicPtr secp256k1PriToPub(Secret const& _secret);
class Secp256k1KeyPair : public KeyPair
{
public:
    using Ptr = std::shared_ptr<Secp256k1KeyPair>;
    Secp256k1KeyPair();
    explicit Secp256k1KeyPair(SecretPtr _secretKey);
    static PublicPtr priToPub(Secret const& _secret) { return secp256k1PriToPub(_secret); }
};

using SECP256K1UncompressPublicKey = std::array<bcos::byte, 65>;
using SECP256K1CompressPublicKey = std::array<bcos::byte, 33>;
using SECP256K1SecretKey = std::array<bcos::byte, 32>;

SECP256K1UncompressPublicKey tag_invoke(
    key::tag_t<key::secretKeyToPublicKey>, SECP256K1SecretKey const& secretKey);
SECP256K1SecretKey tag_invoke(keypair::tag_t<keypair::secretKey>, const Secp256k1KeyPair& keyPair);
SECP256K1UncompressPublicKey tag_invoke(
    keypair::tag_t<keypair::publicKey>, const Secp256k1KeyPair& keyPair);

template <bool compress>
auto tag_invoke(key::tag_t<key::serializePublicKey> /*unused*/, auto&& publicKey)
    requires std::same_as<std::decay_t<decltype(publicKey)>, SECP256K1UncompressPublicKey> ||
             std::same_as<std::decay_t<decltype(publicKey)>, SECP256K1CompressPublicKey>
{
    if constexpr ((compress && std::same_as<std::decay_t<decltype(publicKey)>,
                                   SECP256K1CompressPublicKey>) ||
                  (!compress && std::same_as<std::decay_t<decltype(publicKey)>,
                                    SECP256K1UncompressPublicKey>))
    {
        return publicKey;
    }

    secp256k1_pubkey pubkey;
    if (secp256k1_ec_pubkey_parse(
            g_SECP256K1_CTX.get(), std::addressof(pubkey), publicKey.data(), publicKey.size()) == 0)
    {
        BOOST_THROW_EXCEPTION(
            PriToPublicKeyException() << errinfo_comment("secp256k1GenerateKeyPair exception"));
    }

    std::conditional_t<compress, SECP256K1CompressPublicKey, SECP256K1UncompressPublicKey>
        serializePubkey;
    size_t outSize = serializePubkey.size();
    if (secp256k1_ec_pubkey_serialize(g_SECP256K1_CTX.get(), serializePubkey.data(),
            std::addressof(outSize), std::addressof(pubkey),
            compress ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED) == 0)
    {
        BOOST_THROW_EXCEPTION(
            PriToPublicKeyException() << errinfo_comment("secp256k1GenerateKeyPair exception"));
    }
    return serializePubkey;
}
}  // namespace bcos::crypto
