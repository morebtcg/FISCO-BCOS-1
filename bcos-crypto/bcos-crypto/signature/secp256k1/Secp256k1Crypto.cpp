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
 * @brief implementation for secp256k1 signature algorithm
 * @file Secp256k1Signature.cpp
 * @date 2021.03.05
 * @author yujiechen
 */

#include <bcos-crypto/signature/Exceptions.h>
#include <bcos-crypto/signature/codec/SignatureDataWithV.h>
#include <bcos-crypto/signature/secp256k1/Secp256k1Crypto.h>
#include <bcos-crypto/signature/secp256k1/Secp256k1KeyPair.h>
#include <secp256k1.h>
#include <secp256k1_recovery.h>
#include <algorithm>
#include <array>
#include <limits>
#include <memory>

using namespace bcos;
using namespace bcos::crypto;

std::shared_ptr<bytes> bcos::crypto::secp256k1Sign(
    const KeyPairInterface& _keyPair, const HashType& _hash)
{
    std::shared_ptr<bytes> signatureData = std::make_shared<bytes>(SECP256K1_SIGNATURE_LEN, 0);
    // secp256k1_ecdsa_recoverable_signature rawSig;
    auto* rawSig = (secp256k1_ecdsa_recoverable_signature*)(signatureData->data());
    if (secp256k1_ecdsa_sign_recoverable(g_SECP256K1_CTX.get(), rawSig, _hash.data(),
            (const unsigned char*)_keyPair.secretKey()->constData(), nullptr, nullptr) == 0)
    {
        BOOST_THROW_EXCEPTION(SignException() << errinfo_comment(
                                  "secp256k1Sign exception, raw data: " + _hash.hex()));
    }
    int recid = 0;
    secp256k1_ecdsa_recoverable_signature_serialize_compact(
        g_SECP256K1_CTX.get(), (unsigned char*)signatureData->data(), &recid, rawSig);
    signatureData->back() = (uint8_t)recid;
    return signatureData;
}

bool bcos::crypto::secp256k1Verify(
    const PublicPtr& _pubKey, const HashType& _hash, bytesConstRef _signatureData)
{
    if ((uint8_t)_signatureData[SECP256K1_SIGNATURE_V] > 3)
    {
        BOOST_THROW_EXCEPTION(
            InvalidSignature() << errinfo_comment(
                "secp256k1 verify illegal argument: recid >= 0 && recid <= 3, recid: " +
                std::to_string((int)_signatureData[SECP256K1_SIGNATURE_V])));
    }

    secp256k1_ecdsa_recoverable_signature sig;
    secp256k1_ecdsa_recoverable_signature_parse_compact(g_SECP256K1_CTX.get(), &sig,
        _signatureData.data(), (int)_signatureData[SECP256K1_SIGNATURE_V]);

    secp256k1_pubkey recoverPubKey;
    if (secp256k1_ecdsa_recover(g_SECP256K1_CTX.get(), &recoverPubKey, &sig, _hash.data()) == 0)
    {
        BOOST_THROW_EXCEPTION(InvalidSignature() << errinfo_comment(
                                  "invalid signature: secp256k1Recover failed, msgHash : " +
                                  _hash.hex() + ", signData:" + *toHexString(_signatureData)));
    }
    std::array<unsigned char, SECP256K1_UNCOMPRESS_PUBLICKEY_LEN> data{};
    size_t serializedPubKeySize = SECP256K1_UNCOMPRESS_PUBLICKEY_LEN;
    secp256k1_ec_pubkey_serialize(g_SECP256K1_CTX.get(), data.data(), &serializedPubKeySize,
        &recoverPubKey, SECP256K1_EC_UNCOMPRESSED);
    return memcmp(data.data() + 1, _pubKey->constData(), SECP256K1_PUBLICKEY_LEN) == 0;
}

KeyPairInterface::UniquePtr bcos::crypto::secp256k1GenerateKeyPair()
{
    auto keyPair = std::make_unique<Secp256k1KeyPair>();

    std::random_device device;
    std::uniform_int_distribution<char> uniformDist(
        std::numeric_limits<char>::min(), std::numeric_limits<char>::max());
    do
    {
        auto range = std::span(keyPair->secretKey()->mutableData(), keyPair->secretKey()->size());
        std::generate(range.begin(), range.end(), [&]() { return uniformDist(device); });
    } while (secp256k1_ec_seckey_verify(g_SECP256K1_CTX.get(),
                 reinterpret_cast<const unsigned char*>(keyPair->secretKey()->constData())) == 0);

    secp256k1_pubkey pubkey;
    if (secp256k1_ec_pubkey_create(g_SECP256K1_CTX.get(), std::addressof(pubkey),
            reinterpret_cast<const unsigned char*>(keyPair->secretKey()->constData())) == 0)
    {
        BOOST_THROW_EXCEPTION(
            GenerateKeyPairException() << errinfo_comment("secp256k1GenerateKeyPair exception"));
    }

    std::array<unsigned char, SECP256K1_UNCOMPRESS_PUBLICKEY_LEN> serializePubkey{};
    size_t outSize = SECP256K1_UNCOMPRESS_PUBLICKEY_LEN;
    if (secp256k1_ec_pubkey_serialize(g_SECP256K1_CTX.get(), serializePubkey.data(),
            std::addressof(outSize), std::addressof(pubkey), SECP256K1_EC_UNCOMPRESSED) == 0)
    {
        BOOST_THROW_EXCEPTION(
            GenerateKeyPairException() << errinfo_comment("secp256k1GenerateKeyPair exception"));
    }
    assert(serializePubkey[0] == 0x4);
    std::copy(
        serializePubkey.begin() + 1, serializePubkey.end(), keyPair->publicKey()->mutableData());

    return keyPair;
}

