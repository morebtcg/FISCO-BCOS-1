#pragma once

#include <boost/property_tree/ptree_fwd.hpp>
#include <cstddef>

namespace bcos::tool
{
class ConsensusConfig
{
public:
    size_t checkpointTimeoutInterval() const { return m_checkpointTimeoutInterval; }
    void setCheckpointTimeoutInterval(size_t checkpointTimeoutInterval)
    {
        m_checkpointTimeoutInterval = checkpointTimeoutInterval;
    }

    size_t pipelineSize() const { return m_pipelineSize; }
    void setPipelineSize(size_t pipelineSize) { m_pipelineSize = pipelineSize; }

    void loadConsensusConfig(
        boost::property_tree::ptree const& config, size_t minConsensusTimeMs, size_t minPipelineSize);

private:
    size_t m_checkpointTimeoutInterval = 0;
    size_t m_pipelineSize = 0;
};
}  // namespace bcos::tool