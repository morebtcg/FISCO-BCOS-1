#pragma once

#include <boost/property_tree/ptree_fwd.hpp>
#include <cstdint>
#include <string>
#include <utility>

namespace bcos::tool
{
class Web3RpcConfig
{
public:
    constexpr static uint16_t DEFAULT_LISTEN_PORT = 8545;
    constexpr static uint32_t DEFAULT_THREAD_SIZE = 8;
    constexpr static uint32_t DEFAULT_FILTER_TIMEOUT_MS = 300000;
    constexpr static uint32_t DEFAULT_MAX_PROCESS_BLOCK = 10;
    constexpr static uint32_t DEFAULT_BATCH_REQUEST_SIZE_LIMIT = 8;
    constexpr static uint32_t DEFAULT_HTTP_BODY_SIZE_LIMIT = 10240000;
    constexpr static int32_t DEFAULT_CORS_MAX_AGE = 86400;

    bool enable() const { return m_enable; }
    void setEnable(bool enable) { m_enable = enable; }

    const std::string& listenIP() const { return m_listenIP; }
    void setListenIP(std::string listenIP) { m_listenIP = std::move(listenIP); }

    uint16_t listenPort() const { return m_listenPort; }
    void setListenPort(uint16_t listenPort) { m_listenPort = listenPort; }

    uint32_t threadSize() const { return m_threadSize; }
    void setThreadSize(uint32_t threadSize) { m_threadSize = threadSize; }

    uint32_t filterTimeout() const { return m_filterTimeout; }
    void setFilterTimeout(uint32_t filterTimeout) { m_filterTimeout = filterTimeout; }

    uint32_t maxProcessBlock() const { return m_maxProcessBlock; }
    void setMaxProcessBlock(uint32_t maxProcessBlock) { m_maxProcessBlock = maxProcessBlock; }

    uint32_t batchRequestSizeLimit() const { return m_batchRequestSizeLimit; }
    void setBatchRequestSizeLimit(uint32_t batchRequestSizeLimit)
    {
        m_batchRequestSizeLimit = batchRequestSizeLimit;
    }

    uint32_t httpBodySizeLimit() const { return m_httpBodySizeLimit; }
    void setHttpBodySizeLimit(uint32_t httpBodySizeLimit)
    {
        m_httpBodySizeLimit = httpBodySizeLimit;
    }

    bool enableCors() const { return m_enableCors; }
    void setEnableCors(bool enableCors) { m_enableCors = enableCors; }

    const std::string& corsAllowedOrigins() const { return m_corsAllowedOrigins; }
    void setCorsAllowedOrigins(std::string corsAllowedOrigins)
    {
        m_corsAllowedOrigins = std::move(corsAllowedOrigins);
    }

    const std::string& corsAllowedMethods() const { return m_corsAllowedMethods; }
    void setCorsAllowedMethods(std::string corsAllowedMethods)
    {
        m_corsAllowedMethods = std::move(corsAllowedMethods);
    }

    const std::string& corsAllowedHeaders() const { return m_corsAllowedHeaders; }
    void setCorsAllowedHeaders(std::string corsAllowedHeaders)
    {
        m_corsAllowedHeaders = std::move(corsAllowedHeaders);
    }

    int32_t corsMaxAge() const { return m_corsMaxAge; }
    void setCorsMaxAge(int32_t corsMaxAge) { m_corsMaxAge = corsMaxAge; }

    bool corsAllowCredentials() const { return m_corsAllowCredentials; }
    void setCorsAllowCredentials(bool corsAllowCredentials)
    {
        m_corsAllowCredentials = corsAllowCredentials;
    }

    bool syncTransaction() const { return m_syncTransaction; }
    void setSyncTransaction(bool syncTransaction) { m_syncTransaction = syncTransaction; }

    void loadWeb3RpcConfig(boost::property_tree::ptree const& config);

private:
    bool m_enable = false;
    std::string m_listenIP = "127.0.0.1";
    uint16_t m_listenPort = DEFAULT_LISTEN_PORT;
    uint32_t m_threadSize = DEFAULT_THREAD_SIZE;
    uint32_t m_filterTimeout = DEFAULT_FILTER_TIMEOUT_MS;
    uint32_t m_maxProcessBlock = DEFAULT_MAX_PROCESS_BLOCK;
    uint32_t m_batchRequestSizeLimit = DEFAULT_BATCH_REQUEST_SIZE_LIMIT;
    uint32_t m_httpBodySizeLimit = DEFAULT_HTTP_BODY_SIZE_LIMIT;
    bool m_enableCors = true;
    std::string m_corsAllowedOrigins = "*";
    std::string m_corsAllowedMethods = "GET, POST, OPTIONS";
    std::string m_corsAllowedHeaders = "Content-Type, Authorization, X-Requested-With";
    int32_t m_corsMaxAge = DEFAULT_CORS_MAX_AGE;
    bool m_corsAllowCredentials = true;
    bool m_syncTransaction = false;
};
}  // namespace bcos::tool