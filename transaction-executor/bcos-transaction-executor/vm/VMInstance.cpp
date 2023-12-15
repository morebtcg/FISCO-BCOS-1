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
    static thread_local std::stack<std::unique_ptr<evmone::advanced::AdvancedExecutionState>>
        executionStates;

    std::unique_ptr<evmone::advanced::AdvancedExecutionState> executionState;
    if (!executionStates.empty())
    {
        executionState = std::move(executionStates.top());
        executionStates.pop();
    }
    else
    {
        executionState = std::make_unique<evmone::advanced::AdvancedExecutionState>();
    }

    executionState->reset(
        *msg, rev, *host, context, std::basic_string_view<uint8_t>(code, codeSize));
    auto result = EVMCResult(evmone::baseline::execute(
        *static_cast<evmone::VM const*>(evm), msg->gas, *executionState, *m_instance));
    executionStates.push(std::move(executionState));
    return result;
}

void bcos::transaction_executor::VMInstance::enableDebugOutput() {}
