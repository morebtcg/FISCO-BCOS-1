#include "HostContext.h"
#include <memory_resource>

evmc_bytes32 bcos::transaction_executor::evm_hash_fn(const uint8_t* data, size_t size)
{
    return toEvmC(executor::GlobalHashImpl::g_hashImpl->hash(bytesConstRef(data, size)));
}

bcos::executor::VMSchedule const& bcos::transaction_executor::vmSchedule()
{
    return executor::FiscoBcosScheduleV320;
}

std::pmr::memory_resource* bcos::transaction_executor::globalMemoryResource()
{
    static std::pmr::synchronized_pool_resource poolResource;
    return std::addressof(poolResource);
}
