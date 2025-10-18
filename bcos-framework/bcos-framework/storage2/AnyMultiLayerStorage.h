#pragma once

#include "AnyStorage.h"
#include "Storage.h"
#include <memory>
#include <type_traits>

namespace bcos::storage2
{

// AnyMultiLayerStorage: 针对 MultiLayerStorage 的类型擦除封装。
// AnyMultiLayerStorage: Type-erased wrapper for MultiLayerStorage.
// 目标：擦除具体 MultiLayerStorage 与其 View 的模板参数，使得调用方仅以
// Key/Value 类型进行编程。fork() 返回的 View 自身可作为 Storage 使用，
// 其实现基于 AnyStorage，且内部维持底层 View 的生命周期，确保安全。
// Goal: erase concrete template parameters (MultiLayerStorage and its View) so callers use
// only Key/Value types. The View from fork() is directly a Storage via AnyStorage and
// safely preserves the underlying lifetime.
//
// 典型用法：
// Typical usage:
//   MultiLayerStorage<...> mls(backend, cache);
//   AnyMultiLayerStorage<Key, Value> anyMLS(mls);
//   auto view = anyMLS.fork();                 // view 可直接用于 storage2 API
//   co_await writeOne(view, key, value);       // 读写等均可
//   auto it = co_await range(view);
//
// 注意：该封装专注于 fork() 的只读/读写视图能力，并不暴露 pushView/mergeBackStorage
// 等需要具体类型的接口；若需要这些高级操作，请在已知具体类型处直接使用 MultiLayerStorage。
// Note: Focuses on fork() read/write views only; does not expose pushView/mergeBackStorage
// that require concrete types. Use concrete MultiLayerStorage for those operations.
template <class Key, class Val>
class AnyMultiLayerStorage
{
public:
    using KeyType = Key;
    using ValueType = Val;

    // View: 可作为 Storage 使用的类型擦除视图。内部持有 AnyStorage，并保持底层 View 的生命周期。
    // View: Type-erased view usable as a Storage. Internally holds AnyStorage and preserves
    // the underlying concrete View's lifetime.
    class View
    {
    public:
        View() = default;

        using StorageValue = ValueType;  // internal alias
        // For storage2::ReadOne deduction, expose Value
        using Value = ValueType;

        // 允许隐式转换为 AnyStorage，以便与现有 storage2 API 无缝互操作
        // Allow implicit conversion to AnyStorage for seamless interop with storage2 APIs
        operator AnyStorage<Key, Val>() const { return m_anyStorage; }

        // 为避免悬垂，返回值为复制（浅拷贝），底层对象由 m_self 持有，生命周期由 View 管理
        // To avoid dangling, return by value (shallow copy). The underlying object is owned by
        // m_self and lifetime is managed by View.
        AnyStorage<Key, Val> asAnyStorage() const { return m_anyStorage; }

        // 在视图上开启可变层（仅当底层支持无参 newMutable）
        // Enable a mutable layer on this view (only if underlying supports no-arg newMutable)
        void newMutable()
        {
            if (m_self)
            {
                (void)m_self->newMutable();
            }
        }

        // tag_invoke 转发至内部 AnyStorage，确保 View 本身即可作为 Storage 使用
        friend auto tag_invoke(storage2::tag_t<storage2::readSome> /*unused*/, View& view,
            ::ranges::input_range auto keys) -> task::Task<std::vector<std::optional<Value>>>
        {
            auto any = view.asAnyStorage();
            co_return co_await storage2::readSome(any, std::move(keys));
        }

        friend auto tag_invoke(storage2::tag_t<storage2::writeSome> /*unused*/, View& view,
            ::ranges::input_range auto keyValues) -> task::Task<void>
        {
            auto any = view.asAnyStorage();
            co_await storage2::writeSome(any, std::move(keyValues));
        }

        friend auto tag_invoke(storage2::tag_t<storage2::removeSome> /*unused*/, View& view,
            ::ranges::input_range auto keys) -> task::Task<void>
        {
            auto any = view.asAnyStorage();
            co_await storage2::removeSome(any, std::move(keys));
        }

        friend auto tag_invoke(storage2::tag_t<storage2::removeSome> /*unused*/, View& view,
            ::ranges::input_range auto keys, DIRECT_TYPE /*unused*/) -> task::Task<void>
        {
            auto any = view.asAnyStorage();
            co_await storage2::removeSome(any, std::move(keys), DIRECT);
        }

        friend auto tag_invoke(storage2::tag_t<storage2::readOne> /*unused*/, View& view, auto key)
            -> task::Task<std::optional<Value>>
        {
            auto any = view.asAnyStorage();
            co_return co_await storage2::readOne(any, std::move(key));
        }

        friend auto tag_invoke(storage2::tag_t<storage2::writeOne> /*unused*/, View& view, auto key,
            auto value) -> task::Task<void>
        {
            auto any = view.asAnyStorage();
            co_await storage2::writeOne(any, std::move(key), std::move(value));
        }

