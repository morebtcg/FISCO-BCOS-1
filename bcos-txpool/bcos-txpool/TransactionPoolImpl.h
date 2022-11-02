#pragma once
#include "TxPool.h"
#include <bcos-concepts/front/Front.h>
#include <bcos-concepts/transaction_pool/TransactionPool.h>
#include <bcos-task/Task.h>

namespace bcos::txpool
{

template <concepts::front::Front Front>
class TransactionPoolImpl
  : public TxPool,
    public concepts::transacton_pool::TransactionPoolBase<TransactionPoolImpl<Front>>
{
public:
    using TxPool::TxPool;

    task::Task<void> impl_submitTransaction(concepts::transaction::Transaction auto transaction,
        bcos::concepts::receipt::TransactionReceipt auto& receipt)
    {
        static_assert(!sizeof(transaction), "Reach wrong path!");

        co_return;
    }

private:
    Front m_front;
};
}  // namespace bcos::txpool