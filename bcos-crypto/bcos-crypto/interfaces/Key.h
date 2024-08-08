#pragma once

#include <span>
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
} serializePublicKey{};

inline constexpr struct DeserializePublicKey
{
    template <class Buffer>
    auto operator()(Buffer const& buffer) const
    {
        return tag_invoke(*this, buffer);
    }
} deserializePublicKey{};

inline constexpr struct SecretKeyToPublicKey
{
    auto operator()(auto const& secretKey) const { return tag_invoke(*this, secretKey); }
} secretKeyToPublicKey{};

template <class SecretKeyType>
concept SecretKey = requires(SecretKeyType&& secretKey) {
    { secretKeyToPublicKey(secretKey) };
};

template <class PublicKeyType>
concept PublicKey = requires(PublicKeyType&& publicKey, std::span<unsigned char> buffer) {
    { serializePublicKey(publicKey) };
    // { deserializePublicKey(buffer) } -> std::convertible_to<bool>;
};

template <auto& Tag>
using tag_t = std::decay_t<decltype(Tag)>;
}  // namespace bcos::crypto::key
