#include "SealerConfig.h"
#include <boost/property_tree/ptree.hpp>

using namespace bcos::tool;

void SealerConfig::loadSealerConfig(boost::property_tree::ptree const& config)
{
    setMinSealTime(static_cast<size_t>(
        config.get<int64_t>("consensus.min_seal_time", DEFAULT_MIN_SEAL_TIME_MS)));
}
