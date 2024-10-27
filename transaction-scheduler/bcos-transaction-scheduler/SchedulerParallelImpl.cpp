#include "SchedulerParallelImpl.h"

bool bcos::transaction_scheduler::compareConflictKeys(
    const protocol::Transaction& lhs, const protocol::Transaction& rhs)
{
    for (auto&& [lhs, rhs] : ::ranges::views::zip(lhs.conflictKeys(), rhs.conflictKeys()))
    {
        if (auto cmp = lhs <=> rhs; !std::is_eq(cmp))
        {
            return std::is_lt(cmp);
        }
    }

    return lhs.conflictKeys().size() < rhs.conflictKeys().size();
}
