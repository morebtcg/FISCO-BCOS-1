#pragma once

#include <boost/property_tree/ptree_fwd.hpp>
#include <util/tc_clientsocket.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bcos::tool
{
class ServiceConfig
{
public:
    std::string const& rpcServiceName() const { return m_rpcServiceName; }
    void setRpcServiceName(std::string rpcServiceName)
    {
        m_rpcServiceName = std::move(rpcServiceName);
    }

    std::string const& gatewayServiceName() const { return m_gatewayServiceName; }
    void setGatewayServiceName(std::string gatewayServiceName)
    {
        m_gatewayServiceName = std::move(gatewayServiceName);
    }

    std::string const& schedulerServiceName() const { return m_schedulerServiceName; }
    void setSchedulerServiceName(std::string schedulerServiceName)
    {
        m_schedulerServiceName = std::move(schedulerServiceName);
    }

    std::string const& executorServiceName() const { return m_executorServiceName; }
    void setExecutorServiceName(std::string executorServiceName)
    {
        m_executorServiceName = std::move(executorServiceName);
    }

    std::string const& txpoolServiceName() const { return m_txpoolServiceName; }
    void setTxpoolServiceName(std::string txpoolServiceName)
    {
        m_txpoolServiceName = std::move(txpoolServiceName);
    }

    std::string const& nodeName() const { return m_nodeName; }
    void setNodeName(std::string nodeName) { m_nodeName = std::move(nodeName); }

    std::string const& tarsProxyConf() const { return m_tarsProxyConf; }
    void setTarsProxyConf(std::string tarsProxyConf)
    {
        m_tarsProxyConf = std::move(tarsProxyConf);
    }

    bool withoutTarsFramework() const { return m_withoutTarsFramework; }
    void setWithoutTarsFramework(bool withoutTarsFramework)
    {
        m_withoutTarsFramework = withoutTarsFramework;
    }

    void loadWithoutTarsFrameworkConfig(
        boost::property_tree::ptree const& config, std::string const& defaultTarsProxyConf);
    void loadServiceConfig(boost::property_tree::ptree const& config,
        std::string const& defaultTarsProxyConf = "./tars_proxy.ini");
    void loadNodeName(
        boost::property_tree::ptree const& config, std::string const& defaultNodeName);
    void loadNodeServiceConfig(boost::property_tree::ptree const& config,
        std::string const& defaultNodeName, std::string const& chainID,
        std::string const& defaultTarsProxyConf = "conf/tars_proxy.ini", bool require = false);
    static std::string getServiceName(boost::property_tree::ptree const& config,
        std::string const& configSection, std::string_view objectName,
        std::string const& defaultValue = "", bool require = true);
    static void checkService(std::string const& serviceType, std::string const& serviceName);
    void loadTarsProxyConfig(std::string const& tarsProxyConf);
    void loadServiceTarsProxyConfig(
        std::string const& serviceSectionName, boost::property_tree::ptree const& config);
    void getTarsClientProxyEndpoints(
        std::string const& clientPrx, std::vector<tars::TC_Endpoint>& endpoints) const;

private:
    std::string m_rpcServiceName;
    std::string m_gatewayServiceName;
    std::string m_schedulerServiceName;
    std::string m_executorServiceName;
    std::string m_txpoolServiceName;
    std::string m_nodeName;
    std::string m_tarsProxyConf;
    bool m_withoutTarsFramework = false;
    std::unordered_map<std::string, std::vector<tars::TC_Endpoint>> m_tarsSN2EndPoints;
};
}  // namespace bcos::tool