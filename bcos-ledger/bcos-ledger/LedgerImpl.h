#pragma once

#include "Ledger.h"
#include "bcos-framework/ledger/Ledger.h"
#include "bcos-task/Task.h"
#include <bcos-concepts/Basic.h>
#include <bcos-concepts/ByteBuffer.h>
#include <bcos-concepts/Hash.h>
#include <bcos-concepts/protocol/Block.h>
#include <bcos-crypto/hasher/Hasher.h>
#include <bcos-crypto/merkle/Merkle.h>
#include <bcos-executor/src/Common.h>
#include <bcos-framework/ledger/LedgerTypeDef.h>
#include <bcos-table/src/StateStorageFactory.h>
#include <bcos-tool/VersionConverter.h>
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/Ranges.h>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <boost/lexical_cast.hpp>
#include <boost/throw_exception.hpp>
#include <type_traits>

namespace bcos::ledger
{
static constexpr const int LIGHTNODE_MAX_REQUEST_BLOCKS_COUNT = 50;

DERIVE_BCOS_EXCEPTION(NotFoundTransaction);
DERIVE_BCOS_EXCEPTION(UnexpectedRowIndex);
DERIVE_BCOS_EXCEPTION(MismatchTransactionCount);
DERIVE_BCOS_EXCEPTION(MismatchParentHash);
DERIVE_BCOS_EXCEPTION(NotFoundBlockHeader);
DERIVE_BCOS_EXCEPTION(GetABIError);
DERIVE_BCOS_EXCEPTION(GetBlockDataError);

struct LedgerStatus
{
    int64_t total = 0;
    int64_t failed = 0;
    int64_t blockNumber = 0;
};

struct BlockDataFlagBase
{
};
struct BlockDataAll : BlockDataFlagBase
{
};
struct BlockDataHeader : BlockDataFlagBase
{
};
struct BlockDataTransactionsMeta : BlockDataFlagBase
{
};
struct BlockDataTransactions : BlockDataFlagBase
{
};
struct BlockDataReceipts : BlockDataFlagBase
{
};
struct BlockDataNonces : BlockDataFlagBase
{
};

template <bcos::crypto::hasher::Hasher Hasher, class Storage>
class LedgerImpl : public Ledger
{
public:
    LedgerImpl(Hasher hasher, Storage storage, bcos::protocol::BlockFactory::Ptr blockFactory,
        bcos::storage::StorageInterface::Ptr storageInterface, size_t blockLimit,
        bcos::storage::StorageInterface::Ptr blockStorage = nullptr)
      : Ledger(std::move(blockFactory), storageInterface, blockLimit, blockStorage),
        m_hasher(std::move(hasher)),
        m_backupStorage(storageInterface),
        m_storage{std::move(storage)},
        m_merkle{m_hasher.clone()}
    {}
    using statusInfoType = std::map<crypto::NodeIDPtr, bcos::protocol::BlockNumber>;

    void setKeyPageSize(size_t keyPageSize) { m_keyPageSize = keyPageSize; }

    template <bcos::concepts::block::Block BlockType>
    void checkParentBlock(BlockType parentBlock, BlockType block)
    {
        std::array<std::byte, Hasher::HASH_SIZE> parentHash;
        bcos::concepts::hash::calculate(parentBlock, m_hasher.clone(), parentHash);

        if (RANGES::empty(block.blockHeader.data.parentInfo) ||
            (block.blockHeader.data.parentInfo[0].blockNumber !=
                parentBlock.blockHeader.data.blockNumber) ||
            !bcos::concepts::bytebuffer::equalTo(
                block.blockHeader.data.parentInfo[0].blockHash, parentHash))
        {
            LEDGER_LOG(ERROR) << "ParentHash mismatch!";
            BOOST_THROW_EXCEPTION(MismatchParentHash{} << errinfo_comment{"No match parentHash!"});
        }
    }

