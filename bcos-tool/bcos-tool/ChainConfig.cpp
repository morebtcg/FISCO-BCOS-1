#include "ChainConfig.h"
#include "Exceptions.h"
#include "bcos-utilities/Common.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/throw_exception.hpp>

using namespace bcos::tool;

namespace
{
constexpr std::int64_t DEFAULT_BLOCK_LIMIT = 1000;
}

void ChainConfig::loadChainConfig(
    boost::property_tree::ptree const& config, bool enforceGroupId, std::int64_t maxBlockLimit)
{
    try
    {
        setSmCrypto(config.get<bool>("chain.sm_crypto", false));
        if (enforceGroupId)
        {
            setGroupID(config.get<std::string>("chain.group_id", "group"));
        }
        setChainID(config.get<std::string>("chain.chain_id", "chain"));
    }
    catch (std::exception const&)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "chain.sm_crypto/chain.group_id/chain.chain_id is null, please set it,"
                                  " if compatibility_version in genesis block >= 3.1.0,"
                                  " 'chain' config should appear in config.genesis, else in config.ini."));
    }
    if (!isalNumStr(chainID()))
    {
        BOOST_THROW_EXCEPTION(
            InvalidConfig() << errinfo_comment("The chainId must be number or digit"));
    }

    auto blockLimit = config.get<std::int64_t>("chain.block_limit", DEFAULT_BLOCK_LIMIT);
    if (blockLimit <= 0 || blockLimit > maxBlockLimit)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "Please set chain.block_limit to positive and less than " +
                                  std::to_string(maxBlockLimit) + " !"));
    }
    setBlockLimit(blockLimit);
}

void ChainConfig::loadWeb3ChainConfig(boost::property_tree::ptree const& config)
{
    setWeb3ChainID(config.get<std::string>("web3.chain_id", "0"));
    if (!isNumStr(web3ChainID()))
    {
        BOOST_THROW_EXCEPTION(
            InvalidConfig() << errinfo_comment("The web3ChainId must be number string"));
    }
}
