#include "../bcos-transaction-executor/TransactionExecutorImpl.h"
#include "TestBytecode.h"
#include "TestMemoryStorage.h"
#include "bcos-codec/bcos-codec/abi/ContractABICodec.h"
#include "bcos-framework/transaction-executor/TransactionExecutor.h"
#include <bcos-crypto/hash/Keccak256.h>
#include <bcos-tars-protocol/protocol/BlockHeaderImpl.h>
#include <bcos-tars-protocol/protocol/TransactionFactoryImpl.h>
#include <bcos-tars-protocol/protocol/TransactionReceiptFactoryImpl.h>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <limits>
#include <memory>
#include <memory_resource>

using namespace bcos;
using namespace bcos::storage2;
using namespace bcos::transaction_executor;

class TestTransactionExecutorImplFixture
{
public:
    MutableStorage storage;
    ledger::LedgerConfig ledgerConfig;
    std::shared_ptr<bcos::crypto::CryptoSuite> cryptoSuite =
        std::make_shared<bcos::crypto::CryptoSuite>(
            bcos::executor::GlobalHashImpl::g_hashImpl, nullptr, nullptr);
    bcostars::protocol::TransactionFactoryImpl transactionFactory{cryptoSuite};
    bcostars::protocol::TransactionReceiptFactoryImpl receiptFactory{cryptoSuite};
    PrecompiledManager precompiledManager{bcos::executor::GlobalHashImpl::g_hashImpl};
    bcos::transaction_executor::TransactionExecutorImpl executor{
        receiptFactory, bcos::executor::GlobalHashImpl::g_hashImpl, precompiledManager};
};

BOOST_FIXTURE_TEST_SUITE(TransactionExecutorImpl, TestTransactionExecutorImplFixture)

BOOST_AUTO_TEST_CASE(execute)
{
    task::syncWait([this]() mutable -> task::Task<void> {
        bcostars::protocol::BlockHeaderImpl blockHeader(
            [inner = bcostars::BlockHeader()]() mutable { return std::addressof(inner); });
        blockHeader.setVersion((uint32_t)bcos::protocol::BlockVersion::V3_1_VERSION);
        blockHeader.calculateHash(*bcos::executor::GlobalHashImpl::g_hashImpl);

        bcos::bytes helloworldBytecodeBinary;
        boost::algorithm::unhex(helloworldBytecode, std::back_inserter(helloworldBytecodeBinary));
        // First deploy
        auto transaction =
            transactionFactory.createTransaction(0, "", helloworldBytecodeBinary, {}, 0, "", "", 0);
        auto receipt = co_await bcos::transaction_executor::executeTransaction(
            executor, storage, blockHeader, *transaction, 0, ledgerConfig, task::syncWait);
        BOOST_CHECK_EQUAL(receipt->status(), 0);
        BOOST_CHECK_EQUAL(receipt->contractAddress(), "e0e794ca86d198042b64285c5ce667aee747509b");

        // Set the value
        bcos::codec::abi::ContractABICodec abiCodec(*bcos::executor::GlobalHashImpl::g_hashImpl);
        auto input = abiCodec.abiIn("setInt(int256)", bcos::s256(10099));
        auto transaction2 = transactionFactory.createTransaction(
            0, std::string(receipt->contractAddress()), input, {}, 0, "", "", 0);
        auto receipt2 = co_await bcos::transaction_executor::executeTransaction(
            executor, storage, blockHeader, *transaction2, 1, ledgerConfig, task::syncWait);
        BOOST_CHECK_EQUAL(receipt2->status(), 0);

        // Get the value
        auto input2 = abiCodec.abiIn("getInt()");
        auto transaction3 = transactionFactory.createTransaction(
            0, std::string(receipt->contractAddress()), input2, {}, 0, "", "", 0);
        auto receipt3 = co_await bcos::transaction_executor::executeTransaction(
            executor, storage, blockHeader, *transaction3, 2, ledgerConfig, task::syncWait);
        BOOST_CHECK_EQUAL(receipt3->status(), 0);
        bcos::s256 getIntResult = -1;
        abiCodec.abiOut(receipt3->output(), getIntResult);
        BOOST_CHECK_EQUAL(getIntResult, 10099);
    }());
}
BOOST_AUTO_TEST_CASE(transientStorageTest)
{
    task::syncWait([this]() mutable -> task::Task<void> {
        bcostars::protocol::BlockHeaderImpl blockHeader(
            [inner = bcostars::BlockHeader()]() mutable { return std::addressof(inner); });
        blockHeader.setVersion((uint32_t)bcos::protocol::BlockVersion::V3_7_0_VERSION);
        blockHeader.calculateHash(*bcos::executor::GlobalHashImpl::g_hashImpl);

        // turn on evm_cancun feature
        ledger::Features features;
        features.set(ledger::Features::Flag::feature_evm_cancun);
        ledgerConfig.setFeatures(features);

        bcos::bytes transientStorageBinary;
        boost::algorithm::unhex(
            transientStorageBytecode, std::back_inserter(transientStorageBinary));
        // First deploy
        auto transaction =
            transactionFactory.createTransaction(0, "", transientStorageBinary, {}, 0, "", "", 0);
        auto receipt = co_await bcos::transaction_executor::executeTransaction(
            executor, storage, blockHeader, *transaction, 3, ledgerConfig, task::syncWait);
        BOOST_CHECK_EQUAL(receipt->status(), 0);

        // test read and write transient storage
        bcos::codec::abi::ContractABICodec abiCodec(*bcos::executor::GlobalHashImpl::g_hashImpl);
        auto input = abiCodec.abiIn("storeIntTest(int256)", bcos::s256(10000));
        auto transaction2 = transactionFactory.createTransaction(
            0, std::string(receipt->contractAddress()), input, {}, 0, "", "", 0);
        auto receipt2 = co_await bcos::transaction_executor::executeTransaction(
            executor, storage, blockHeader, *transaction2, 4, ledgerConfig, task::syncWait);
        BOOST_CHECK_EQUAL(receipt2->status(), 0);
        bcos::s256 getIntResult = -1;
        abiCodec.abiOut(receipt2->output(), getIntResult);
        BOOST_CHECK_EQUAL(getIntResult, 10000);
    }());
}

