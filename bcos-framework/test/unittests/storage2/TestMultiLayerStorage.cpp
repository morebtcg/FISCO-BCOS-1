#include "bcos-framework/storage2/MemoryStorage.h"
#include "bcos-framework/storage2/Storage.h"
#include "bcos-framework/transaction-executor/StateKey.h"
#include "bcos-task/Wait.h"
#include "bcos-framework/storage2/MultiLayerStorage.h"
#include <fmt/format.h>
#include <boost/test/unit_test.hpp>
#include <variant>
#include <string_view>

using namespace bcos;
using namespace bcos::storage2;
using namespace bcos::executor_v1;
using namespace std::string_view_literals;

class TestMultiLayerStorageFixture
{
public:
    using MutableStorage = memory_storage::MemoryStorage<StateKey, StateValue,
        memory_storage::Attribute(memory_storage::ORDERED | memory_storage::LOGICAL_DELETION)>;
    using BackendStorage = memory_storage::MemoryStorage<StateKey, StateValue,
        memory_storage::Attribute(memory_storage::ORDERED | memory_storage::CONCURRENT),
        std::hash<StateKey>>;

    TestMultiLayerStorageFixture() : multiLayerStorage(backendStorage) {}

    BackendStorage backendStorage;
    MultiLayerStorage<MutableStorage, void, BackendStorage> multiLayerStorage;
};

using MLS = MultiLayerStorage<TestMultiLayerStorageFixture::MutableStorage, void,
    TestMultiLayerStorageFixture::BackendStorage>;

static task::Task<void> run_noMutable(MLS& mls)
{
    auto view = mls.fork();
    storage::Entry entry;
    bool caught = false;
    try
    {
        co_await storage2::writeOne(view, StateKey{"test_table"sv, "test_key"sv}, std::move(entry));
    }
    catch (const NotExistsMutableStorageError&)
    {
        caught = true;
    }
    BOOST_CHECK(caught);
    co_return;
}

static task::Task<void> run_readWriteMutable(MLS& mls)
{
    auto view = std::make_optional(mls.fork());
    view->newMutable();
    StateKey key{"test_table"sv, "test_key"sv};

    storage::Entry entry;
    entry.set("Hello world!");
    co_await storage2::writeOne(*view, key, entry);

    ::ranges::single_view keyViews(key);
    auto values = co_await storage2::readSome(*view, keyViews);

    BOOST_CHECK_EQUAL(values[0]->get(), entry.get());
    // pushView shouldn't throw
    try
    {
        mls.pushView(std::move(*view));
    }
    catch (...)
    {
        BOOST_CHECK(false);
    }

    auto view2 = mls.fork();
    bool caught2 = false;
    try
    {
        co_await storage2::writeOne(view2, key, entry);
    }
    catch (const NotExistsMutableStorageError&)
    {
        caught2 = true;
    }
    BOOST_CHECK(caught2);
    co_return;
}

static task::Task<void> run_merge(MLS& mls)
{
    auto view = std::make_optional(mls.fork());
    view->newMutable();
    auto toKey = ::ranges::views::transform(
        [](int num) { return StateKey{"test_table"sv, fmt::format("key: {}", num)}; });
    auto toValue = ::ranges::views::transform([](int num) {
        storage::Entry entry;
        entry.set(fmt::format("value: {}", num));
        return entry;
    });

    constexpr int kCount = 100;
    co_await storage2::writeSome(*view,
        ::ranges::views::zip(::ranges::iota_view<int, int>(0, kCount) | toKey,
            ::ranges::iota_view<int, int>(0, kCount) | toValue));

    bool caught3 = false;
    try
    {
        co_await mls.mergeBackStorage();
    }
    catch (const NotExistsImmutableStorageError&)
    {
        caught3 = true;
    }
    BOOST_CHECK(caught3);

    mls.pushView(std::move(*view));
    co_await mls.mergeBackStorage();

    auto view2 = mls.fork();
    auto keys = ::ranges::iota_view<int, int>(0, kCount) | toKey;
    auto values = co_await storage2::readSome(view2, keys);

    for (auto&& [index, value] : ::ranges::views::enumerate(values))
    {
        BOOST_CHECK_EQUAL(value->get(), fmt::format("value: {}", index));
    }
    BOOST_CHECK_EQUAL(::ranges::size(values), kCount);

    auto view3 = mls.fork();
    view3.newMutable();
    constexpr int kRemoveL = 20;
    constexpr int kRemoveR = 30;
    co_await storage2::removeSome(view3, ::ranges::iota_view<int, int>(kRemoveL, kRemoveR) | toKey);
    mls.pushView(std::move(view3));
    co_await mls.mergeBackStorage();

    auto values2 = co_await storage2::readSome(view3, keys);
    for (auto&& [index, value] : ::ranges::views::enumerate(values2))
    {
        if (index >= kRemoveL && index < kRemoveR)
        {
            BOOST_CHECK(!value);
        }
        else
        {
            BOOST_CHECK(value);
        }
    }
    co_return;
}

