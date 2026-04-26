#include "RpcConfig.h"
#include <boost/property_tree/ptree.hpp>

using namespace bcos::tool;

bool RpcConfig::loadRpcConfig(boost::property_tree::ptree const& config)
{
    constexpr auto FILTER_TIMEOUT_SECONDS = DEFAULT_FILTER_TIMEOUT_MS / 1000;
    constexpr auto MILLISECONDS_PER_SECOND = DEFAULT_FILTER_TIMEOUT_MS / FILTER_TIMEOUT_SECONDS;
    auto listenIP = config.get<std::string>("rpc.listen_ip", "0.0.0.0");
    auto listenPort = config.get<int>("rpc.listen_port", DEFAULT_LISTEN_PORT);
    auto threadCount = config.get<int>("rpc.thread_count", DEFAULT_THREAD_POOL_SIZE);
    auto filterTimeout = config.get<int>("rpc.filter_timeout", FILTER_TIMEOUT_SECONDS);
    auto maxProcessBlock = config.get<int>("rpc.filter_max_process_block", DEFAULT_MAX_PROCESS_BLOCK);
    auto smSsl = config.get<bool>("rpc.sm_ssl", false);
    auto disableSsl = config.get<bool>("rpc.disable_ssl", false);
    if (auto enableSsl = config.get_optional<bool>("rpc.enable_ssl"))
    {
        disableSsl = !enableSsl.value();
    }
    auto needRetInput = config.get<bool>("rpc.return_input_params", true);

    setListenIP(listenIP);
    setListenPort(static_cast<uint16_t>(listenPort));
    setThreadPoolSize(static_cast<uint32_t>(threadCount));
    setDisableSsl(disableSsl);
    setSmSsl(smSsl);
    setFilterTimeout(static_cast<uint32_t>(filterTimeout * MILLISECONDS_PER_SECOND));
    setMaxProcessBlock(static_cast<uint32_t>(maxProcessBlock));
    return needRetInput;
}
