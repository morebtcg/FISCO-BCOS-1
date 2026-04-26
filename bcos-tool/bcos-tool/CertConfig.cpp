#include "CertConfig.h"
#include <boost/property_tree/ptree.hpp>

using namespace bcos::tool;

void CertConfig::loadCertConfig(boost::property_tree::ptree const& config)
{
    auto certPath = config.get<std::string>("cert.ca_path", "./");

    auto smCaCertFile = certPath + "/" + config.get<std::string>("cert.sm_ca_cert", "sm_ca.crt");
    auto smNodeCertFile =
        certPath + "/" + config.get<std::string>("cert.sm_node_cert", "sm_ssl.crt");
    auto smNodeKeyFile =
        certPath + "/" + config.get<std::string>("cert.sm_node_key", "sm_ssl.key");
    auto smEnNodeCertFile =
        certPath + "/" + config.get<std::string>("cert.sm_ennode_cert", "sm_enssl.crt");
    auto smEnNodeKeyFile =
        certPath + "/" + config.get<std::string>("cert.sm_ennode_key", "sm_enssl.key");

    auto caCertFile = certPath + "/" + config.get<std::string>("cert.ca_cert", "ca.crt");
    auto nodeCertFile = certPath + "/" + config.get<std::string>("cert.node_cert", "ssl.crt");
    auto nodeKeyFile = certPath + "/" + config.get<std::string>("cert.node_key", "ssl.key");

    setCertPath(certPath);
    setSmCaCert(smCaCertFile);
    setSmNodeCert(smNodeCertFile);
    setSmNodeKey(smNodeKeyFile);
    setEnSmNodeCert(smEnNodeCertFile);
    setEnSmNodeKey(smEnNodeKeyFile);
    setCaCert(caCertFile);
    setNodeCert(nodeCertFile);
    setNodeKey(nodeKeyFile);
}
