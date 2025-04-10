#include "VMFactory.h"

bcos::executor_v1::VMInstance bcos::executor_v1::VMFactory::create(
    VMKind kind, bytesConstRef code, evmc_revision mode)
{
    switch (kind)
    {
    case VMKind::evmone:
    {
        return VMInstance{
            std::make_shared<evmone::baseline::CodeAnalysis>(evmone::baseline::analyze(
                mode, evmone::bytes_view((const uint8_t*)code.data(), code.size())))};
    }
    default:
        BOOST_THROW_EXCEPTION(UnknownVMError{});
    }
}
