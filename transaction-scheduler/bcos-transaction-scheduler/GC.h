#pragma once
#include <oneapi/tbb/task_arena.h>

namespace bcos::transaction_scheduler
{
class GC
{
public:
    static void collect(auto&&... resources)
    {
        static tbb::task_arena arena(1, 1, tbb::task_arena::priority::low);
        arena.enqueue([resources = std::make_tuple(
                           std::forward<decltype(resources)>(resources)...)]() noexcept {});
    }
};
}  // namespace bcos::transaction_scheduler