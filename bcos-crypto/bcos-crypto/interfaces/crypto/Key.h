#pragma once

#include <type_traits>

namespace bcos::crypto::key
{

inline constexpr struct SerializePublicKey
{
    template <bool compress>
    auto operator()(auto const& publicKey) const
    {
        return tag_invoke<compress>(*this, publicKey);
    }
} serializePublicKey;

inline constexpr struct SecretKeyToPublicKey
{
    auto operator()(auto const& secretKey) const { return tag_invoke(*this, secretKey); }
} secretKeyToPublicKey;

template <class SecretKeyType>
concept SecretKey = requires(SecretKeyType&& secretKey) {
    { secretKeyToPublicKey(secretKey) };
};

template <class PublicKeyType>
concept PublicKey = requires(PublicKeyType&& publicKey) {
    { serializePublicKey(publicKey) };
};

template <auto& Tag>
using tag_t = std::decay_t<decltype(Tag)>;
}  // namespace bcos::crypto::key