        friend auto tag_invoke(storage2::tag_t<storage2::removeOne> /*unused*/, View& view,
            auto key) -> task::Task<void>
        {
            auto any = view.asAnyStorage();
            co_await storage2::removeOne(any, std::move(key));
        }

        friend auto tag_invoke(storage2::tag_t<storage2::removeOne> /*unused*/, View& view,
            auto key, DIRECT_TYPE /*unused*/) -> task::Task<void>
        {
            auto any = view.asAnyStorage();
            co_await storage2::removeOne(any, std::move(key), DIRECT);
        }

        friend auto tag_invoke(storage2::tag_t<storage2::range> /*unused*/, View& view)
            -> task::Task<typename AnyStorage<Key, Val>::Iterator>
        {
            auto any = view.asAnyStorage();
            co_return co_await storage2::range(any);
        }

        friend auto tag_invoke(storage2::tag_t<storage2::range> /*unused*/, View& view,
            RANGE_SEEK_TYPE /*unused*/, const Key& key)
            -> task::Task<typename AnyStorage<Key, Val>::Iterator>
        {
            auto any = view.asAnyStorage();
            co_return co_await storage2::range(any, RANGE_SEEK, key);
        }

    private:
        struct ViewConcept
        {
            ViewConcept() = default;
            ViewConcept(const ViewConcept&) = delete;
            ViewConcept& operator=(const ViewConcept&) = delete;
            ViewConcept(ViewConcept&&) = delete;
            ViewConcept& operator=(ViewConcept&&) = delete;
            virtual ~ViewConcept() = default;
            virtual AnyStorage<Key, Val> makeAny() = 0;
            // Enable creating a mutable layer if underlying view supports no-arg newMutable()
            // 如果底层视图支持无参 newMutable()，则启用可变层
            virtual bool newMutable() = 0;
        };

        template <class ConcreteView>
        struct ViewModel : ViewConcept
        {
            explicit ViewModel(ConcreteView&& viewObj)
              : m_view(std::make_shared<ConcreteView>(std::move(viewObj)))
            {}

            AnyStorage<Key, Val> makeAny() override { return AnyStorage<Key, Val>(*m_view); }

            bool newMutable() override
            {
                if constexpr (requires(
                                  ConcreteView& concreteViewRef) { concreteViewRef.newMutable(); })
                {
                    m_view->newMutable();
                    return true;
                }
                else
                {
                    return false;
                }
            }

            std::shared_ptr<ConcreteView> m_view;
        };

        template <class ConcreteView>
        static View makeFrom(ConcreteView&& concreteView)
        {
            using CV = std::remove_cvref_t<ConcreteView>;
            auto model = std::make_shared<ViewModel<CV>>(std::forward<ConcreteView>(concreteView));
            View out;
            out.m_self = std::move(model);
            out.m_anyStorage = out.m_self->makeAny();
            return out;
        }

        std::shared_ptr<ViewConcept> m_self;
        AnyStorage<Key, Val> m_anyStorage;

        template <class UKey, class UValue>
        friend class AnyMultiLayerStorage;
    };

    AnyMultiLayerStorage() = default;

    template <class MultiLayer>
        requires(!std::is_same_v<std::remove_cvref_t<MultiLayer>, AnyMultiLayerStorage> &&
                 std::same_as<typename std::remove_cvref_t<MultiLayer>::Key, Key> &&
                 std::same_as<typename std::remove_cvref_t<MultiLayer>::Value, Val>)
    explicit AnyMultiLayerStorage(MultiLayer& mls)
      : m_self(std::make_shared<Model<MultiLayer>>(mls))
    {}

    // 生成可作为 Storage 使用的视图
    View fork() { return m_self->fork(); }


    // 直接获取 AnyStorage 视图，便于调用方无需再从 View 提取
    AnyStorage<Key, Val> forkAny() { return m_self->forkAny(); }

private:
    struct Concept
    {
        Concept() = default;
        Concept(const Concept&) = delete;
        Concept& operator=(const Concept&) = delete;
        Concept(Concept&&) = delete;
        Concept& operator=(Concept&&) = delete;
        virtual ~Concept() = default;
        virtual View fork() = 0;
        virtual AnyStorage<Key, Val> forkAny() = 0;
    };

    template <class MultiLayer>
    struct Model : Concept
    {
        explicit Model(MultiLayer& mls) : m_mls(std::addressof(mls))
        {
            // 静态检查 Key/Value 一致性
            // Static check for Key/Value consistency
            static_assert(std::same_as<typename MultiLayer::Key, Key>);
            static_assert(std::same_as<typename MultiLayer::Value, Val>);
        }

        View fork() override
        {
            auto view = m_mls->fork();
            return View::makeFrom(std::move(view));
        }

        AnyStorage<Key, Val> forkAny() override
        {
            auto view = m_mls->fork();
            return AnyStorage<Key, Val>(view);
        }

        MultiLayer* m_mls;
    };

    std::shared_ptr<Concept> m_self;
};

}  // namespace bcos::storage2
