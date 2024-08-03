#pragma once

#include <type_traits>

namespace bcos::crypto::keypair
{

inline constexpr struct SecretKey
{
    auto operator()(auto const& keyPair) const { return tag_invoke(*this, keyPair); }
} secretKey;

inline constexpr struct PublicKey
{
    auto operator()(auto const& keyPair) const { return tag_invoke(*this, keyPair); }
} publicKey;

template <class KeyPairType>
concept KeyPair = requires(KeyPairType&& keyPair) {
    { secretKey(keyPair) };
    { publicKey(keyPair) };
};

template <auto& Tag>
using tag_t = std::decay_t<decltype(Tag)>;
}  // namespace bcos::crypto::keypair