#include "VMInstance.h"

bcos::transaction_executor::VMInstance::VMInstance(
    std::shared_ptr<evmone::baseline::CodeAnalysis const> instance) noexcept
  : m_instance(std::move(instance))
{}

bcos::transaction_executor::EVMCResult bcos::transaction_executor::VMInstance::execute(
    const struct evmc_host_interface* host, struct evmc_host_context* context, evmc_revision rev,
    const evmc_message* msg, const uint8_t* code, size_t codeSize)
{
    static auto const* evm = evmc_create_evmone();

    // FIXME: 使用thread_local可能在tbb协程切换时出问题！可以用多线程对象池解决
    // FIXME: Using thread_local may have problems switching TBB coroutines! It can be solved with a
    // multi-threaded object pool
    static thread_local evmone::advanced::AdvancedExecutionState executionState;

    executionState.reset(
        *msg, rev, *host, context, std::basic_string_view<uint8_t>(code, codeSize));
    return EVMCResult(evmone::baseline::execute(
        *static_cast<evmone::VM const*>(evm), msg->gas, executionState, *m_instance));
}

void bcos::transaction_executor::VMInstance::enableDebugOutput() {}
