#include "Secp256k1Impl.h"
#include "../Exceptions.h"
#include <secp256k1.h>

static const std::unique_ptr<secp256k1_context,
    decltype([](auto* ptr) { secp256k1_context_destroy(ptr); })>
    g_SECP256K1_CTX{secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY)};

bcos::crypto::key::secp256k1::SECP256K1UncompressPublicKey tag_invoke(
    bcos::crypto::key::tag_t<bcos::crypto::key::secretKeyToPublicKey> /*unused*/,
    bcos::crypto::key::secp256k1::SECP256K1SecretKeyRef secretKey)
{
    secp256k1_pubkey pubkey;
    if (secp256k1_ec_pubkey_create(
            g_SECP256K1_CTX.get(), std::addressof(pubkey), secretKey.data()) == 0)
    {
        BOOST_THROW_EXCEPTION(bcos::crypto::PriToPublicKeyException()
                              << bcos::errinfo_comment("secp256k1GenerateKeyPair exception"));
    }

    bcos::crypto::key::secp256k1::SECP256K1UncompressPublicKey serializePubkey{};
    size_t outSize = serializePubkey.size();
    if (secp256k1_ec_pubkey_serialize(g_SECP256K1_CTX.get(), serializePubkey.data(),
            std::addressof(outSize), std::addressof(pubkey), SECP256K1_EC_UNCOMPRESSED) == 0)
    {
        BOOST_THROW_EXCEPTION(bcos::crypto::PriToPublicKeyException()
                              << bcos::errinfo_comment("secp256k1GenerateKeyPair exception"));
    }
    assert(serializePubkey[0] == 0x4);
    return serializePubkey;
}

// bcos::crypto::key::secp256k1::SECP256K1SecretKey tag_invoke(
//     bcos::crypto::keypair::tag_t<bcos::crypto::keypair::secretKey> /*unused*/,
//     const bcos::crypto::Secp256k1KeyPair& keyPair)
// {
//     bcos::crypto::key::secp256k1::SECP256K1SecretKey secretKey;
//     auto secretKeyPtr = keyPair.secretKey();
//     std::copy(secretKeyPtr->constData(),
//         secretKeyPtr->constData() + bcos::crypto::SECP256K1_PRIVATE_LEN, secretKey.begin());
//     return secretKey;
// }

// bcos::crypto::key::secp256k1::SECP256K1UncompressPublicKey tag_invoke(
//     bcos::crypto::keypair::tag_t<bcos::crypto::keypair::publicKey> /*unused*/,
//     const bcos::crypto::Secp256k1KeyPair& keyPair)
// {
//     bcos::crypto::key::secp256k1::SECP256K1UncompressPublicKey decompressPublicKey;
//     decompressPublicKey[0] = 0x4;

//     auto publicKeyPtr = keyPair.publicKey();
//     std::copy(keyPair.publicKey()->constData(),
//         keyPair.publicKey()->constData() + bcos::crypto::SECP256K1_PUBLIC_LEN,
//         decompressPublicKey.begin() + 1);
//     return decompressPublicKey;
// }