#pragma once

#include <boost/property_tree/ptree_fwd.hpp>
#include <cstdint>
#include <string>
#include <utility>

namespace bcos::tool
{
class ChainConfig
{
public:
    bool smCrypto() const { return m_smCrypto; }
    void setSmCrypto(bool smCrypto) { m_smCrypto = smCrypto; }

    std::string const& chainID() const { return m_chainID; }
    void setChainID(std::string chainID) { m_chainID = std::move(chainID); }

    std::string const& groupID() const { return m_groupID; }
    void setGroupID(std::string groupID) { m_groupID = std::move(groupID); }

    std::string const& web3ChainID() const { return m_web3ChainID; }
    void setWeb3ChainID(std::string web3ChainID) { m_web3ChainID = std::move(web3ChainID); }

    std::int64_t blockLimit() const { return m_blockLimit; }
    void setBlockLimit(std::int64_t blockLimit) { m_blockLimit = blockLimit; }

    void loadChainConfig(boost::property_tree::ptree const& config, bool enforceGroupId,
        std::int64_t maxBlockLimit);
    void loadWeb3ChainConfig(boost::property_tree::ptree const& config);

private:
    bool m_smCrypto = false;
    std::string m_chainID;
    std::string m_groupID;
    std::string m_web3ChainID;
    std::int64_t m_blockLimit = 0;
};
}  // namespace bcos::tool
