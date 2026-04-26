#include "StorageConfig.h"
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>

using namespace bcos;
using namespace bcos::tool;

void StorageConfig::loadStorageConfig(
    boost::property_tree::ptree const& config, std::string const& groupID)
{
    auto storagePath = config.get<std::string>("storage.data_path", "data/" + groupID);
    auto storageType = config.get<std::string>("storage.type", "RocksDB");
    auto keyPageSize =
        config.get<int32_t>("storage.key_page_size", static_cast<int32_t>(DEFAULT_KEY_PAGE_SIZE));
    auto maxWriteBufferNumber =
        config.get<int32_t>("storage.max_write_buffer_number", DEFAULT_MAX_WRITE_BUFFER_NUMBER);
    auto maxBackgroundJobs =
        config.get<int32_t>("storage.max_background_jobs", DEFAULT_MAX_BACKGROUND_JOBS);
    auto writeBufferSize = config.get<size_t>("storage.write_buffer_size", DEFAULT_WRITE_BUFFER_SIZE);
    auto minWriteBufferNumberToMerge = config.get<int32_t>(
        "storage.min_write_buffer_number_to_merge", DEFAULT_MIN_WRITE_BUFFER_NUMBER_TO_MERGE);
    auto blockCacheSize = config.get<size_t>("storage.block_cache_size", DEFAULT_BLOCK_CACHE_SIZE);
    auto enableStatistics = config.get<bool>("storage.enable_statistics", false);
    auto enableRocksDBBlob = config.get<bool>("storage.enable_rocksdb_blob", false);
    auto pdCaPath = config.get<std::string>("storage.pd_ssl_ca_path", "");
    auto pdCertPath = config.get<std::string>("storage.pd_ssl_cert_path", "");
    auto pdKeyPath = config.get<std::string>("storage.pd_ssl_key_path", "");
    auto enableArchive = config.get<bool>("storage.enable_archive", false);
    auto syncArchivedBlocks = config.get<bool>("storage.sync_archived_blocks", false);
    auto enableSeparateBlockAndState =
        config.get<bool>("storage.enable_separate_block_state", false);
    if (boost::iequals(storageType, "TiKV"))
    {
        enableSeparateBlockAndState = false;
    }

    auto stateDBPath = storagePath + "/state";
    auto blockDBPath = storagePath + "/block";

    std::string archiveListenIP;
    uint16_t archiveListenPort = 0;
    if (enableArchive)
    {
        archiveListenIP = config.get<std::string>("storage.archive_ip");
        archiveListenPort = config.get<uint16_t>("storage.archive_port");
    }

    auto pdAddrsValue = config.get<std::string>("storage.pd_addrs", "127.0.0.1:2379");
    std::vector<std::string> pdAddrs;
    boost::split(pdAddrs, pdAddrsValue, boost::is_any_of(","));

    auto enableLRUCacheStorage = config.get<bool>("storage.enable_cache", true);
    auto cacheSize = config.get<ssize_t>("storage.cache_size", DEFAULT_CACHE_SIZE);

    setStoragePath(storagePath);
    setStorageType(storageType);
    setKeyPageSize(keyPageSize);
    setMaxWriteBufferNumber(maxWriteBufferNumber);
    setMaxBackgroundJobs(maxBackgroundJobs);
    setWriteBufferSize(writeBufferSize);
    setMinWriteBufferNumberToMerge(minWriteBufferNumberToMerge);
    setBlockCacheSize(blockCacheSize);
    setEnableStatistics(enableStatistics);
    setEnableRocksDBBlob(enableRocksDBBlob);
    setPdAddrs(std::move(pdAddrs));
    setPdCaPath(pdCaPath);
    setPdCertPath(pdCertPath);
    setPdKeyPath(pdKeyPath);
    setEnableArchive(enableArchive);
    setSyncArchivedBlocks(syncArchivedBlocks);
    setEnableSeparateBlockAndState(enableSeparateBlockAndState);
    setStateDBPath(stateDBPath);
    setBlockDBPath(blockDBPath);
    setArchiveListenIP(archiveListenIP);
    setArchiveListenPort(archiveListenPort);
    setEnableLRUCacheStorage(enableLRUCacheStorage);
    setCacheSize(cacheSize);
}