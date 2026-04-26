#include "FailOverConfig.h"
#include "Exceptions.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/throw_exception.hpp>

using namespace bcos::tool;

void FailOverConfig::loadFailOverConfig(boost::property_tree::ptree const& config,
    bool enforceMemberID, unsigned defaultLeaseTTLSeconds)
{
    auto enableFailOver = config.get("failover.enable", false);
    setEnable(enableFailOver);
    if (!enableFailOver)
    {
        setClusterUrl("");
        setMemberID("");
        setLeaseTTL(0);
        return;
    }

    auto failOverClusterUrl = config.get<std::string>("failover.cluster_url", "127.0.0.1:2379");
    auto memberID = config.get("failover.member_id", std::string{});
    if (memberID.empty() && enforceMemberID)
    {
        BOOST_THROW_EXCEPTION(
            InvalidConfig() << errinfo_comment("Please set failover.member_id must be non-empty "));
    }
    auto leaseTTL = config.get<unsigned>("failover.lease_ttl", defaultLeaseTTLSeconds);
    if (leaseTTL < defaultLeaseTTLSeconds)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "Please set failover.lease_ttl to no less than " +
                                  std::to_string(defaultLeaseTTLSeconds) + " seconds!"));
    }

    setClusterUrl(failOverClusterUrl);
    setMemberID(memberID);
    setLeaseTTL(leaseTTL);
}
