#include "ServiceConfig.h"
#include "Exceptions.h"
#include "bcos-framework/protocol/ServiceDesc.h"
#include "bcos-utilities/BoostLog.h"
#include "bcos-utilities/Common.h"
#include "fisco-bcos-tars-service/Common/TarsUtils.h"
#include <bcos-framework/Common.h>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#define SERVICE_CONFIG_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("ServiceConfig")

using namespace bcos::tool;

namespace
{
std::string defaultServiceName(
    std::string const& chainID, std::string const& nodeName, std::string const& serviceName)
{
    return chainID + "." + nodeName + serviceName;
}
}

void ServiceConfig::loadWithoutTarsFrameworkConfig(
    boost::property_tree::ptree const& config, std::string const& defaultTarsProxyConf)
{
    setWithoutTarsFramework(config.get<bool>("service.without_tars_framework", false));
    setTarsProxyConf(config.get<std::string>("service.tars_proxy_conf", defaultTarsProxyConf));
}

void ServiceConfig::loadServiceConfig(
    boost::property_tree::ptree const& config, std::string const& defaultTarsProxyConf)
{
    auto rpcServiceName = getServiceName(config, "service.rpc", bcos::protocol::RPC_SERVANT_NAME);
    auto gatewayServiceName =
        getServiceName(config, "service.gateway", bcos::protocol::GATEWAY_SERVANT_NAME);
    setRpcServiceName(rpcServiceName);
    setGatewayServiceName(gatewayServiceName);
    loadWithoutTarsFrameworkConfig(config, defaultTarsProxyConf);

    SERVICE_CONFIG_LOG(INFO) << LOG_DESC("loadServiceConfig")
                             << LOG_KV("rpcServiceName", rpcServiceName)
                             << LOG_KV("gatewayServiceName", gatewayServiceName)
                             << LOG_KV("withoutTarsFramework", withoutTarsFramework());

    if (withoutTarsFramework())
    {
        loadTarsProxyConfig(tarsProxyConf());
    }
}

void ServiceConfig::loadNodeName(
    boost::property_tree::ptree const& config, std::string const& defaultNodeName)
{
    auto nodeName = config.get<std::string>("service.node_name", "");
    if (nodeName.empty())
    {
        nodeName = defaultNodeName;
    }
    setNodeName(std::move(nodeName));
}

void ServiceConfig::loadNodeServiceConfig(boost::property_tree::ptree const& config,
    std::string const& defaultNodeName, std::string const& chainID,
    std::string const& defaultTarsProxyConf, bool require)
{
    loadNodeName(config, defaultNodeName);
    auto const& loadedNodeName = nodeName();
    if (!isalNumStr(loadedNodeName))
    {
        BOOST_THROW_EXCEPTION(
            InvalidConfig() << errinfo_comment("The node name must be number or digit"));
    }

    loadWithoutTarsFrameworkConfig(config, defaultTarsProxyConf);

    SERVICE_CONFIG_LOG(INFO) << LOG_DESC("loadNodeServiceConfig")
                             << LOG_KV("withoutTarsFramework", withoutTarsFramework());

    if (withoutTarsFramework())
    {
        loadTarsProxyConfig(tarsProxyConf());
    }

    auto schedulerServiceName = getServiceName(config, "service.scheduler",
        bcos::protocol::SCHEDULER_SERVANT_NAME,
        defaultServiceName(chainID, loadedNodeName, bcos::protocol::SCHEDULER_SERVICE_NAME),
        require);
    auto executorServiceName = getServiceName(config, "service.executor",
        bcos::protocol::EXECUTOR_SERVANT_NAME,
        defaultServiceName(chainID, loadedNodeName, bcos::protocol::EXECUTOR_SERVICE_NAME),
        require);
    auto txpoolServiceName = getServiceName(config, "service.txpool",
        bcos::protocol::TXPOOL_SERVANT_NAME,
        defaultServiceName(chainID, loadedNodeName, bcos::protocol::TXPOOL_SERVICE_NAME),
        require);
    setSchedulerServiceName(schedulerServiceName);
    setExecutorServiceName(executorServiceName);
    setTxpoolServiceName(txpoolServiceName);

    SERVICE_CONFIG_LOG(INFO) << LOG_DESC("load node service")
                             << LOG_KV("nodeName", loadedNodeName)
                             << LOG_KV("withoutTarsFramework", withoutTarsFramework())
                             << LOG_KV("schedulerServiceName", schedulerServiceName)
                             << LOG_KV("executorServiceName", executorServiceName);
}

std::string ServiceConfig::getServiceName(boost::property_tree::ptree const& config,
    std::string const& configSection, std::string_view objectName,
    std::string const& defaultValue, bool require)
{
    auto serviceName = config.get<std::string>(configSection, defaultValue);
    if (!require)
    {
        return serviceName;
    }

    checkService(configSection, serviceName);
    return bcos::protocol::getPrxDesc(serviceName, std::string(objectName));
}

