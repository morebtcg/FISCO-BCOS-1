#pragma once
#include "StateStorageInterface.h"
#include "bcos-framework/storage/Entry.h"
#include "bcos-framework/storage/StorageInterface.h"
#include <optional>
#include <string>
#include <vector>

namespace bcos::storage
{
class StorageWrapperInterface
{
public:
    StorageWrapperInterface() = default;
    StorageWrapperInterface(const StorageWrapperInterface&) = default;
    StorageWrapperInterface(StorageWrapperInterface&&) = default;
    StorageWrapperInterface& operator=(const StorageWrapperInterface&) = default;
    StorageWrapperInterface& operator=(StorageWrapperInterface&&) = default;
    virtual ~StorageWrapperInterface() = default;

    virtual std::vector<std::string> getPrimaryKeys(const std::string_view& table,
        const std::optional<storage::Condition const>& _condition) = 0;
    virtual std::optional<storage::Entry> getRow(
        const std::string_view& table, const std::string_view& _key) = 0;
    virtual std::vector<std::optional<storage::Entry>> getRows(const std::string_view& table,
        RANGES::any_view<std::string_view,
            RANGES::category::input | RANGES::category::random_access | RANGES::category::sized>
            keys) = 0;
    virtual void setRow(
        const std::string_view& table, const std::string_view& key, storage::Entry entry) = 0;
    virtual std::optional<storage::Table> createTable(
        std::string _tableName, std::string _valueFields) = 0;
    virtual std::optional<storage::Table> openTable(std::string_view tableName) = 0;
    virtual std::pair<size_t, Error::Ptr> count(const std::string_view& _table) = 0;
};
}  // namespace bcos::storage