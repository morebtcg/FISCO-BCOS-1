#pragma once

#include <boost/property_tree/ptree_fwd.hpp>
#include <cstddef>
#include <cstdint>

namespace bcos::tool
{
class TxPoolConfig
{
public:
    constexpr static size_t DEFAULT_LIMIT = 15000;
    constexpr static size_t DEFAULT_NOTIFY_WORKER_NUM = 2;
    constexpr static int64_t DEFAULT_TXS_EXPIRATION_TIME_SECONDS = 600;

    size_t limit() const { return m_limit; }
    void setLimit(size_t limit) { m_limit = limit; }

    size_t notifyWorkerNum() const { return m_notifyWorkerNum; }
    void setNotifyWorkerNum(size_t notifyWorkerNum) { m_notifyWorkerNum = notifyWorkerNum; }

    size_t verifierWorkerNum() const { return m_verifierWorkerNum; }
    void setVerifierWorkerNum(size_t verifierWorkerNum) { m_verifierWorkerNum = verifierWorkerNum; }

    int64_t txsExpirationTime() const { return m_txsExpirationTime; }
    void setTxsExpirationTime(int64_t txsExpirationTime) { m_txsExpirationTime = txsExpirationTime; }

    bool checkBlockLimit() const { return m_checkBlockLimit; }
    void setCheckBlockLimit(bool checkBlockLimit) { m_checkBlockLimit = checkBlockLimit; }

    bool enableTxsFromFreeNode() const { return m_enableTxsFromFreeNode; }
    void setEnableTxsFromFreeNode(bool enableTxsFromFreeNode)
    {
        m_enableTxsFromFreeNode = enableTxsFromFreeNode;
    }

    void loadTxPoolConfig(boost::property_tree::ptree const& config, size_t minSealTime,
        int64_t minConsensusTimeMs);

private:
    size_t m_limit = 0;
    size_t m_notifyWorkerNum = 0;
    size_t m_verifierWorkerNum = 0;
    int64_t m_txsExpirationTime = 0;
    bool m_checkBlockLimit = true;
    bool m_enableTxsFromFreeNode = false;
};
}  // namespace bcos::tool