#include "SyncConfig.h"
#include <boost/property_tree/ptree.hpp>

using namespace bcos::tool;

void SyncConfig::loadSyncConfig(boost::property_tree::ptree const& config)
{
    setAllowFreeNodeSync(config.get<bool>("sync.allow_free_node", false));
    setEnableSendBlockStatusByTree(config.get<bool>("sync.sync_block_by_tree", false));
    setEnableSendTxByTree(config.get<bool>("sync.send_txs_by_tree", false));
    setTreeWidth(config.get<std::uint32_t>("sync.tree_width", 3));
}
