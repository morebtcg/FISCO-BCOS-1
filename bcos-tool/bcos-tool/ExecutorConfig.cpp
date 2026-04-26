#include "ExecutorConfig.h"
#include "Exceptions.h"
#include "bcos-framework/Common.h"
#include "bcos-framework/protocol/Protocol.h"
#include "bcos-utilities/BoostLog.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/throw_exception.hpp>

using namespace bcos::tool;

void ExecutorConfig::loadGenesisExecutorConfig(
    boost::property_tree::ptree const& config, std::uint32_t compatibilityVersion)
{
    try
    {
        setIsWasm(config.get<bool>("executor.is_wasm", false));
        setIsAuthCheck(config.get<bool>("executor.is_auth_check", false));
        setIsSerialExecute(config.get<bool>("executor.is_serial_execute", false));
        setExecutorVersion(config.get<int>("executor.version", 0));
    }
    catch (std::exception const&)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "executor.is_wasm/executor.is_auth_check/"
                                  "executor.is_serial_execute is null, please set it!"));
    }

    if (isWasm() && !isSerialExecute())
    {
        if (compatibilityVersion >=
            static_cast<std::uint32_t>(bcos::protocol::BlockVersion::V3_1_VERSION))
        {
            BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                      "loadExecutorConfig wasm only support serial executing, "
                                      "please set is_serial_execute to true"));
        }
        BCOS_LOG(WARNING) << METRIC
                          << LOG_DESC(
                                 "loadExecutorConfig wasm with serial executing is not recommended");
    }
    if (isWasm() && isAuthCheck())
    {
        if (compatibilityVersion >=
            static_cast<std::uint32_t>(bcos::protocol::BlockVersion::V3_1_VERSION))
        {
            BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                      "loadExecutorConfig auth only support solidity, "
                                      "please set is_auth_check to false or set is_wasm to false"));
        }
        BCOS_LOG(WARNING) << METRIC
                          << LOG_DESC("loadExecutorConfig wasm auth is not supported for now");
    }

    try
    {
        setAuthAdminAccount(config.get<std::string>("executor.auth_admin_account", ""));
        if (authAdminAccount().empty() &&
            (isAuthCheck() ||
                compatibilityVersion >=
                    static_cast<std::uint32_t>(bcos::protocol::BlockVersion::V3_3_VERSION)))
            [[unlikely]]
        {
            BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                      "executor.auth_admin_account is empty, "
                                      "please set correct auth_admin_account"));
        }
    }
    catch (std::exception const&)
    {
        if (isAuthCheck() ||
            compatibilityVersion >=
                static_cast<std::uint32_t>(bcos::protocol::BlockVersion::V3_3_VERSION))
        {
            BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                      "executor.auth_admin_account is null, "
                                      "please set correct auth_admin_account"));
        }
    }
}

void ExecutorConfig::loadExecutorNormalConfig(boost::property_tree::ptree const& config)
{
    setEnableDag(config.get<bool>("executor.enable_dag", true));
}
