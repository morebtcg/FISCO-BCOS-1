#pragma once

#include "../impl/TarsSerializable.h"
#include "bcos-crypto/interfaces/crypto/KeyFactory.h"
#include "bcos-crypto/interfaces/crypto/KeyInterface.h"
#include "bcos-framework/consensus/ConsensusNodeInterface.h"
#include "bcos-tars-protocol/tars/ConsensusNode.h"

namespace bcostars::protocol
{
class ConsensusNodeImpl : public bcos::consensus::ConsensusNodeInterface
{
private:
    bcostars::ConsensusNode m_inner;
    std::reference_wrapper<bcos::crypto::KeyFactory> m_keyFactory;

public:
    ConsensusNodeImpl(bcos::crypto::KeyFactory& keyFactory) : m_keyFactory(keyFactory){};
    void encode(bcos::bytes& output) const { bcos::concepts::serialize::encode(m_inner, output); };
    void decode(const bcos::bytesConstRef input)
    {
        bcos::concepts::serialize::decode(input, m_inner);
    }

    bcos::crypto::PublicPtr nodeID() const override
    {
        return m_keyFactory.get().createKey(
            bcos::bytesConstRef{(const bcos::byte*)m_inner.nodeID.data(), m_inner.nodeID.size()});
    };
    uint64_t voteWeight() const override { return m_inner.voteWeight; }
    uint64_t termWeight() const override { return m_inner.termWeight; }
};
}  // namespace bcostars::protocol