void secp256k1RecoverPrimitive(const HashType& _hash, char* _pubKey, bytesConstRef _signatureData)
{
    secp256k1_ecdsa_recoverable_signature sig;
    secp256k1_ecdsa_recoverable_signature_parse_compact(g_SECP256K1_CTX.get(), &sig,
        _signatureData.data(), (int)_signatureData[SECP256K1_SIGNATURE_V]);
    if (secp256k1_ecdsa_recover(
            g_SECP256K1_CTX.get(), (secp256k1_pubkey*)_pubKey, &sig, _hash.data()) == 0)
    {
        BOOST_THROW_EXCEPTION(InvalidSignature() << errinfo_comment(
                                  "invalid signature: secp256k1Recover failed, msgHash : " +
                                  _hash.hex() + ", signData:" + *toHexString(_signatureData)));
    }
    size_t serializedPubkeySize = SECP256K1_UNCOMPRESS_PUBLICKEY_LEN;
    std::array<unsigned char, SECP256K1_UNCOMPRESS_PUBLICKEY_LEN> data{};

    secp256k1_ec_pubkey_serialize(g_SECP256K1_CTX.get(), data.data(), &serializedPubkeySize,
        (secp256k1_pubkey*)_pubKey, SECP256K1_EC_UNCOMPRESSED);
    assert(data[0] == 0x04);
    memcpy(_pubKey, data.data() + 1, SECP256K1_PUBLICKEY_LEN);
}

PublicPtr bcos::crypto::secp256k1Recover(const HashType& _hash, bytesConstRef _signatureData)
{
    if ((uint8_t)_signatureData[SECP256K1_SIGNATURE_V] > 3)
    {
        BOOST_THROW_EXCEPTION(
            InvalidSignature() << errinfo_comment(
                "secp256k1 recover illegal argument: recid >= 0 && recid <= 3, recid: " +
                std::to_string((int)_signatureData[SECP256K1_SIGNATURE_V])));
    }
    auto pubKey = std::make_shared<KeyImpl>(SECP256K1_PUBLIC_LEN);
    secp256k1RecoverPrimitive(_hash, pubKey->mutableData(), _signatureData);
    return pubKey;
}

std::pair<bool, bytes> bcos::crypto::secp256k1Recover(Hash::Ptr _hashImpl, bytesConstRef _input)
{
    struct
    {
        HashType hash;
        h256 v;
        h256 r;
        h256 s;
    } in;
    memcpy(&in, _input.data(), std::min(_input.size(), sizeof(in)));
    u256 v = (u256)in.v;
    if (v >= 27 && v <= 28)
    {
        auto signatureData = SignatureDataWithV(in.r, in.s, (byte)((int)v - 27));
        try
        {
            auto encodedBytes = signatureData.encode();
            auto publicKey = secp256k1Recover(
                in.hash, bytesConstRef(encodedBytes->data(), encodedBytes->size()));
            auto address = calculateAddress(_hashImpl, publicKey);
            return {true, address.asBytes()};
        }
        catch (const std::exception& e)
        {
            CRYPTO_LOG(WARNING) << LOG_DESC("secp256k1Recover failed")
                                << LOG_KV("message", boost::diagnostic_information(e));
        }
    }
    return {false, {}};
}

std::pair<bool, bytes> Secp256k1Crypto::recoverAddress(
    crypto::Hash& _hashImpl, const HashType& _hash, bytesConstRef _signatureData) const
{
    std::array<unsigned char, SECP256K1_UNCOMPRESS_PUBLICKEY_LEN> data{};
    secp256k1_ecdsa_recoverable_signature sig;
    secp256k1_pubkey pubkey;
    secp256k1_ecdsa_recoverable_signature_parse_compact(g_SECP256K1_CTX.get(), &sig,
        _signatureData.data(), (int)_signatureData[SECP256K1_SIGNATURE_V]);
    if (secp256k1_ecdsa_recover(g_SECP256K1_CTX.get(), &pubkey, &sig, _hash.data()) == 0)
    {
        BOOST_THROW_EXCEPTION(InvalidSignature() << errinfo_comment(
                                  "invalid signature: secp256k1Recover failed, msgHash : " +
                                  _hash.hex() + ", signData:" + *toHexString(_signatureData)));
    }
    size_t serializedPubkeySize = SECP256K1_UNCOMPRESS_PUBLICKEY_LEN;

    secp256k1_ec_pubkey_serialize(g_SECP256K1_CTX.get(), data.data(), &serializedPubkeySize,
        &pubkey, SECP256K1_EC_UNCOMPRESSED);
    assert(data[0] == 0x04);
    return {true, calculateAddress(_hashImpl, data.data() + 1, SECP256K1_PUBLICKEY_LEN)};
}

bool Secp256k1Crypto::verify(std::shared_ptr<bytes const> _pubKeyBytes, const HashType& _hash,
    bytesConstRef _signatureData) const
{
    return secp256k1Verify(
        std::make_shared<KeyImpl>(SECP256K1_PUBLIC_LEN, _pubKeyBytes), _hash, _signatureData);
}

KeyPairInterface::UniquePtr Secp256k1Crypto::createKeyPair(SecretPtr _secretKey) const
{
    return std::make_unique<Secp256k1KeyPair>(_secretKey);
}