#pragma once

#include <boost/property_tree/ptree_fwd.hpp>
#include <cstdint>

namespace bcos::tool
{
class SyncConfig
{
public:
    bool allowFreeNodeSync() const { return m_allowFreeNodeSync; }
    void setAllowFreeNodeSync(bool allowFreeNodeSync)
    {
        m_allowFreeNodeSync = allowFreeNodeSync;
    }

    bool enableSendBlockStatusByTree() const { return m_enableSendBlockStatusByTree; }
    void setEnableSendBlockStatusByTree(bool enableSendBlockStatusByTree)
    {
        m_enableSendBlockStatusByTree = enableSendBlockStatusByTree;
    }

    bool enableSendTxByTree() const { return m_enableSendTxByTree; }
    void setEnableSendTxByTree(bool enableSendTxByTree)
    {
        m_enableSendTxByTree = enableSendTxByTree;
    }

    std::uint32_t treeWidth() const { return m_treeWidth; }
    void setTreeWidth(std::uint32_t treeWidth) { m_treeWidth = treeWidth; }

    void loadSyncConfig(boost::property_tree::ptree const& config);

private:
    bool m_allowFreeNodeSync = false;
    bool m_enableSendBlockStatusByTree = false;
    bool m_enableSendTxByTree = false;
    std::uint32_t m_treeWidth = 3;
};
}  // namespace bcos::tool