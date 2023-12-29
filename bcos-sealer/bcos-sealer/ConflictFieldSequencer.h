#pragma once

#include "bcos-framework/protocol/TransactionMetaData.h"
#include "bcos-utilities/Ranges.h"
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>

namespace bcos::sealer
{

struct SequenceItem
{
    protocol::TransactionMetaData::Ptr transactionMetaData;
    std::vector<size_t> readWriteKeys;
    std::vector<bool> writes;
};

class ConflictFieldSequencer
{
public:
    // 排序交易并生成执行计划
    void sequence(RANGES::range auto& transactions)
        requires std::convertible_to<RANGES::range_value_t<decltype(transactions)>,
            protocol::TransactionMetaData::Ptr>
    {
        std::vector<SequenceItem> sequenceItems;
        sequenceItems.resize(RANGES::size(transactions));

        tbb::parallel_for(
            tbb::blocked_range(0L, RANGES::size(transactions)), [&](auto const& range) {
                for (auto i = range.begin(); i != range.end(); ++i)
                {
                    auto& sequenceItem = sequenceItems[i];
                    sequenceItem.transactionMetaData = transactions[i];
                }
            });


        RANGES::sort(transactions, [](protocol::TransactionMetaData::Ptr const& lhs,
                                       protocol::TransactionMetaData::Ptr const& rhs) {
            auto lhsConflictFields = lhs->conflictFields();
            auto rhsConflictFields = rhs->conflictFields();


            return lhs->hash() < rhs->hash();
        });
    }
};

}  // namespace bcos::sealer