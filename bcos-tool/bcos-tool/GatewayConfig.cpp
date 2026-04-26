#include "GatewayConfig.h"
#include <boost/property_tree/ptree.hpp>

using namespace bcos::tool;

void GatewayConfig::loadGatewayConfig(boost::property_tree::ptree const& config)
{
    auto listenIP = config.get<std::string>("p2p.listen_ip", "0.0.0.0");
    auto listenPort = config.get<int>("p2p.listen_port", DEFAULT_LISTEN_PORT);
    auto nodesDir = config.get<std::string>("p2p.nodes_path", "./");
    auto nodesFile = config.get<std::string>("p2p.nodes_file", "nodes.json");
    auto smSsl = config.get<bool>("p2p.sm_ssl", false);

    setListenIP(listenIP);
    setListenPort(static_cast<uint16_t>(listenPort));
    setNodeDir(nodesDir);
    setSmSsl(smSsl);
    setNodeFileName(nodesFile);
}
