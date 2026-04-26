#include "ConsensusConfig.h"
#include "Exceptions.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/throw_exception.hpp>

using namespace bcos::tool;

void ConsensusConfig::loadConsensusConfig(
    boost::property_tree::ptree const& config, size_t minConsensusTimeMs, size_t minPipelineSize)
{
    auto checkPointTimeoutInterval =
        config.get<int64_t>("consensus.checkpoint_timeout", static_cast<int64_t>(minConsensusTimeMs));
    auto pipelineSize =
        config.get<int64_t>("consensus.pipeline_size", static_cast<int64_t>(minPipelineSize));
    if (checkPointTimeoutInterval < static_cast<int64_t>(minConsensusTimeMs))
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "Please set consensus.checkpoint_timeout to no less than " +
                                  std::to_string(minConsensusTimeMs) + "ms!"));
    }
    if (pipelineSize < static_cast<int64_t>(minPipelineSize))
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "Please set consensus.pipeline_size to no less than " +
                                  std::to_string(minPipelineSize)));
    }

    setCheckpointTimeoutInterval(static_cast<size_t>(checkPointTimeoutInterval));
    setPipelineSize(static_cast<size_t>(pipelineSize));
}