    crypto::NodeIDs filterSyncNodeList(
        statusInfoType const& peersStatusInfo, bcos::protocol::BlockNumber needBlockNumber)
    {
        crypto::NodeIDs requestNodeIDList;
        for (const auto& nodeStatus : peersStatusInfo)
        {
            LEDGER_LOG(INFO) << LOG_KV("nodeID", nodeStatus.first->hex())
                             << LOG_KV("blockNumber: ", nodeStatus.second)
                             << LOG_KV("，needBlockNumber: ", needBlockNumber);
            if (nodeStatus.second >= needBlockNumber)
            {
                requestNodeIDList.push_back(nodeStatus.first);
            }
        }
        LEDGER_LOG(DEBUG) << LOG_KV("requestNodeIDList size", requestNodeIDList.size());
        return requestNodeIDList;
    }

public:
    template <class... Flags>
    task::Task<void> impl_getBlock(bcos::concepts::block::BlockNumber auto blockNumber,
        bcos::concepts::block::Block auto& block)
    {
        LEDGER_LOG(INFO) << "getBlock: " << blockNumber;

        auto blockNumberStr = boost::lexical_cast<std::string>(blockNumber);
        (co_await getBlockData<Flags>(blockNumberStr, block), ...);
    }

    template <class... Flags>
    task::Task<void> getBlock(bcos::concepts::block::BlockNumber auto blockNumber,
        bcos::concepts::block::Block auto& block)
    {
        co_await impl_getBlock<Flags...>(blockNumber, block);
    }

    template <class... Flags>
    task::Task<void> impl_getBlockByNodeList(bcos::concepts::block::BlockNumber auto blockNumber,
        bcos::concepts::block::Block auto& block, bcos::crypto::NodeIDs const& nodeList)
    {
        try
        {
            LEDGER_LOG(INFO) << "getBlockByNodeList: " << blockNumber;
            auto blockNumberStr = boost::lexical_cast<std::string>(blockNumber);
            (co_await getBlockData<Flags>(blockNumberStr, block), ...);
        }
        catch (NotFoundBlockHeader& e)
        {
            LEDGER_LOG(ERROR) << "Not found block";
            block = {};
        }
        co_return;
    }

    template <class... Flags>
    task::Task<void> getBlockByNodeList(bcos::concepts::block::BlockNumber auto blockNumber,
        bcos::concepts::block::Block auto& block, bcos::crypto::NodeIDs const& nodeList)
    {
        co_await impl_getBlockByNodeList<Flags...>(blockNumber, block, nodeList);
    }

    template <class... Flags>
    task::Task<void> impl_setBlock(bcos::concepts::block::Block auto block)
    {
        LEDGER_LOG(INFO) << "setBlock: " << block.blockHeader.data.blockNumber;

        auto blockNumberStr = boost::lexical_cast<std::string>(block.blockHeader.data.blockNumber);
        (co_await setBlockData<Flags>(blockNumberStr, block), ...);
        co_return;
    }

    template <class... Flags>
    task::Task<void> setBlock(bcos::concepts::block::Block auto block)
    {
        co_await impl_setBlock<Flags...>(std::move(block));
    }

    task::Task<void> getBlockNumberByHash(
        bcos::concepts::bytebuffer::ByteBuffer auto const& hash, std::integral auto& number)
    {
        co_await impl_getBlockNumberByHash(hash, number);
    }

    task::Task<void> getBlockHashByNumber(
        std::integral auto number, bcos::concepts::bytebuffer::ByteBuffer auto& hash)
    {
        co_await impl_getBlockHashByNumber(number, hash);
    }

    task::Task<std::string> getABI(std::string contractAddress)
    {
        co_return co_await impl_getABI(std::move(contractAddress));
    }

    task::Task<void> getTransactions(RANGES::range auto const& hashes, RANGES::range auto& out)
    {
        co_await impl_getTransactions(hashes, out);
    }

    task::Task<void> impl_getBlockNumberByHash(
        bcos::concepts::bytebuffer::ByteBuffer auto const& hash, std::integral auto& number)
    {
        LEDGER_LOG(INFO) << "getBlockNumberByHash request";

        auto entry = storage().getRow(SYS_HASH_2_NUMBER, bcos::concepts::bytebuffer::toView(hash));

        if (!entry)
        {
            number = -1;
            co_return;
        }

        try
        {
            number = boost::lexical_cast<bcos::protocol::BlockNumber>(entry->getField(0));
        }
        catch (boost::bad_lexical_cast& e)
        {
            // Ignore the exception
            LEDGER_LOG(INFO) << "Cast blockNumber failed, may be empty, set to default value -1"
                             << LOG_KV("blockNumber str", entry->getField(0));
            number = -1;
        }
    }

