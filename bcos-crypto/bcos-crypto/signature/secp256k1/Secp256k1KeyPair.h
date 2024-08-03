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
#include <bcos-crypto/interfaces/crypto/Signature.h>
#include <bcos-crypto/signature/key/KeyPair.h>
#include <secp256k1.h>

namespace bcos::crypto
{
constexpr static int SECP256K1_PUBLIC_LEN = 64;
constexpr static int SECP256K1_PRIVATE_LEN = 32;

static const std::unique_ptr<secp256k1_context, decltype(&secp256k1_context_destroy)>
    g_SECP256K1_CTX{secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY),
        &secp256k1_context_destroy};

PublicPtr secp256k1PriToPub(Secret const& _secret);
class Secp256k1KeyPair : public KeyPair
{
public:
    using Ptr = std::shared_ptr<Secp256k1KeyPair>;
    Secp256k1KeyPair()
      : KeyPair(SECP256K1_PUBLIC_LEN, SECP256K1_PRIVATE_LEN, KeyPairType::Secp256K1)
    {}
    explicit Secp256k1KeyPair(SecretPtr _secretKey) : Secp256k1KeyPair()
    {
        m_secretKey = _secretKey;
        m_publicKey = priToPub(_secretKey);
        m_type = KeyPairType::Secp256K1;
    }
    virtual PublicPtr priToPub(SecretPtr _secret) { return secp256k1PriToPub(*_secret); }
};
}  // namespace bcos::crypto