BOOST_AUTO_TEST_CASE(transientStorageContractTest)
{
    task::syncWait([this]() mutable -> task::Task<void> {
        auto cryptoSuite = std::make_shared<bcos::crypto::CryptoSuite>(
            bcos::executor::GlobalHashImpl::g_hashImpl, nullptr, nullptr);

        PrecompiledManager precompiledManager(bcos::executor::GlobalHashImpl::g_hashImpl);
        bcos::transaction_executor::TransactionExecutorImpl executor(
            receiptFactory, bcos::executor::GlobalHashImpl::g_hashImpl, precompiledManager);
        bcostars::protocol::BlockHeaderImpl blockHeader(
            [inner = bcostars::BlockHeader()]() mutable { return std::addressof(inner); });
        blockHeader.setVersion((uint32_t)bcos::protocol::BlockVersion::V3_1_VERSION);
        blockHeader.calculateHash(*bcos::executor::GlobalHashImpl::g_hashImpl);

        // turn on evm_cancun feature
        ledger::Features features;
        features.set(ledger::Features::Flag::feature_evm_cancun);
        ledgerConfig.setFeatures(features);

        bcos::bytes transientStorageBinary;
        boost::algorithm::unhex(
            transientStorageContractTestByteCode, std::back_inserter(transientStorageBinary));
        // First deploy
        auto transaction =
            transactionFactory.createTransaction(0, "", transientStorageBinary, {}, 0, "", "", 0);
        auto receipt = co_await bcos::transaction_executor::executeTransaction(
            executor, storage, blockHeader, *transaction, 5, ledgerConfig, task::syncWait);
        BOOST_CHECK_EQUAL(receipt->status(), 0);

        // test read and write transient storage
        bcos::codec::abi::ContractABICodec abiCodec(*bcos::executor::GlobalHashImpl::g_hashImpl);
        auto input = abiCodec.abiIn("checkAndVerifyIntValue(int256)", bcos::h256(12345));
        auto transaction2 = transactionFactory.createTransaction(
            0, std::string(receipt->contractAddress()), input, {}, 0, "", "", 0);
        auto receipt2 = co_await bcos::transaction_executor::executeTransaction(
            executor, storage, blockHeader, *transaction2, 6, ledgerConfig, task::syncWait);
        BOOST_CHECK_EQUAL(receipt2->status(), 0);
        bool checkResult = false;
        abiCodec.abiOut(receipt2->output(), checkResult);
        BOOST_CHECK_EQUAL(checkResult, true);
    }());
}

struct TestMemoryResource : public std::pmr::memory_resource
{
    size_t m_size = std::numeric_limits<size_t>::max();
    int m_count = 0;

    void* do_allocate(std::size_t bytes, std::size_t alignment) override
    {
        m_size = bytes;
        ++m_count;
        return std::malloc(bytes);
    }
    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override { std::free(p); }
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == &other;
    }
};

BOOST_AUTO_TEST_CASE(testExecuteStackSize)
{
    task::syncWait([this]() -> task::Task<void> {
        bcostars::protocol::BlockHeaderImpl blockHeader(
            [inner = bcostars::BlockHeader()]() mutable { return std::addressof(inner); });
        blockHeader.setVersion((uint32_t)bcos::protocol::BlockVersion::V3_1_VERSION);
        blockHeader.calculateHash(*bcos::executor::GlobalHashImpl::g_hashImpl);

        bcos::bytes helloworldBytecodeBinary;
        boost::algorithm::unhex(helloworldBytecode, std::back_inserter(helloworldBytecodeBinary));
        // First deploy
        auto transaction =
            transactionFactory.createTransaction(0, "", helloworldBytecodeBinary, {}, 0, "", "", 0);

        TestMemoryResource memoryResource;
        auto generator = bcos::transaction_executor::execute3Step(executor, storage, blockHeader,
            *transaction, 0, ledgerConfig, task::syncWait, std::allocator_arg,
            std::pmr::polymorphic_allocator<>(std::addressof(memoryResource)));
        protocol::TransactionReceipt::Ptr receipt;
        for (auto currentReceipt : generator)
        {
            BOOST_CHECK_LT(memoryResource.m_size, transaction_executor::EXECUTOR_STACK);
            if (currentReceipt)
            {
                receipt = currentReceipt;
            }
        }

        BOOST_CHECK_EQUAL(memoryResource.m_count, 1);
        BOOST_CHECK(receipt);
        BOOST_CHECK_EQUAL(receipt->status(), 0);
        BOOST_CHECK_EQUAL(receipt->contractAddress(), "e0e794ca86d198042b64285c5ce667aee747509b");

        co_return;
    }());
}

BOOST_AUTO_TEST_SUITE_END()