    task::Task<void> impl_getBlockHashByNumber(
        std::integral auto number, bcos::concepts::bytebuffer::ByteBuffer auto& hash)
    {
        LEDGER_LOG(INFO) << "getBlockHashByNumber request" << LOG_KV("blockNumber", number);

        auto key = boost::lexical_cast<std::string>(number);
        auto entry = storage().getRow(SYS_NUMBER_2_HASH, key);
        if (!entry)
        {
            LEDGER_LOG(DEBUG) << "Not found block number: " << number;
            co_return;
        }

        auto hashStr = entry->getField(0);
        bcos::concepts::bytebuffer::assignTo(hashStr, hash);
    }

    task::Task<std::string> impl_getABI(std::string _contractAddress)
    {
        // try to get compatibilityVersion
        std::string contractTableName = getContractTableName("/apps/", _contractAddress);
        auto versionEntry =
            storage().getRow(ledger::SYS_CONFIG, ledger::SYSTEM_KEY_COMPATIBILITY_VERSION);
        auto [compatibilityVersionStr, number] =
            versionEntry->template getObject<SystemConfigEntry>();
        if (!versionEntry)
        {
            LEDGER_LOG(WARNING) << "Not found compatibilityVersion: ";
            BOOST_THROW_EXCEPTION(
                GetABIError{} << errinfo_comment{"get compatibilityVersion not found"});
        }
        m_compatibilityVersion = bcos::tool::toVersionNumber(compatibilityVersionStr);
        LEDGER_LOG(TRACE) << "getABI contractAddress is: " << _contractAddress
                          << ", contractTableName is: " << contractTableName
                          << ", m_compatibilityVersion is " << m_compatibilityVersion;

        // create keyPageStorage
        auto stateStorageFactory = std::make_shared<storage::StateStorageFactory>(m_keyPageSize);
        // getABI function begin in version 320
        auto keyPageIgnoreTables = std::make_shared<std::set<std::string, std::less<>>>(
            storage::IGNORED_ARRAY_310.begin(), storage::IGNORED_ARRAY_310.end());

        // 此处为只读操作，不可能写，因此无需关注setRowWithDirtyFlag
        // This is a read-only operation and it is impossible to write, so there is no need to pay
        // attention to setRowWithDirtyFlag
        auto stateStorage = stateStorageFactory->createStateStorage(
            m_backupStorage, m_compatibilityVersion, false, true, keyPageIgnoreTables);

        // try to get codeHash
        auto codeHashEntry = stateStorage->getRow(contractTableName, "codeHash");
        if (!codeHashEntry.second) [[unlikely]]
        {
            LEDGER_LOG(WARNING) << "Not found codeHash contractAddress:" << _contractAddress;
            BOOST_THROW_EXCEPTION(GetABIError{} << errinfo_comment{"Get CodeHash not found"});
        }
        auto codeHash = codeHashEntry.second->getField(0);

        // according to codeHash get abi
        auto entry = stateStorage->getRow(SYS_CONTRACT_ABI, codeHash);
        if (!entry.second) [[unlikely]]
        {
            LEDGER_LOG(WARNING) << "Not found contractAddress abi:" << _contractAddress;
            BOOST_THROW_EXCEPTION(GetABIError{} << errinfo_comment{"Get Abi not found"});
        }

        std::string abiStr = std::string(entry.second->getField(0));
        LEDGER_LOG(TRACE) << "contractAddress is " << _contractAddress
                          << "ledger impl get abi is: " << abiStr;
        co_return abiStr;
    }

