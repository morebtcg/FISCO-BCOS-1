#pragma once

#include <type_traits>
namespace bcos::concepts::serialize
{

inline constexpr struct Encode
{
    template <class Object, class Buffer>
    void operator()(const Object& object, Buffer& out) const
    {
        tag_invoke(*this, object, out);
    }
} encode{};

inline constexpr struct Decode
{
    template <class Buffer, class Object>
    void operator()(Buffer const& input, Object& object) const
    {
        tag_invoke(*this, input, object);
    }
} decode{};

template <auto& Tag>
using tag_t = std::decay_t<decltype(Tag)>;

}  // namespace bcos::concepts::serialize
