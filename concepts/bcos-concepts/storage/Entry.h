#pragma once
#include "../Hash.h"
#include "../Serialize.h"

namespace bcos::concepts::storage
{

template <class EntryType>
concept Entry = requires(EntryType entry, unsigned index)
{
    !entry;
    entry[index];
    serialize::Serializable<EntryType>;
    hash::Hashable<EntryType>;
};

}  // namespace bcos::concepts::storage