    task::Task<void> impl_getTransactions(RANGES::range auto const& hashes, RANGES::range auto& out)
    {
        bcos::concepts::resizeTo(out, RANGES::size(hashes));
        using DataType = RANGES::range_value_t<std::remove_cvref_t<decltype(out)>>;

        constexpr auto tableName =
            bcos::concepts::transaction::Transaction<DataType> ? SYS_HASH_2_TX : SYS_HASH_2_RECEIPT;

        LEDGER_LOG(INFO) << "getTransactions: " << tableName << " " << RANGES::size(hashes);
        auto entries = storage().getRows(std::string_view{tableName}, hashes);

        bcos::concepts::resizeTo(out, RANGES::size(hashes));
        tbb::parallel_for(tbb::blocked_range<size_t>(0U, RANGES::size(entries)),
            [&entries, &out](const tbb::blocked_range<size_t>& range) {
                for (auto index = range.begin(); index != range.end(); ++index)
                {
                    if (!entries[index])
                    {
                        [[unlikely]] BOOST_THROW_EXCEPTION(
                            NotFoundTransaction{} << errinfo_comment{"Get transaction not found"});
                    }

                    auto field = entries[index]->getField(0);
                    auto bytesRef =
                        bcos::bytesConstRef((const bcos::byte*)field.data(), field.size());
                    bcos::concepts::serialize::decode(bytesRef, out[index]);
                }
            });

        co_return;
    }

    task::Task<LedgerStatus> impl_getStatus()
    {
        LEDGER_LOG(TRACE) << "getStatus";
        constexpr static auto keys = std::to_array({SYS_KEY_TOTAL_TRANSACTION_COUNT,
            SYS_KEY_TOTAL_FAILED_TRANSACTION, SYS_KEY_CURRENT_NUMBER});

        LedgerStatus status;
        auto entries = storage().getRows(SYS_CURRENT_STATE, keys);
        for (auto i = 0U; i < RANGES::size(entries); ++i)
        {
            auto& entry = entries[i];

            int64_t value = 0;
            if (entry)
            {
                [[likely]] value = boost::lexical_cast<int64_t>(entry->getField(0));
            }

            switch (i)
            {
            case 0:
                status.total = value;
                break;
            case 1:
                status.failed = value;
                break;
            case 2:
                status.blockNumber = value;
                break;
            default:
                BOOST_THROW_EXCEPTION(
                    UnexpectedRowIndex{} << errinfo_comment{
                        "Unexpected getRows index: " + boost::lexical_cast<std::string>(i)});
                break;
            }
        }
        LEDGER_LOG(TRACE) << "getStatus result: " << status.total << " | " << status.failed << " | "
                          << status.blockNumber;

        co_return status;
    }

    task::Task<LedgerStatus> getStatus() { co_return co_await impl_getStatus(); }

    task::Task<std::map<crypto::NodeIDPtr, bcos::protocol::BlockNumber>> impl_getAllPeersStatus()
    {
        std::map<crypto::NodeIDPtr, bcos::protocol::BlockNumber> allPeersStatus;
        // assert(false); //never reach here
        co_return allPeersStatus;
    }

    task::Task<std::map<crypto::NodeIDPtr, bcos::protocol::BlockNumber>> getAllPeersStatus()
    {
        co_return co_await impl_getAllPeersStatus();
    }

    template <class LedgerType, bcos::concepts::block::Block BlockType>
    task::Task<size_t> impl_sync(LedgerType& source, bool onlyHeader)
    {
        auto& sourceLedger = bcos::concepts::getRef(source);
        auto status = co_await impl_getStatus();
        auto allPeersStatus = co_await sourceLedger.getAllPeersStatus();
        bcos::protocol::BlockNumber currentMaxBlockNumber = 0;
        for (const auto& nodeStatus : allPeersStatus)
        {
            if (nodeStatus.second > currentMaxBlockNumber)
            {
                currentMaxBlockNumber = nodeStatus.second;
            }
        }
        LEDGER_LOG(DEBUG) << LOG_KV("allPeersStatus", allPeersStatus.size())
                          << LOG_KV("currentMaxBlockNumber", currentMaxBlockNumber);
        std::optional<BlockType> parentBlock;
        size_t syncedBlock = 0;
        auto syncBlockNumber = status.blockNumber + LIGHTNODE_MAX_REQUEST_BLOCKS_COUNT;
        if (allPeersStatus.size() != 0 && currentMaxBlockNumber < syncBlockNumber)
        {
            syncBlockNumber = currentMaxBlockNumber;
        }
        // sync block
        for (auto blockNumber = status.blockNumber + 1; blockNumber <= syncBlockNumber;
            ++blockNumber)
        {
            LEDGER_LOG(INFO) << "Syncing block from remote: " << blockNumber << " | "
                             << syncBlockNumber << " | " << onlyHeader;
            BlockType block;
            auto syncNodeList = filterSyncNodeList(allPeersStatus, blockNumber);
            if (onlyHeader)
            {
                co_await sourceLedger.template getBlockByNodeList<BlockDataHeader>(
                    blockNumber, block, syncNodeList);
            }
            else
            {
                co_await sourceLedger.template getBlockByNodeList<BlockDataAll>(
                    blockNumber, block, syncNodeList);
            }
            // if getBlockByNodeList return empty block, break
            if (RANGES::empty(block.blockHeader.data.parentInfo))
            {
                LEDGER_LOG(WARNING)
                    << LOG_DESC("No blockHeader in block") << LOG_KV("blockNumber", blockNumber);
                break;
            }
            if (blockNumber > 0)
            {
                if (!parentBlock)
                {
                    parentBlock = BlockType();
                    co_await impl_getBlock<BlockDataHeader>(blockNumber - 1, *parentBlock);
                }
                checkParentBlock(*parentBlock, block);
            }
            if (onlyHeader)
            {
                co_await impl_setBlock<BlockDataHeader>(block);
            }
            else
            {
                co_await impl_setBlock<BlockDataAll>(block);
            }
            parentBlock = std::move(block);
            ++syncedBlock;
        }
        co_return syncedBlock;
    }

