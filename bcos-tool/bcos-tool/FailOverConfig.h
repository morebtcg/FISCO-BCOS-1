#pragma once

#include <boost/property_tree/ptree_fwd.hpp>
#include <string>
#include <utility>

namespace bcos::tool
{
class FailOverConfig
{
public:
    bool enable() const { return m_enable; }
    void setEnable(bool enable) { m_enable = enable; }

    std::string const& clusterUrl() const { return m_clusterUrl; }
    void setClusterUrl(std::string clusterUrl) { m_clusterUrl = std::move(clusterUrl); }

    std::string const& memberID() const { return m_memberID; }
    void setMemberID(std::string memberID) { m_memberID = std::move(memberID); }

    unsigned leaseTTL() const { return m_leaseTTL; }
    void setLeaseTTL(unsigned leaseTTL) { m_leaseTTL = leaseTTL; }

    void loadFailOverConfig(boost::property_tree::ptree const& config, bool enforceMemberID,
        unsigned defaultLeaseTTLSeconds);

private:
    bool m_enable = false;
    std::string m_clusterUrl;
    std::string m_memberID;
    unsigned m_leaseTTL = 0;
};
}  // namespace bcos::tool