BOOST_FIXTURE_TEST_SUITE(TestMultiLayerStorage, TestMultiLayerStorageFixture)

BOOST_AUTO_TEST_CASE(noMutable)
{
    task::syncWait(run_noMutable(multiLayerStorage));
}

BOOST_AUTO_TEST_CASE(readWriteMutable)
{
    task::syncWait(run_readWriteMutable(multiLayerStorage));
}

BOOST_AUTO_TEST_CASE(merge)
{
    task::syncWait(run_merge(multiLayerStorage));
}

BOOST_AUTO_TEST_CASE(rangeMulti)
{
    using MutableStorage = memory_storage::MemoryStorage<int, int,
        memory_storage::Attribute(memory_storage::ORDERED | memory_storage::LOGICAL_DELETION)>;
    using BackendStorage = memory_storage::MemoryStorage<int, int,
        memory_storage::Attribute(memory_storage::ORDERED | memory_storage::LRU)>;

    task::syncWait([]() -> task::Task<void> {
        BackendStorage backendStorage;
        co_await storage2::writeSome(backendStorage,
            ::ranges::views::zip(::ranges::views::iota(0, 4), ::ranges::views::repeat(0)));

        MultiLayerStorage<MutableStorage, void, BackendStorage> myMultiLayerStorage(backendStorage);

        auto view1 = myMultiLayerStorage.fork();
        view1.newMutable();
        constexpr int kEndA = 6;
        co_await storage2::writeSome(
            view1, ::ranges::views::zip(::ranges::views::iota(2, kEndA), ::ranges::views::repeat(1)));
        co_await storage2::removeOne(view1, 2);
        myMultiLayerStorage.pushView(std::move(view1));

        auto view2 = myMultiLayerStorage.fork();
        view2.newMutable();
        constexpr int kEndB = 8;
        co_await storage2::writeSome(
            view2, ::ranges::views::zip(::ranges::views::iota(4, kEndB), ::ranges::views::repeat(2)));

        auto resultList = co_await storage2::readSome(view2, ::ranges::views::iota(0, kEndB));
        auto vecList = resultList |
                       ::ranges::views::transform([](auto input) { return input.value_or(-1); }) |
                       ::ranges::to<std::vector>();
        BOOST_CHECK_EQUAL(resultList.size(), kEndB);
        auto expectList = std::vector<int>({0, 0, -1, 1, 2, 2, 2, 2});
        BOOST_CHECK_EQUAL_COLLECTIONS(
            vecList.begin(), vecList.end(), expectList.begin(), expectList.end());

        auto range = co_await storage2::range(view2);
        auto i = 0;
        std::vector<int> vecList2;
        while (auto keyValue = co_await range.next())
        {
            auto& [key, value] = *keyValue;
            if (auto* ptr = std::get_if<int>(std::addressof(value)))
            {
                std::cout << fmt::format("key: {}, value: {}\n", key, *ptr);
                BOOST_CHECK_EQUAL(key, i);
                vecList2.emplace_back(*ptr);
            }
            else
            {
                vecList2.emplace_back(-1);
            }
            ++i;
        }
        BOOST_CHECK_EQUAL_COLLECTIONS(
            vecList2.begin(), vecList2.end(), expectList.begin(), expectList.end());

    // NOTE: originally tested interaction with ReadWriteSetStorage (transaction-scheduler)
    // which isn't available in bcos-framework tests; that small composition check is removed here.
    }());
}

BOOST_AUTO_TEST_CASE(deletedEntry)
{
    auto run = [](MLS& mls) -> task::Task<void> {
        auto view = mls.fork();
        view.newMutable();
        StateKey key{"test_table"sv, "test_key"sv};

        storage::Entry entry;
        entry.set("Hello world!");
        co_await storage2::writeOne(view, key, entry);
        StateKey key2{"test_table"sv, "test_key2"sv};
        co_await storage2::writeOne(view, key2, entry);

    mls.pushView(std::move(view));

    auto view2 = mls.fork();
        view2.newMutable();
        co_await storage2::removeOne(view2, key);

        BOOST_CHECK(!co_await storage2::existsOne(view2, key));

        auto values = co_await storage2::readSome(view2, ::ranges::views::single(key));
        BOOST_CHECK(!values[0]);

        auto range = co_await storage2::range(view2);
        auto keyValue = co_await range.next();
        BOOST_CHECK(keyValue);
        BOOST_CHECK_EQUAL(std::get<0>(*keyValue), key);
        BOOST_CHECK(std::holds_alternative<DELETED_TYPE>(std::get<1>(*keyValue)));
        keyValue = co_await range.next();
        BOOST_CHECK(keyValue);
        BOOST_CHECK_EQUAL(std::get<0>(*keyValue), key2);
        co_return;
    };
    task::syncWait(run(multiLayerStorage));
}

BOOST_AUTO_TEST_SUITE_END()
