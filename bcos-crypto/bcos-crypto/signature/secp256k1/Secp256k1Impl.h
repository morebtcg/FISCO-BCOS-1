#pragma once
#include "../../interfaces/Key.h"
// #include "../../interfaces/KeyPair.h"
// #include "Secp256k1KeyPair.h"
#include "bcos-utilities/Common.h"

namespace bcos::crypto::key::secp256k1
{
using SECP256K1UncompressPublicKey = std::array<bcos::byte, 65>;
using SECP256K1CompressPublicKey = std::array<bcos::byte, 33>;

struct SECP256K1SecretKey : public std::span<const bcos::byte, 32>
{
    using std::span<const bcos::byte, 32>::span;
};
struct SECP256K1SecretKeyRef : public std::span<const bcos::byte, 32>
{
    using std::span<const bcos::byte, 32>::span;
};

bcos::crypto::key::secp256k1::SECP256K1UncompressPublicKey tag_invoke(
    bcos::crypto::key::tag_t<bcos::crypto::key::secretKeyToPublicKey> const& /*unused*/,
    bcos::crypto::key::secp256k1::SECP256K1SecretKeyRef secretKey);

}  // namespace bcos::crypto::key::secp256k1


// SECP256K1SecretKey tag_invoke(keypair::tag_t<bcos::crypto::keypair::secretKey>, const
// bcos::crypto::Secp256k1KeyPair& keyPair); SECP256K1UncompressPublicKey tag_invoke(
//     keypair::tag_t<keypair::publicKey>, const Secp256k1KeyPair& keyPair);

// template <bool compress>
// auto tag_invoke(key::tag_t<key::serializePublicKey> /*unused*/, auto&& publicKey)
//     requires std::same_as<std::decay_t<decltype(publicKey)>, SECP256K1UncompressPublicKey> ||
//              std::same_as<std::decay_t<decltype(publicKey)>, SECP256K1CompressPublicKey>
// {
//     if constexpr ((compress && std::same_as<std::decay_t<decltype(publicKey)>,
//                                    SECP256K1CompressPublicKey>) ||
//                   (!compress && std::same_as<std::decay_t<decltype(publicKey)>,
//                                     SECP256K1UncompressPublicKey>))
//     {
//         return publicKey;
//     }

//     secp256k1_pubkey pubkey;
//     if (secp256k1_ec_pubkey_parse(
//             g_SECP256K1_CTX.get(), std::addressof(pubkey), publicKey.data(), publicKey.size()) ==
//             0)
//     {
//         BOOST_THROW_EXCEPTION(
//             PriToPublicKeyException() << errinfo_comment("secp256k1GenerateKeyPair exception"));
//     }

//     std::conditional_t<compress, SECP256K1CompressPublicKey, SECP256K1UncompressPublicKey>
//         serializePubkey;
//     size_t outSize = serializePubkey.size();
//     if (secp256k1_ec_pubkey_serialize(g_SECP256K1_CTX.get(), serializePubkey.data(),
//             std::addressof(outSize), std::addressof(pubkey),
//             compress ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED) == 0)
//     {
//         BOOST_THROW_EXCEPTION(
//             PriToPublicKeyException() << errinfo_comment("secp256k1GenerateKeyPair exception"));
//     }
//     return serializePubkey;
// }