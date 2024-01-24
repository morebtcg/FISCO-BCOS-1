#pragma once
#include "../protocol/Protocol.h"
#include "../storage/Entry.h"
#include "../storage/LegacyStorageMethods.h"
#include "../storage2/Storage.h"
#include "bcos-concepts/Exception.h"
#include "bcos-framework/ledger/LedgerTypeDef.h"
#include "bcos-framework/transaction-executor/StateKey.h"
#include "bcos-task/Task.h"
#include "bcos-tool/Exceptions.h"
#include <bcos-utilities/Ranges.h>
#include <boost/throw_exception.hpp>
#include <array>
#include <bitset>
#include <magic_enum.hpp>

namespace bcos::ledger
{

struct NoSuchFeatureError : public bcos::error::Exception
{
};

class Features
{
public:
    // Use for storage key, do not change the enum name!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // At most 256 flag
    enum class Flag
    {
        bugfix_revert,  // https://github.com/FISCO-BCOS/FISCO-BCOS/issues/3629
        bugfix_statestorage_hash,
        bugfix_evm_create2_delegatecall_staticcall_codecopy,
        bugfix_event_log_order,
        bugfix_call_noaddr_return,
        bugfix_precompiled_codehash,
        feature_dmc2serial,
        feature_sharding,
        feature_rpbft,
        feature_paillier,
        feature_balance,
        feature_balance_precompiled,
        feature_balance_policy1,
        feature_paillier_add_raw,
        feature_predeploy,
    };

private:
    std::bitset<magic_enum::enum_count<Flag>()> m_flags;

public:
    static Flag string2Flag(std::string_view str)
    {
        auto value = magic_enum::enum_cast<Flag>(str);
        if (!value)
        {
            BOOST_THROW_EXCEPTION(NoSuchFeatureError{});
        }
        return *value;
    }

    void validate(std::string_view flag) const
    {
        auto value = magic_enum::enum_cast<Flag>(flag);
        if (!value)
        {
            BOOST_THROW_EXCEPTION(NoSuchFeatureError{});
        }

        validate(*value);
    }

    void validate(Flag flag) const
    {
        if (flag == Flag::feature_balance_precompiled && !get(Flag::feature_balance))
        {
            BOOST_THROW_EXCEPTION(bcos::tool::InvalidSetFeature{}
                                  << errinfo_comment("must set feature_balance first"));
        }
        if (flag == Flag::feature_balance_policy1 && !get(Flag::feature_balance_precompiled))
        {
            BOOST_THROW_EXCEPTION(bcos::tool::InvalidSetFeature{}
                                  << errinfo_comment("must set feature_balance_precompiled first"));
        }
    }

    bool get(Flag flag) const
    {
        auto index = magic_enum::enum_index(flag);
        if (!index)
        {
            BOOST_THROW_EXCEPTION(NoSuchFeatureError{});
        }

        return m_flags[*index];
    }
    bool get(std::string_view flag) const { return get(string2Flag(flag)); }

    void set(Flag flag)
    {
        auto index = magic_enum::enum_index(flag);
        if (!index)
        {
            BOOST_THROW_EXCEPTION(NoSuchFeatureError{});
        }

        validate(flag);
        m_flags[*index] = true;
    }
    void set(std::string_view flag) { set(string2Flag(flag)); }

    void setToShardingDefault(protocol::BlockVersion version)
    {
        if (version >= protocol::BlockVersion::V3_3_VERSION &&
            version <= protocol::BlockVersion::V3_4_VERSION)
        {
            set(Flag::feature_sharding);
        }
    }

    void setToDefault(protocol::BlockVersion version)
    {
        if (version >= protocol::BlockVersion::V3_2_3_VERSION)
        {
            set(Flag::bugfix_revert);
        }
        if (version >= protocol::BlockVersion::V3_2_4_VERSION)
        {
            set(Flag::bugfix_statestorage_hash);
            set(Flag::bugfix_evm_create2_delegatecall_staticcall_codecopy);
        }
        if (version >= protocol::BlockVersion::V3_2_6_VERSION)
        {
            set(Flag::bugfix_event_log_order);
            set(Flag::bugfix_call_noaddr_return);
            set(Flag::bugfix_precompiled_codehash);
        }

        setToShardingDefault(version);
    }

    auto flags() const
    {
        return RANGES::views::iota(0LU, m_flags.size()) |
               RANGES::views::transform([this](size_t index) {
                   auto flag = magic_enum::enum_value<Flag>(index);
                   return std::make_tuple(flag, magic_enum::enum_name(flag), m_flags[index]);
               });
    }

    static auto featureKeys()
    {
        return RANGES::views::iota(0LU, magic_enum::enum_count<Flag>()) |
               RANGES::views::transform([](size_t index) {
                   auto flag = magic_enum::enum_value<Flag>(index);
                   return magic_enum::enum_name(flag);
               });
    }
};

inline task::Task<void> readFromStorage(Features& features, auto&& storage, long blockNumber)
{
    decltype(auto) keys = bcos::ledger::Features::featureKeys();
    auto entries = co_await storage2::readSome(std::forward<decltype(storage)>(storage),
        keys | RANGES::views::transform([](std::string_view key) {
            return transaction_executor::StateKeyView(ledger::SYS_CONFIG, key);
        }));
    for (auto&& [key, entry] : RANGES::views::zip(keys, entries))
    {
        if (entry)
        {
            auto [value, enableNumber] = entry->template getObject<ledger::SystemConfigEntry>();
            if (blockNumber >= enableNumber)
            {
                features.set(key);
            }
        }
    }
}

inline task::Task<void> writeToStorage(Features const& features, auto&& storage, long blockNumber)
{
    decltype(auto) flags =
        features.flags() | RANGES::views::filter([](auto&& tuple) { return std::get<2>(tuple); });
    co_await storage2::writeSome(std::forward<decltype(storage)>(storage),
        RANGES::views::transform(flags,
            [](auto&& tuple) {
                return transaction_executor::StateKey(ledger::SYS_CONFIG, std::get<1>(tuple));
            }),
        RANGES::views::transform(flags, [&](auto&& tuple) {
            storage::Entry entry;
            entry.setObject(SystemConfigEntry{
                boost::lexical_cast<std::string>((int)std::get<2>(tuple)), blockNumber});
            return entry;
        }));
}

inline std::ostream& operator<<(std::ostream& stream, Features::Flag flag)
{
    stream << magic_enum::enum_name(flag);
    return stream;
}

}  // namespace bcos::ledger