void ServiceConfig::checkService(std::string const& serviceType, std::string const& serviceName)
{
    if (serviceName.empty())
    {
        BOOST_THROW_EXCEPTION(
            InvalidConfig() << errinfo_comment("Must set service name for " + serviceType + "!"));
    }

    std::vector<std::string> serviceNameList;
    boost::split(serviceNameList, serviceName, boost::is_any_of("."));
    std::string errorMsg =
        "Must set service name in format of application_name.server_name with only include letters "
        "and numbers for " +
        serviceType + ", invalid config now is:" + serviceName;
    if (serviceNameList.size() != 2)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(errorMsg));
    }

    for (auto const& serviceNamePart : serviceNameList)
    {
        if (!isalNumStr(serviceNamePart))
        {
            BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(errorMsg));
        }
    }
}

void ServiceConfig::loadTarsProxyConfig(std::string const& tarsProxyConf)
{
    if (!m_tarsSN2EndPoints.empty())
    {
        SERVICE_CONFIG_LOG(INFO) << LOG_BADGE("loadTarsProxyConfig")
                                 << LOG_DESC("tars proxy config has been loaded");
        return;
    }

    boost::property_tree::ptree proxyTree;
    try
    {
        boost::property_tree::read_ini(tarsProxyConf, proxyTree);

        loadServiceTarsProxyConfig("front", proxyTree);
        loadServiceTarsProxyConfig("rpc", proxyTree);
        loadServiceTarsProxyConfig("gateway", proxyTree);
        loadServiceTarsProxyConfig("executor", proxyTree);
        loadServiceTarsProxyConfig("txpool", proxyTree);
        loadServiceTarsProxyConfig("scheduler", proxyTree);
        loadServiceTarsProxyConfig("pbft", proxyTree);
        loadServiceTarsProxyConfig("ledger", proxyTree);

        SERVICE_CONFIG_LOG(INFO) << LOG_BADGE("loadTarsProxyConfig")
                                 << LOG_KV("tars service endpoints size", m_tarsSN2EndPoints.size());
    }
    catch (std::exception const& e)
    {
        SERVICE_CONFIG_LOG(ERROR) << LOG_BADGE("loadTarsProxyConfig")
                                  << LOG_DESC("load tars proxy config failed")
                                  << LOG_KV("e", e.what())
                                  << LOG_KV("tarsProxyConf", tarsProxyConf);

        BOOST_THROW_EXCEPTION(InvalidParameter() << errinfo_comment(
                                  "Load tars proxy config failed, e: " + std::string(e.what())));
    }
}

void ServiceConfig::loadServiceTarsProxyConfig(
    std::string const& serviceSectionName, boost::property_tree::ptree const& config)
{
    if (!config.get_child_optional(serviceSectionName))
    {
        SERVICE_CONFIG_LOG(WARNING) << LOG_BADGE("loadServiceTarsProxyConfig")
                                    << LOG_DESC("service name not exist")
                                    << LOG_KV("serviceName", serviceSectionName);
        return;
    }

    for (auto const& item : config.get_child(serviceSectionName))
    {
        if (item.first.find("proxy.") != 0)
        {
            continue;
        }

        auto endpoint = bcostars::string2TarsEndPoint(item.second.data());
        m_tarsSN2EndPoints[serviceSectionName].push_back(endpoint);

        SERVICE_CONFIG_LOG(INFO) << LOG_BADGE("loadTarsProxyConfig") << LOG_DESC("add element")
                                 << LOG_KV("serviceName", serviceSectionName)
                                 << LOG_KV("endpoint", endpoint.toString());
    }

    SERVICE_CONFIG_LOG(INFO) << LOG_BADGE("loadTarsProxyConfig")
                             << LOG_KV("serviceName", serviceSectionName)
                             << LOG_KV("endpoints size", m_tarsSN2EndPoints[serviceSectionName].size());
}

void ServiceConfig::getTarsClientProxyEndpoints(
    std::string const& clientPrx, std::vector<tars::TC_Endpoint>& endpoints) const
{
    if (!withoutTarsFramework())
    {
        SERVICE_CONFIG_LOG(TRACE) << LOG_BADGE("getTarsClientProxyEndpoints")
                                  << "not work with tars rpc"
                                  << LOG_KV("withoutTarsFramework", withoutTarsFramework());
        return;
    }

    endpoints.clear();

    auto it = m_tarsSN2EndPoints.find(boost::to_lower_copy(clientPrx));
    if (it != m_tarsSN2EndPoints.end())
    {
        endpoints = it->second;

        SERVICE_CONFIG_LOG(DEBUG) << LOG_BADGE("getTarsClientProxyEndpoints")
                                  << LOG_DESC("find tars client proxy endpoints")
                                  << LOG_KV("serviceName", clientPrx)
                                  << LOG_KV("endpoints size", endpoints.size());
    }

    if (endpoints.empty())
    {
        SERVICE_CONFIG_LOG(WARNING) << LOG_BADGE("getTarsClientProxyEndpoints")
                                    << LOG_DESC("can not find tars client proxy endpoints")
                                    << LOG_KV("serviceName", clientPrx);

        BOOST_THROW_EXCEPTION(InvalidParameter() << errinfo_comment(
                                  "Can't find tars client proxy endpoints, serviceName : " +
                                  clientPrx));
    }
}
