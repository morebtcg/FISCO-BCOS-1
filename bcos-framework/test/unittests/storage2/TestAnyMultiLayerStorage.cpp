#include "bcos-framework/storage2/AnyMultiLayerStorage.h"
#include "bcos-framework/storage2/MemoryStorage.h"
#include "bcos-framework/storage2/MultiLayerStorage.h"
#include "bcos-task/Wait.h"
#include <boost/test/unit_test.hpp>
#include <string>

using namespace bcos;
using namespace bcos::storage2;
using namespace bcos::storage2::memory_storage;

// 这里构造一个典型的多层结构：
// - 可变层：MemoryStorage<Key, Value, Attribute(ORDERED | LOGICAL_DELETION)>
// - 后端层：MemoryStorage<Key, Value, ORDERED>
// - 缓存层：可选，这里演示无缓存和有缓存两种
// This sets up a typical multi-layer structure:
// - Mutable layer: MemoryStorage<Key, Value, Attribute(ORDERED | LOGICAL_DELETION)>
// - Backend layer: MemoryStorage<Key, Value, ORDERED>
// - Cache layer: optional, demonstrate both without cache and with cache

BOOST_AUTO_TEST_SUITE(TestAnyMultiLayerStorage)

BOOST_AUTO_TEST_CASE(fork_view_read_write)
{
    task::syncWait([]() -> task::Task<void> {
        // backend ordered storage (no logical deletion)
        MemoryStorage<int, std::string, ORDERED> backend;
        // cache ordered storage (optional)
        MemoryStorage<int, std::string, ORDERED> cache;

    // MultiLayerStorage 需要带逻辑删除的可变层
    // MultiLayerStorage requires a mutable layer with logical deletion
        using Mutable = MemoryStorage<int, std::string, Attribute(ORDERED | LOGICAL_DELETION)>;
    // 没有 cache 的情形
    // Case without cache
        {
            MultiLayerStorage<Mutable, void, decltype(backend)> mlsNoCache(backend);
            AnyMultiLayerStorage<int, std::string> anyMLS(mlsNoCache);
            auto view = anyMLS.fork();
            view.newMutable();

            // 初始：backend 空，写入 view（可变层）
            // Initially backend is empty; write into the view (mutable layer)
            co_await writeOne(view, 1, std::string{"a"});
            co_await writeOne(view, 2, std::string{"b"});

            // 读回
            // Read back
            auto valueFirst = co_await readOne(view, 1);
            auto valueSecond = co_await readOne(view, 2);
            BOOST_REQUIRE(valueFirst);
            BOOST_REQUIRE(valueSecond);
            BOOST_CHECK_EQUAL(*valueFirst, "a");
            BOOST_CHECK_EQUAL(*valueSecond, "b");

            // range 统计
            // Iterate range and count present values
            auto it = co_await range(view);
            int count = 0;
            while (auto item = co_await it.next())
            {
                auto& [k, storageValue] = *item;
                if (std::holds_alternative<storage2::NOT_EXISTS_TYPE>(storageValue) ||
                    std::holds_alternative<storage2::DELETED_TYPE>(storageValue))
                {
                    continue;
                }
                (void)k;
                ++count;
            }
            BOOST_CHECK_EQUAL(count, 2);
        }

    // 带 cache 的情形
    // Case with cache
        {
            MultiLayerStorage<Mutable, decltype(cache), decltype(backend)> mls(backend, cache);
            AnyMultiLayerStorage<int, std::string> anyMLS(mls);

            // fork 只读视图
            // Fork a read-only view (no mutable layer)
            auto viewR = anyMLS.fork();
            // 新建一个有可变层的视图
            // Create another view with a mutable layer
            auto viewW = anyMLS.fork();
            viewW.newMutable();

            constexpr int kKey10 = 10;
            constexpr int kKey12 = 12;
            co_await writeSome(
                viewW, std::vector<std::pair<int, std::string>>{{kKey10, "x10"}, {kKey12, "x12"}});

            // 读（先从可变层命中，否则到 cache/backend）
            // Read (hit mutable layer first, otherwise fallback to cache/backend)
            constexpr int kKey11 = 11;
            auto readValues = co_await readSome(viewW, std::vector<int>{kKey10, kKey11, kKey12});
            BOOST_CHECK(readValues[0] && *readValues[0] == "x10");
            BOOST_CHECK(!readValues[1]);
            BOOST_CHECK(readValues[2] && *readValues[2] == "x12");

            // viewR 只读视图（没有可变层），也能看到未合并数据吗？不会，viewR 不带可变层
            // Can read-only view see unmerged data? No, it has no mutable layer.
            auto readValuesR = co_await readSome(viewR, std::vector<int>{kKey10, kKey12});
            BOOST_CHECK(!readValuesR[0]);
            BOOST_CHECK(!readValuesR[1]);
        }
    }());
}

BOOST_AUTO_TEST_CASE(forkAny_direct_usage)
{
    task::syncWait([]() -> task::Task<void> {
        MemoryStorage<int, std::string, ORDERED> backend;
        using Mutable = MemoryStorage<int, std::string, Attribute(ORDERED | LOGICAL_DELETION)>;
        MultiLayerStorage<Mutable, void, decltype(backend)> mls(backend);
        AnyMultiLayerStorage<int, std::string> anyMLS(mls);

        auto view = anyMLS.fork();
        view.newMutable();

    // 使用 View 进行读写（隐式转 AnyStorage 可用）
    // Use View for read/write (implicitly convertible to AnyStorage)
        co_await writeSome(view, std::vector<std::pair<int, std::string>>{{1, "a"}, {2, "b"}});
        auto readBack = co_await readSome(view, std::vector<int>{1, 2, 3});
        BOOST_CHECK(readBack[0] && *readBack[0] == "a");
        BOOST_CHECK(readBack[1] && *readBack[1] == "b");
        BOOST_CHECK(!readBack[2]);
    }());
}

BOOST_AUTO_TEST_SUITE_END()
