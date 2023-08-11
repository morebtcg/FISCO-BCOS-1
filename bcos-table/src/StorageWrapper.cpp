#include "StorageWrapper.h"

std::vector<std::string> bcos::storage::StorageWrapper::getPrimaryKeys(
    const std::string_view& table, const std::optional<storage::Condition const>& _condition)
{
    GetPrimaryKeysReponse value;
    m_storage->asyncGetPrimaryKeys(table, _condition, [&value](auto&& error, auto&& keys) mutable {
        value = {std::forward<decltype(error)>(error), std::forward<decltype(keys)>(keys)};
    });

    // After coroutine switch, set the recoder
    setRecoder(m_recoder);

    auto& [error, keys] = value;

    if (error)
    {
        BOOST_THROW_EXCEPTION(*error);
    }

    return std::move(keys);
}
std::optional<bcos::storage::Entry> bcos::storage::StorageWrapper::getRow(
    const std::string_view& table, const std::string_view& _key)
{
    if (m_codeCache && table.compare(M_SYS_CODE_BINARY) == 0)
    {
        auto it = m_codeCache->find(std::string(_key));
        if (it != m_codeCache->end())
        {
            return it->second;
        }

        auto code = getRowInternal(table, _key);
        if (code.has_value())
        {
            m_codeCache->emplace(std::string(_key), code);
        }

        return code;
    }

    if (m_codeHashCache && _key.compare(M_ACCOUNT_CODE_HASH) == 0)
    {
        auto it = m_codeHashCache->find(std::string(table));
        if (it != m_codeHashCache->end())
        {
            return it->second;
        }

        auto codeHash = getRowInternal(table, _key);
        if (codeHash.has_value())
        {
            m_codeHashCache->emplace(std::string(table), codeHash);
        }

        return codeHash;
    }

    return getRowInternal(table, _key);
}
std::vector<std::optional<bcos::storage::Entry>> bcos::storage::StorageWrapper::getRows(
    const std::string_view& table,
    RANGES::any_view<std::string_view,
        RANGES::category::input | RANGES::category::random_access | RANGES::category::sized>
        keys)
{
    GetRowsResponse value;
    m_storage->asyncGetRows(table, std::move(keys), [&value](auto&& error, auto&& entries) mutable {
        value = {std::forward<decltype(error)>(error), std::forward<decltype(entries)>(entries)};
    });


    auto& [error, entries] = value;

    if (error)
    {
        BOOST_THROW_EXCEPTION(*error);
    }

    return std::move(entries);
}
void bcos::storage::StorageWrapper::setRow(
    const std::string_view& table, const std::string_view& key, storage::Entry entry)
{
    SetRowResponse value;

    m_storage->asyncSetRow(table, key, std::move(entry), [&value](auto&& error) mutable {
        value = std::tuple{std::forward<decltype(error)>(error)};
    });

    auto& [error] = value;

    if (error)
    {
        BOOST_THROW_EXCEPTION(*error);
    }
}
std::optional<bcos::storage::Table> bcos::storage::StorageWrapper::createTable(
    std::string _tableName, std::string _valueFields)
{
    auto ret = createTableWithoutException(std::move(_tableName), std::move(_valueFields));
    if (std::get<0>(ret))
    {
        BOOST_THROW_EXCEPTION(*(std::get<0>(ret)));
    }

    return std::get<1>(ret);
}

std::optional<bcos::storage::Table> bcos::storage::StorageWrapper::openTable(
    std::string_view tableName)
{
    auto ret = openTableWithoutException(tableName);
    if (std::get<0>(ret))
    {
        BOOST_THROW_EXCEPTION(*(std::get<0>(ret)));
    }

    return std::get<1>(ret);
}
std::pair<size_t, bcos::Error::Ptr> bcos::storage::StorageWrapper::count(
    const std::string_view& _table)
{
    return m_storage->count(_table);
}
void bcos::storage::StorageWrapper::setRecoder(storage::Recoder::Ptr recoder)
{
    m_storage->setRecoder(std::move(recoder));
}
void bcos::storage::StorageWrapper::setCodeCache(EntryCachePtr cache)
{
    m_codeCache = std::move(cache);
}
void bcos::storage::StorageWrapper::setCodeHashCache(EntryCachePtr cache)
{
    m_codeHashCache = std::move(cache);
}
std::optional<bcos::storage::Entry> bcos::storage::StorageWrapper::getRowInternal(
    const std::string_view& table, const std::string_view& _key)
{
    GetRowResponse value;
    m_storage->asyncGetRow(table, _key, [&value](auto&& error, auto&& entry) mutable {
        value = {std::forward<decltype(error)>(error), std::forward<decltype(entry)>(entry)};
    });

    auto& [error, entry] = value;

    if (error)
    {
        BOOST_THROW_EXCEPTION(*error);
    }

    return std::move(entry);
}
std::tuple<bcos::Error::UniquePtr, std::optional<bcos::storage::Table>>
bcos::storage::StorageWrapper::openTableWithoutException(std::string_view tableName)
{
    std::promise<OpenTableResponse> openPromise;
    m_storage->asyncOpenTable(tableName, [&](auto&& error, auto&& table) mutable {
        openPromise.set_value(
            {std::forward<decltype(error)>(error), std::forward<decltype(table)>(table)});
    });
    auto value = openPromise.get_future().get();
    return value;
}
std::tuple<bcos::Error::UniquePtr, std::optional<bcos::storage::Table>>
bcos::storage::StorageWrapper::createTableWithoutException(
    std::string _tableName, std::string _valueFields)
{
    std::promise<OpenTableResponse> createPromise;
    m_storage->asyncCreateTable(std::move(_tableName), std::move(_valueFields),
        [&](Error::UniquePtr&& error, auto&& table) mutable {
            createPromise.set_value({std::move(error), std::forward<decltype(table)>(table)});
        });
    auto value = createPromise.get_future().get();
    return value;
}