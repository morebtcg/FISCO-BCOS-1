#pragma once
#include <boost/property_tree/ptree_fwd.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace bcos::tool
{
class StorageConfig
{
public:
    constexpr static size_t DEFAULT_KEY_PAGE_SIZE = 10240;
    constexpr static int DEFAULT_MAX_WRITE_BUFFER_NUMBER = 4;
    constexpr static int DEFAULT_MAX_BACKGROUND_JOBS = 4;
    constexpr static size_t DEFAULT_WRITE_BUFFER_SIZE = 64 << 20;
    constexpr static int DEFAULT_MIN_WRITE_BUFFER_NUMBER_TO_MERGE = 1;
    constexpr static size_t DEFAULT_BLOCK_CACHE_SIZE = 128 << 20;
    constexpr static ssize_t DEFAULT_CACHE_SIZE = static_cast<ssize_t>(32) * 1024 * 1024;

    StorageConfig() = default;

    const std::string& storageType() const { return m_storageType; }
    const std::string& storagePath() const { return m_storagePath; }
    size_t keyPageSize() const { return m_keyPageSize; }
    int maxWriteBufferNumber() const { return m_maxWriteBufferNumber; }
    int maxBackgroundJobs() const { return m_maxBackgroundJobs; }
    size_t writeBufferSize() const { return m_writeBufferSize; }
    int minWriteBufferNumberToMerge() const { return m_minWriteBufferNumberToMerge; }
    size_t blockCacheSize() const { return m_blockCacheSize; }
    bool enableStatistics() const { return m_enableStatistics; }
    bool enableRocksDBBlob() const { return m_enableRocksDBBlob; }
    std::vector<std::string> const& pdAddrs() const { return m_pdAddrs; }
    const std::string& pdCaPath() const { return m_pdCaPath; }
    const std::string& pdCertPath() const { return m_pdCertPath; }
    const std::string& pdKeyPath() const { return m_pdKeyPath; }
    bool enableArchive() const { return m_enableArchive; }
    bool syncArchivedBlocks() const { return m_syncArchivedBlocks; }
    bool enableSeparateBlockAndState() const { return m_enableSeparateBlockAndState; }
    const std::string& stateDBPath() const { return m_stateDBPath; }
    const std::string& blockDBPath() const { return m_blockDBPath; }
    const std::string& archiveListenIP() const { return m_archiveListenIP; }
    uint16_t archiveListenPort() const { return m_archiveListenPort; }
    bool enableLRUCacheStorage() const { return m_enableLRUCacheStorage; }
    ssize_t cacheSize() const { return m_cacheSize; }
    const std::string& storageDBName() const { return m_storageDBName; }
    const std::string& stateDBName() const { return m_stateDBName; }

    void setStorageType(const std::string& storageType) { m_storageType = storageType; }
    void setStoragePath(const std::string& storagePath) { m_storagePath = storagePath; }
    void setKeyPageSize(size_t keyPageSize) { m_keyPageSize = keyPageSize; }
    void setMaxWriteBufferNumber(int maxWriteBufferNumber)
    {
        m_maxWriteBufferNumber = maxWriteBufferNumber;
    }
    void setMaxBackgroundJobs(int maxBackgroundJobs) { m_maxBackgroundJobs = maxBackgroundJobs; }
    void setWriteBufferSize(size_t writeBufferSize) { m_writeBufferSize = writeBufferSize; }
    void setMinWriteBufferNumberToMerge(int minWriteBufferNumberToMerge)
    {
        m_minWriteBufferNumberToMerge = minWriteBufferNumberToMerge;
    }
    void setBlockCacheSize(size_t blockCacheSize) { m_blockCacheSize = blockCacheSize; }
    void setEnableStatistics(bool enableStatistics) { m_enableStatistics = enableStatistics; }
    void setEnableRocksDBBlob(bool enableRocksDBBlob)
    {
        m_enableRocksDBBlob = enableRocksDBBlob;
    }
    void setPdAddrs(std::vector<std::string> pdAddrs) { m_pdAddrs = std::move(pdAddrs); }
    void setPdCaPath(const std::string& pdCaPath) { m_pdCaPath = pdCaPath; }
    void setPdCertPath(const std::string& pdCertPath) { m_pdCertPath = pdCertPath; }
    void setPdKeyPath(const std::string& pdKeyPath) { m_pdKeyPath = pdKeyPath; }
    void setEnableArchive(bool enableArchive) { m_enableArchive = enableArchive; }
    void setSyncArchivedBlocks(bool syncArchivedBlocks) { m_syncArchivedBlocks = syncArchivedBlocks; }
    void setEnableSeparateBlockAndState(bool enableSeparateBlockAndState)
    {
        m_enableSeparateBlockAndState = enableSeparateBlockAndState;
    }
    void setStateDBPath(const std::string& stateDBPath) { m_stateDBPath = stateDBPath; }
    void setBlockDBPath(const std::string& blockDBPath) { m_blockDBPath = blockDBPath; }
    void setArchiveListenIP(const std::string& archiveListenIP)
    {
        m_archiveListenIP = archiveListenIP;
    }
    void setArchiveListenPort(uint16_t archiveListenPort) { m_archiveListenPort = archiveListenPort; }
    void setEnableLRUCacheStorage(bool enableLRUCacheStorage)
    {
        m_enableLRUCacheStorage = enableLRUCacheStorage;
    }
    void setCacheSize(ssize_t cacheSize) { m_cacheSize = cacheSize; }
    void setStorageDBName(const std::string& storageDBName) { m_storageDBName = storageDBName; }
    void setStateDBName(const std::string& stateDBName) { m_stateDBName = stateDBName; }

    void loadStorageConfig(
        boost::property_tree::ptree const& config, std::string const& groupID);

private:

    std::string m_storageType = "RocksDB";
    std::string m_storagePath = "./storage";
    size_t m_keyPageSize = DEFAULT_KEY_PAGE_SIZE;
    int m_maxWriteBufferNumber = DEFAULT_MAX_WRITE_BUFFER_NUMBER;
    int m_maxBackgroundJobs = DEFAULT_MAX_BACKGROUND_JOBS;
    size_t m_writeBufferSize = DEFAULT_WRITE_BUFFER_SIZE;
    int m_minWriteBufferNumberToMerge = DEFAULT_MIN_WRITE_BUFFER_NUMBER_TO_MERGE;
    size_t m_blockCacheSize = DEFAULT_BLOCK_CACHE_SIZE;
    bool m_enableStatistics = false;
    bool m_enableRocksDBBlob = false;
    std::vector<std::string> m_pdAddrs;
    std::string m_pdCaPath;
    std::string m_pdCertPath;
    std::string m_pdKeyPath;
    bool m_enableArchive = false;
    bool m_syncArchivedBlocks = false;
    bool m_enableSeparateBlockAndState = false;
    std::string m_stateDBPath;
    std::string m_blockDBPath;
    std::string m_archiveListenIP;
    uint16_t m_archiveListenPort = 0;
    bool m_enableLRUCacheStorage = true;
    ssize_t m_cacheSize = DEFAULT_CACHE_SIZE;
    std::string m_storageDBName = "storage";
    std::string m_stateDBName = "state";
};
} // namespace bcos::tool