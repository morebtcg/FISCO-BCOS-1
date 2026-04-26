#pragma once

#include <boost/property_tree/ptree_fwd.hpp>
#include <string>
#include <utility>

namespace bcos::tool
{
class CertConfig
{
public:
    std::string const& certPath() const { return m_certPath; }
    void setCertPath(std::string certPath) { m_certPath = std::move(certPath); }

    std::string const& caCert() const { return m_caCert; }
    void setCaCert(std::string caCert) { m_caCert = std::move(caCert); }

    std::string const& nodeCert() const { return m_nodeCert; }
    void setNodeCert(std::string nodeCert) { m_nodeCert = std::move(nodeCert); }

    std::string const& nodeKey() const { return m_nodeKey; }
    void setNodeKey(std::string nodeKey) { m_nodeKey = std::move(nodeKey); }

    std::string const& smCaCert() const { return m_smCaCert; }
    void setSmCaCert(std::string smCaCert) { m_smCaCert = std::move(smCaCert); }

    std::string const& smNodeCert() const { return m_smNodeCert; }
    void setSmNodeCert(std::string smNodeCert) { m_smNodeCert = std::move(smNodeCert); }

    std::string const& smNodeKey() const { return m_smNodeKey; }
    void setSmNodeKey(std::string smNodeKey) { m_smNodeKey = std::move(smNodeKey); }

    std::string const& enSmNodeCert() const { return m_enSmNodeCert; }
    void setEnSmNodeCert(std::string enSmNodeCert) { m_enSmNodeCert = std::move(enSmNodeCert); }

    std::string const& enSmNodeKey() const { return m_enSmNodeKey; }
    void setEnSmNodeKey(std::string enSmNodeKey) { m_enSmNodeKey = std::move(enSmNodeKey); }

    void loadCertConfig(boost::property_tree::ptree const& config);

private:
    std::string m_certPath;
    std::string m_caCert;
    std::string m_nodeCert;
    std::string m_nodeKey;
    std::string m_smCaCert;
    std::string m_smNodeCert;
    std::string m_smNodeKey;
    std::string m_enSmNodeCert;
    std::string m_enSmNodeKey;
};
}  // namespace bcos::tool