    template <class LedgerType, bcos::concepts::block::Block BlockType>
    task::Task<size_t> sync(LedgerType& source, bool onlyHeader)
    {
        co_return co_await impl_sync<LedgerType, BlockType>(source, onlyHeader);
    }

    template <std::same_as<BlockDataHeader>>
    task::Task<void> getBlockData(
        std::string_view blockNumberKey, bcos::concepts::block::Block auto& block)
    {
        LEDGER_LOG(DEBUG) << "getBlockData header: " << blockNumberKey;

        auto entry = storage().getRow(SYS_NUMBER_2_BLOCK_HEADER, blockNumberKey);
        if (!entry) [[unlikely]]
        {
            BOOST_THROW_EXCEPTION(
                NotFoundBlockHeader{} << errinfo_comment{"Not found block header!"});
        }

        auto field = entry->getField(0);
        bcos::concepts::serialize::decode(field, block.blockHeader);

        co_return;
    }

    template <std::same_as<BlockDataTransactionsMeta>>
    task::Task<void> getBlockData(
        std::string_view blockNumberKey, bcos::concepts::block::Block auto& block)
    {
        LEDGER_LOG(DEBUG) << "getBlockData transaction metadata: " << blockNumberKey;

        auto entry = storage().getRow(SYS_NUMBER_2_TXS, blockNumberKey);
        if (!entry) [[unlikely]]
        {
            LEDGER_LOG(INFO) << "GetBlock not found transaction meta data!";
            co_return;
        }

        auto field = entry->getField(0);
        std::remove_reference_t<decltype(block)> metadataBlock;
        bcos::concepts::serialize::decode(field, metadataBlock);
        block.transactionsMetaData = std::move(metadataBlock.transactionsMetaData);
        block.transactionsMerkle = std::move(metadataBlock.transactionsMerkle);
        block.receiptsMerkle = std::move(metadataBlock.receiptsMerkle);
    }

    template <class Type>
        requires std::same_as<Type, BlockDataTransactions> ||
             std::same_as<Type, BlockDataReceipts>
    task::Task<void> getBlockData(
        std::string_view blockNumberKey, bcos::concepts::block::Block auto& block)
    {
        LEDGER_LOG(DEBUG) << "getBlockData transactions or receipts: " << blockNumberKey;

        if (RANGES::empty(block.transactionsMetaData))
        {
            LEDGER_LOG(INFO) << "GetBlock not found transaction meta data!";
            co_return;
        }

        auto hashesRange =
            block.transactionsMetaData |
            RANGES::views::transform(
                [](typename decltype(block.transactionsMetaData)::value_type const& metaData)
                    -> auto& { return metaData.hash; });
        auto outputSize = RANGES::size(block.transactionsMetaData);

        if constexpr (std::is_same_v<Type, BlockDataTransactions>)
        {
            bcos::concepts::resizeTo(block.transactions, outputSize);
            co_await impl_getTransactions(std::move(hashesRange), block.transactions);
        }
        else
        {
            bcos::concepts::resizeTo(block.receipts, outputSize);
            co_await impl_getTransactions(std::move(hashesRange), block.receipts);
        }
    }

