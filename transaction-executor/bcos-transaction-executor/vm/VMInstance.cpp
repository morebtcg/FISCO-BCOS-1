#include "VMInstance.h"

bcos::transaction_executor::VMInstance::VMInstance(
    std::shared_ptr<evmone::baseline::CodeAnalysis const> instance) noexcept
  : m_instance(std::move(instance))
{}

bcos::transaction_executor::EVMCResult bcos::transaction_executor::VMInstance::execute(
    const struct evmc_host_interface* host, struct evmc_host_context* context, evmc_revision rev,
    const evmc_message* msg, const uint8_t* code, size_t codeSize)
{
    // if (!m_executionState)
    // {
    //     prepareExecutionState(host, context, rev, msg, code, codeSize);
    // }
    static evmone::advanced::AdvancedExecutionState executionState;
    static thread_local auto const* evm = evmc_create_evmone();
    executionState.reset(
        *msg, rev, *host, context, std::basic_string_view<uint8_t>(code, codeSize));
    return EVMCResult(evmone::baseline::execute(
        *static_cast<evmone::VM const*>(evm), msg->gas, executionState, *m_instance));
}

void bcos::transaction_executor::VMInstance::enableDebugOutput() {}
