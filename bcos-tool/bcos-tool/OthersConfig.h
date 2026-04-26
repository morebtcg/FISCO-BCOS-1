#pragma once

#include "BaselineSchedulerConfig.h"
#include "TarsRPCConfig.h"
#include <boost/property_tree/ptree_fwd.hpp>
#include <bcos-utilities/Common.h>
#include <bcos-utilities/DataConvertUtility.h>
#include <cstddef>
#include <utility>

namespace bcos::tool
{
class OthersConfig
{
public:
    constexpr static size_t DEFAULT_VM_CACHE_SIZE = 1024;
    constexpr static int DEFAULT_BASELINE_SCHEDULER_CHUNK_SIZE = 100;
    constexpr static int DEFAULT_BASELINE_SCHEDULER_MAX_THREAD = 16;
    constexpr static uint32_t DEFAULT_TARS_RPC_THREAD_COUNT = 8;

    int sendTxTimeout() const { return m_sendTxTimeout; }
    void setSendTxTimeout(int sendTxTimeout) { m_sendTxTimeout = sendTxTimeout; }

    size_t vmCacheSize() const { return m_vmCacheSize; }
    void setVmCacheSize(size_t vmCacheSize) { m_vmCacheSize = vmCacheSize; }

    bool checkTransactionSignature() const { return m_checkTransactionSignature; }
    void setCheckTransactionSignature(bool checkTransactionSignature)
    {
        m_checkTransactionSignature = checkTransactionSignature;
    }

    bool checkParallelConflict() const { return m_checkParallelConflict; }
    void setCheckParallelConflict(bool checkParallelConflict)
    {
        m_checkParallelConflict = checkParallelConflict;
    }

    bool singlePointConsensus() const { return m_singlePointConsensus; }
    void setSinglePointConsensus(bool singlePointConsensus)
    {
        m_singlePointConsensus = singlePointConsensus;
    }

    bytes const& forceSender() const { return m_forceSender; }
    void setForceSender(bytes forceSender) { m_forceSender = std::move(forceSender); }

    BaselineSchedulerConfig const& baselineSchedulerConfig() const
    {
        return m_baselineSchedulerConfig;
    }
    void setBaselineSchedulerConfig(BaselineSchedulerConfig baselineSchedulerConfig)
    {
        m_baselineSchedulerConfig = baselineSchedulerConfig;
    }

    TarsRPCConfig const& tarsRPCConfig() const { return m_tarsRPCConfig; }
    void setTarsRPCConfig(TarsRPCConfig tarsRPCConfig)
    {
        m_tarsRPCConfig = std::move(tarsRPCConfig);
    }

    void loadOthersConfig(boost::property_tree::ptree const& config);

private:
    int m_sendTxTimeout = -1;
    size_t m_vmCacheSize = DEFAULT_VM_CACHE_SIZE;
    bool m_checkTransactionSignature = true;
    bool m_checkParallelConflict = true;
    bool m_singlePointConsensus = false;
    bytes m_forceSender;
    BaselineSchedulerConfig m_baselineSchedulerConfig;
    TarsRPCConfig m_tarsRPCConfig;
};
}  // namespace bcos::tool