    template <std::same_as<BlockDataNonces>>
    task::Task<void> getBlockData(
        std::string_view blockNumberKey, bcos::concepts::block::Block auto& block)
    {
        LEDGER_LOG(DEBUG) << "getBlockData nonce: " << blockNumberKey;

        auto entry = storage().getRow(SYS_BLOCK_NUMBER_2_NONCES, blockNumberKey);
        if (!entry)
        {
            LEDGER_LOG(INFO) << "GetBlock not found nonce data!";
            co_return;
        }

        std::remove_reference_t<decltype(block)> nonceBlock;
        auto field = entry->getField(0);
        bcos::concepts::serialize::decode(field, nonceBlock);
        block.nonceList = std::move(nonceBlock.nonceList);
    }

    template <std::same_as<BlockDataAll>>
    task::Task<void> getBlockData(
        std::string_view blockNumberKey, bcos::concepts::block::Block auto& block)
    {
        LEDGER_LOG(DEBUG) << "getBlockData all: " << blockNumberKey;

        try
        {
            co_await getBlockData<BlockDataHeader>(blockNumberKey, block);
            co_await getBlockData<BlockDataTransactionsMeta>(blockNumberKey, block);
            co_await getBlockData<BlockDataTransactions>(blockNumberKey, block);
            co_await getBlockData<BlockDataReceipts>(blockNumberKey, block);
            co_await getBlockData<BlockDataNonces>(blockNumberKey, block);
        }
        catch (std::exception const& e)
        {
            LEDGER_LOG(ERROR) << "getBlockData all failed";
            BOOST_THROW_EXCEPTION(
                GetBlockDataError{} << errinfo_comment{"getBlockData all failed!"});
        }
    }

    template <std::same_as<BlockDataHeader>>
    task::Task<void> setBlockData(
        std::string_view blockNumberKey, bcos::concepts::block::Block auto& block)
    {
        LEDGER_LOG(DEBUG) << "setBlockData header: " << blockNumberKey;

        // number 2 header
        bcos::storage::Entry number2HeaderEntry;
        std::vector<bcos::byte> number2HeaderBuffer;
        bcos::concepts::serialize::encode(block.blockHeader, number2HeaderBuffer);
        number2HeaderEntry.importFields({std::move(number2HeaderBuffer)});
        storage().setRow(SYS_NUMBER_2_BLOCK_HEADER, blockNumberKey, std::move(number2HeaderEntry));

        // number 2 block hash
        bcos::storage::Entry hashEntry;
        hashEntry.importFields({block.blockHeader.dataHash});
        storage().setRow(SYS_NUMBER_2_HASH, blockNumberKey, std::move(hashEntry));

        // block hash 2 number
        bcos::storage::Entry hash2NumberEntry;
        hash2NumberEntry.importFields({std::string(blockNumberKey)});
        storage().setRow(SYS_HASH_2_NUMBER,
            std::string_view{block.blockHeader.dataHash.data(), block.blockHeader.dataHash.size()},
            std::move(hash2NumberEntry));

        // current number
        bcos::storage::Entry numberEntry;
        numberEntry.importFields({std::string(blockNumberKey)});
        storage().setRow(SYS_CURRENT_STATE, SYS_KEY_CURRENT_NUMBER, std::move(numberEntry));

        co_return;
    }

    template <std::same_as<BlockDataNonces>>
    task::Task<void> setBlockData(
        std::string_view blockNumberKey, bcos::concepts::block::Block auto& block)
    {
        LEDGER_LOG(DEBUG) << "setBlockData nonce " << blockNumberKey;

        std::remove_cvref_t<decltype(block)> blockNonceList;
        blockNonceList.nonceList = std::move(block.nonceList);
        bcos::storage::Entry number2NonceEntry;
        std::vector<bcos::byte> number2NonceBuffer;
        bcos::concepts::serialize::encode(blockNonceList, number2NonceBuffer);
        number2NonceEntry.importFields({std::move(number2NonceBuffer)});
        storage().setRow(SYS_BLOCK_NUMBER_2_NONCES, blockNumberKey, std::move(number2NonceEntry));

        co_return;
    }

