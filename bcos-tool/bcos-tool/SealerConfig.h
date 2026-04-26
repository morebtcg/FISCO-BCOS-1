#pragma once

#include <boost/property_tree/ptree_fwd.hpp>
#include <cstddef>

namespace bcos::tool
{
class SealerConfig
{
public:
    constexpr static size_t DEFAULT_MIN_SEAL_TIME_MS = 500;

    size_t minSealTime() const { return m_minSealTime; }
    void setMinSealTime(size_t minSealTime) { m_minSealTime = minSealTime; }

    void loadSealerConfig(boost::property_tree::ptree const& config);

private:
    size_t m_minSealTime = 0;
};
}  // namespace bcos::tool