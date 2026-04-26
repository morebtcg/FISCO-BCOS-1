#include "Web3RpcConfig.h"
#include <boost/property_tree/ptree.hpp>

using namespace bcos::tool;

void Web3RpcConfig::loadWeb3RpcConfig(boost::property_tree::ptree const& config)
{
    constexpr auto FILTER_TIMEOUT_SECONDS = DEFAULT_FILTER_TIMEOUT_MS / 1000;
    constexpr auto MILLISECONDS_PER_SECOND = DEFAULT_FILTER_TIMEOUT_MS / FILTER_TIMEOUT_SECONDS;
    auto listenIP = config.get<std::string>("web3_rpc.listen_ip", "127.0.0.1");
    auto listenPort = config.get<int>("web3_rpc.listen_port", DEFAULT_LISTEN_PORT);
    auto threadCount = config.get<int>("web3_rpc.thread_count", DEFAULT_THREAD_SIZE);
    auto filterTimeout = config.get<int>("web3_rpc.filter_timeout", FILTER_TIMEOUT_SECONDS);
    auto maxProcessBlock = config.get<int>("web3_rpc.filter_max_process_block", DEFAULT_MAX_PROCESS_BLOCK);
    auto enableWeb3Rpc = config.get<bool>("web3_rpc.enable", false);
    auto batchRequestSizeLimit =
        config.get<int>("web3_rpc.batch_request_size_limit", DEFAULT_BATCH_REQUEST_SIZE_LIMIT);
    auto requestBodySizeLimit =
        config.get<int>("web3_rpc.request_body_size_limit", DEFAULT_HTTP_BODY_SIZE_LIMIT);
    auto enableCors = config.get<bool>("web3_rpc.enable_cors", true);
    auto corsAllowCredentials = config.get<bool>("web3_rpc.cors_allow_credentials", true);
    auto corsAllowedOrigins = config.get<std::string>("web3_rpc.cors_allowed_origins", "*");
    auto corsAllowedMethods =
        config.get<std::string>("web3_rpc.cors_allowed_methods", "GET, POST, OPTIONS");
    auto corsAllowedHeaders = config.get<std::string>(
        "web3_rpc.cors_allowed_headers", "Content-Type, Authorization, X-Requested-With");
    auto corsMaxAge = config.get<int32_t>("web3_rpc.cors_max_age", DEFAULT_CORS_MAX_AGE);
    auto syncTransaction = config.get<bool>("web3_rpc.sync_transaction", false);

    setEnable(enableWeb3Rpc);
    setListenIP(listenIP);
    setListenPort(static_cast<uint16_t>(listenPort));
    setThreadSize(static_cast<uint32_t>(threadCount));
    setFilterTimeout(static_cast<uint32_t>(filterTimeout * MILLISECONDS_PER_SECOND));
    setMaxProcessBlock(static_cast<uint32_t>(maxProcessBlock));
    setBatchRequestSizeLimit(static_cast<uint32_t>(batchRequestSizeLimit));
    setHttpBodySizeLimit(static_cast<uint32_t>(requestBodySizeLimit));
    setEnableCors(enableCors);
    setCorsAllowedOrigins(corsAllowedOrigins);
    setCorsAllowedMethods(corsAllowedMethods);
    setCorsAllowedHeaders(corsAllowedHeaders);
    setCorsMaxAge(corsMaxAge);
    setCorsAllowCredentials(corsAllowCredentials);
    setSyncTransaction(syncTransaction);
}