    template <std::same_as<BlockDataTransactionsMeta>>
    task::Task<void> setBlockData(
        std::string_view blockNumberKey, bcos::concepts::block::Block auto& block)
    {
        LEDGER_LOG(DEBUG) << "setBlockData transaction metadata: " << blockNumberKey;

        if (RANGES::empty(block.transactionsMetaData) && !RANGES::empty(block.transactions))
        {
            block.transactionsMetaData.resize(block.transactions.size());
            tbb::parallel_for(tbb::blocked_range<size_t>(0, block.transactions.size()),
                [&block, this](const tbb::blocked_range<size_t>& range) {
                    for (auto i = range.begin(); i < range.end(); ++i)
                    {
                        bcos::concepts::hash::calculate(block.transactions[i], m_hasher.clone(),
                            block.transactionsMetaData[i].hash);
                    }
                });
        }

        if (std::empty(block.transactionsMetaData))
        {
            LEDGER_LOG(INFO) << "setBlockData not found transaction meta data!";
            co_return;
        }

        std::remove_cvref_t<decltype(block)> transactionsBlock;
        std::swap(block.transactionsMetaData, transactionsBlock.transactionsMetaData);

        bcos::storage::Entry number2TransactionHashesEntry;
        std::vector<bcos::byte> number2TransactionHashesBuffer;
        bcos::concepts::serialize::encode(transactionsBlock, number2TransactionHashesBuffer);
        number2TransactionHashesEntry.importFields({std::move(number2TransactionHashesBuffer)});
        storage().setRow(
            SYS_NUMBER_2_TXS, blockNumberKey, std::move(number2TransactionHashesEntry));
        std::swap(transactionsBlock.transactionsMetaData, block.transactionsMetaData);

        co_return;
    }

    template <std::same_as<BlockDataAll>>
    task::Task<void> setBlockData(
        std::string_view blockNumberKey, bcos::concepts::block::Block auto& block)
    {
        LEDGER_LOG(DEBUG) << "setBlockData all: " << blockNumberKey;

        co_await setBlockData<BlockDataHeader>(blockNumberKey, block);
        co_await setBlockData<BlockDataTransactionsMeta>(blockNumberKey, block);
        co_await setBlockData<BlockDataNonces>(blockNumberKey, block);
    }

    task::Task<void> impl_setupGenesisBlock(bcos::concepts::block::Block auto block)
    {
        try
        {
            decltype(block) currentBlock;

            co_await impl_getBlock<BlockDataHeader>(0, currentBlock);
            co_return;
        }
        catch (NotFoundBlockHeader& e)
        {
            LEDGER_LOG(INFO) << "Not found genesis block, may be not initialized";
        }

        co_await impl_setBlock<BlockDataHeader>(std::move(block));
    }

    task::Task<void> impl_checkGenesisBlock(bcos::concepts::block::Block auto block)
    {
        try
        {
            decltype(block) currentBlock;

            co_await impl_getBlock<BlockDataHeader>(0, currentBlock);
            co_return;
        }
        catch (NotFoundBlockHeader& e)
        {
            LEDGER_LOG(INFO) << "Not found genesis block, may be not initialized";
            BOOST_THROW_EXCEPTION(
                NotFoundBlockHeader{} << errinfo_comment{"Not found genesis block!"});
        }
    }

    task::Task<void> setupGenesisBlock(bcos::concepts::block::Block auto block)
    {
        co_await impl_setupGenesisBlock(std::move(block));
    }

    task::Task<void> checkGenesisBlock(bcos::concepts::block::Block auto block)
    {
        co_await impl_checkGenesisBlock(std::move(block));
    }

private:

    auto& storage() { return bcos::concepts::getRef(m_storage); }

    Hasher m_hasher;
    bcos::storage::StorageInterface::Ptr m_backupStorage;
    Storage m_storage;
    crypto::merkle::Merkle<Hasher> m_merkle;  // Use the default width 2
    uint32_t m_compatibilityVersion = 0;
    size_t m_keyPageSize = 0;
};

}  // namespace bcos::ledger
