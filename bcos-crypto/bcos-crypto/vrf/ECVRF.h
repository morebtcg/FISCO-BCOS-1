#pragma once

#include "../interfaces/crypto/Key.h"
#include "bcos-crypto/interfaces/crypto/KeyPairInterface.h"
#include "bcos-crypto/signature/secp256k1/Secp256k1KeyPair.h"
#include <secp256k1.h>
#include <cstdlib>
#include <cstring>

namespace bcos::crypto::vrf
{

using VRFProof = std::array<bcos::byte, 81>;

inline VRFProof vrfProve(KeyPairInterface const& keyPair, bytesConstRef message)
{
    VRFProof vrfProof;

    return vrfProof;
}

static auto vrf_hash_to_curve_tai(auto const& publicKey, bytesConstRef message)
{
    unsigned char ctr = 0;

    auto pk_string = key::serializePublicKey.template operator()<true>(publicKey);

    auto full_len = 1 + 1 + pk_string.size() + message.size() + 1;
    std::vector<unsigned char> full_string;
    full_string.reserve(full_len);

    full_string.push_back(0xfe);
    full_string.push_back(0x01);
    full_string.insert(full_string.end(), pk_string.begin(), pk_string.end());
    full_string.insert(full_string.end(), message.begin(), message.end());

    for (;; ++ctr)
    {
        /* the first byte used by the inplace arbitrary_string_to_point() */
        std::array<unsigned char, 33> hash_string;
        /* update the ctr directly on the string */
        full_string.back() = ctr;
        /* calculate the hash_string */
        sha256(&hash_string[1], full_string, full_len);
        /* arbitrary_string_to_point (inplace) */
        hash_string[0] = 0x02;

        // std::array<unsigned char, 33> point;
        secp256k1_pubkey point;
        if (key::deserializePublicKey(hash_string))
        {
        }
        if (string_to_point(point, hash_string) && !secp256k1_ge_is_infinity(point))
        {
            VRF_DEBUG_PRINT(("try_and_increment succeded on ctr = %d\n", ctr));
            free(full_string);
            return 1;
        }
    }
}
}  // namespace bcos::crypto::vrf