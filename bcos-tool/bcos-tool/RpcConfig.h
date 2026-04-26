#pragma once
#include <boost/property_tree/ptree_fwd.hpp>
#include <string>
#include <cstdint>

namespace bcos::tool
{
class RpcConfig
{
public:
    constexpr static uint16_t DEFAULT_LISTEN_PORT = 20200;
    constexpr static uint32_t DEFAULT_THREAD_POOL_SIZE = 8;
    constexpr static uint32_t DEFAULT_FILTER_TIMEOUT_MS = 300000;
    constexpr static uint32_t DEFAULT_MAX_PROCESS_BLOCK = 10;

    RpcConfig() = default;

    const std::string& listenIP() const { return m_listenIP; }
    uint16_t listenPort() const { return m_listenPort; }
    uint32_t threadPoolSize() const { return m_threadPoolSize; }
    uint32_t filterTimeout() const { return m_filterTimeout; }
    uint32_t maxProcessBlock() const { return m_maxProcessBlock; }
    bool smSsl() const { return m_smSsl; }
    bool disableSsl() const { return m_disableSsl; }

    void setListenIP(const std::string& listenIP) { m_listenIP = listenIP; }
    void setListenPort(uint16_t listenPort) { m_listenPort = listenPort; }
    void setThreadPoolSize(uint32_t threadPoolSize) { m_threadPoolSize = threadPoolSize; }
    void setFilterTimeout(uint32_t filterTimeout) { m_filterTimeout = filterTimeout; }
    void setMaxProcessBlock(uint32_t maxProcessBlock) { m_maxProcessBlock = maxProcessBlock; }
    void setSmSsl(bool smSsl) { m_smSsl = smSsl; }
    void setDisableSsl(bool disableSsl) { m_disableSsl = disableSsl; }

    bool loadRpcConfig(boost::property_tree::ptree const& config);

private:
    std::string m_listenIP = "0.0.0.0";
    uint16_t m_listenPort = DEFAULT_LISTEN_PORT;
    uint32_t m_threadPoolSize = DEFAULT_THREAD_POOL_SIZE;
    uint32_t m_filterTimeout = DEFAULT_FILTER_TIMEOUT_MS;
    uint32_t m_maxProcessBlock = DEFAULT_MAX_PROCESS_BLOCK;
    bool m_smSsl = false;
    bool m_disableSsl = false;
};
} // namespace bcos::tool