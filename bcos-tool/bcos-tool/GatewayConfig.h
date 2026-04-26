#pragma once

#include <boost/property_tree/ptree_fwd.hpp>
#include <cstdint>
#include <string>
#include <utility>

namespace bcos::tool
{
class GatewayConfig
{
public:
    constexpr static uint16_t DEFAULT_LISTEN_PORT = 30300;

    const std::string& listenIP() const { return m_listenIP; }
    void setListenIP(std::string listenIP) { m_listenIP = std::move(listenIP); }

    uint16_t listenPort() const { return m_listenPort; }
    void setListenPort(uint16_t listenPort) { m_listenPort = listenPort; }

    bool smSsl() const { return m_smSsl; }
    void setSmSsl(bool smSsl) { m_smSsl = smSsl; }

    const std::string& nodeDir() const { return m_nodeDir; }
    void setNodeDir(std::string nodeDir) { m_nodeDir = std::move(nodeDir); }

    const std::string& nodeFileName() const { return m_nodeFileName; }
    void setNodeFileName(std::string nodeFileName) { m_nodeFileName = std::move(nodeFileName); }

    void loadGatewayConfig(boost::property_tree::ptree const& config);

private:
    std::string m_listenIP = "0.0.0.0";
    uint16_t m_listenPort = DEFAULT_LISTEN_PORT;
    bool m_smSsl = false;
    std::string m_nodeDir = "./";
    std::string m_nodeFileName = "nodes.json";
};
}  // namespace bcos::tool