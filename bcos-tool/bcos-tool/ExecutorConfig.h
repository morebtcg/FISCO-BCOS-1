#pragma once

#include <boost/property_tree/ptree_fwd.hpp>
#include <cstdint>
#include <string>
#include <utility>

namespace bcos::tool
{
class ExecutorConfig
{
public:
    bool isWasm() const { return m_isWasm; }
    void setIsWasm(bool isWasm) { m_isWasm = isWasm; }

    bool isAuthCheck() const { return m_isAuthCheck; }
    void setIsAuthCheck(bool isAuthCheck) { m_isAuthCheck = isAuthCheck; }

    bool isSerialExecute() const { return m_isSerialExecute; }
    void setIsSerialExecute(bool isSerialExecute) { m_isSerialExecute = isSerialExecute; }

    int executorVersion() const { return m_executorVersion; }
    void setExecutorVersion(int executorVersion) { m_executorVersion = executorVersion; }

    std::string const& authAdminAccount() const { return m_authAdminAccount; }
    void setAuthAdminAccount(std::string authAdminAccount)
    {
        m_authAdminAccount = std::move(authAdminAccount);
    }

    bool enableDag() const { return m_enableDag; }
    void setEnableDag(bool enableDag) { m_enableDag = enableDag; }

    void loadGenesisExecutorConfig(
        boost::property_tree::ptree const& config, std::uint32_t compatibilityVersion);
    void loadExecutorNormalConfig(boost::property_tree::ptree const& config);

private:
    bool m_isWasm = false;
    bool m_isAuthCheck = true;
    std::string m_authAdminAccount;
    bool m_isSerialExecute = true;
    int m_executorVersion = 0;
    bool m_enableDag = true;
};
}  // namespace bcos::tool
