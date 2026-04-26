#include "OthersConfig.h"
#include <boost/property_tree/ptree.hpp>

using namespace bcos;
using namespace bcos::tool;

void OthersConfig::loadOthersConfig(boost::property_tree::ptree const& config)
{
    auto sendTxTimeout = config.get<int>("others.send_tx_timeout", -1);
    auto vmCacheSize = config.get<size_t>("executor.vm_cache_size", DEFAULT_VM_CACHE_SIZE);

    BaselineSchedulerConfig baselineSchedulerConfig;
    baselineSchedulerConfig.grainSize = config.get<int>(
        "executor.baseline_scheduler_chunksize", DEFAULT_BASELINE_SCHEDULER_CHUNK_SIZE);
    baselineSchedulerConfig.maxThread = config.get<int>(
        "executor.baseline_scheduler_maxthread", DEFAULT_BASELINE_SCHEDULER_MAX_THREAD);
    baselineSchedulerConfig.parallel = config.get<bool>("executor.baseline_scheduler_parallel", false);

    TarsRPCConfig tarsRPCConfig;
    tarsRPCConfig.host = config.get<std::string>("rpc.tars_rpc_host", "127.0.0.1");
    tarsRPCConfig.port = static_cast<uint16_t>(config.get<int>("rpc.tars_rpc_port", 0));
    tarsRPCConfig.threadCount = static_cast<uint32_t>(
        config.get<int>("rpc.tars_rpc_thread_count", DEFAULT_TARS_RPC_THREAD_COUNT));

    auto checkTransactionSignature =
        config.get<bool>("experimental.check_transaction_signature", true);
    auto checkParallelConflict = config.get<bool>("experimental.check_parallel_conflict", true);
    auto singlePointConsensus = config.get<bool>("experimental.single_point_consensus", false);
    bytes forceSender;
    if (auto forceSenderHex = config.get<std::string>("experimental.force_sender", {});
        !forceSenderHex.empty())
    {
        forceSender = fromHexWithPrefix(forceSenderHex);
    }

    setSendTxTimeout(sendTxTimeout);
    setVmCacheSize(vmCacheSize);
    setCheckTransactionSignature(checkTransactionSignature);
    setCheckParallelConflict(checkParallelConflict);
    setSinglePointConsensus(singlePointConsensus);
    setForceSender(std::move(forceSender));
    setBaselineSchedulerConfig(baselineSchedulerConfig);
    setTarsRPCConfig(std::move(tarsRPCConfig));
}
