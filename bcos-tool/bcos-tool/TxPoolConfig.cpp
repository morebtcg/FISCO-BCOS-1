#include "TxPoolConfig.h"
#include "Exceptions.h"
#include "bcos-utilities/BoostLog.h"
#include <algorithm>
#include <boost/property_tree/ptree.hpp>
#include <boost/throw_exception.hpp>
#include <thread>

using namespace bcos::tool;

namespace
{
constexpr size_t MAX_DEFAULT_VERIFIER_WORKERS = 8;
constexpr int64_t MILLISECONDS_PER_SECOND = 1000;

size_t defaultVerifierWorkerNum()
{
    return std::min(
        MAX_DEFAULT_VERIFIER_WORKERS, static_cast<size_t>(std::thread::hardware_concurrency() + 1U));
}
}  // namespace

void TxPoolConfig::loadTxPoolConfig(
    boost::property_tree::ptree const& config, size_t minSealTime, int64_t minConsensusTimeMs)
{
    auto txpoolLimit = config.get<int64_t>("txpool.limit", static_cast<int64_t>(DEFAULT_LIMIT));
    if (txpoolLimit <= 0)
    {
        BOOST_THROW_EXCEPTION(
            InvalidConfig() << errinfo_comment("Please set txpool.limit to positive !"));
    }

    auto notifyWorkerNum = config.get<int64_t>(
        "txpool.notify_worker_num", static_cast<int64_t>(DEFAULT_NOTIFY_WORKER_NUM));
    if (notifyWorkerNum <= 0)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "Please set txpool.notify_worker_num to positive !"));
    }

    auto verifierWorkerNum = config.get<int64_t>("txpool.verify_worker_num",
        static_cast<int64_t>(defaultVerifierWorkerNum()));
    if (verifierWorkerNum <= 0)
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "Please set txpool.verify_worker_num to positive !"));
    }

    auto txsExpirationTime =
        config.get<int64_t>("txpool.txs_expiration_time", DEFAULT_TXS_EXPIRATION_TIME_SECONDS);
    if (txsExpirationTime * MILLISECONDS_PER_SECOND <= minConsensusTimeMs) [[unlikely]]
    {
        BCOS_LOG(WARNING) << LOG_BADGE("NodeConfig")
                          << LOG_DESC(
                                 "loadTxPoolConfig: the configured txs_expiration_time is smaller "
                                 "than default consensus time, reset to the consensus time")
                          << LOG_KV("txsExpirationTime(seconds)", txsExpirationTime)
                          << LOG_KV("defaultConsTime", minConsensusTimeMs);
    }

    auto txsExpirationTimeMs = std::max(
        {txsExpirationTime * MILLISECONDS_PER_SECOND, minConsensusTimeMs, static_cast<int64_t>(minSealTime)});

    setLimit(static_cast<size_t>(txpoolLimit));
    setNotifyWorkerNum(static_cast<size_t>(notifyWorkerNum));
    setVerifierWorkerNum(static_cast<size_t>(verifierWorkerNum));
    setTxsExpirationTime(txsExpirationTimeMs);
    setCheckBlockLimit(config.get<bool>("txpool.check_block_limit", true));
    setEnableTxsFromFreeNode(config.get<bool>("txpool.enable_txs_from_free_node", false));
}
