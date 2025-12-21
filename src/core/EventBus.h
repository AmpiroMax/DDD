#ifndef DDD_CORE_EVENT_BUS_H
#define DDD_CORE_EVENT_BUS_H

#include <functional>
#include <memory>
#include <queue>
#include <typeindex>
#include <unordered_map>
#include <vector>

class EventBus {
  public:
    struct SubscriptionToken {
        std::type_index eventType{typeid(void)};
        std::size_t id{0};

        bool isValid() const { return eventType != std::type_index(typeid(void)) && id != 0; }
    };

    template <typename E> void emit(const E &e) { queue_.push(std::make_unique<Wrapper<E>>(e)); }

    // Backwards-compatible subscribe API (no unsubscribe).
    template <typename E> void subscribe(std::function<void(const E &)> handler) {
        (void)subscribeWithToken<E>(std::move(handler));
    }

    // Preferred subscribe API: returns token that can be used with unsubscribe().
    template <typename E> SubscriptionToken subscribeWithToken(std::function<void(const E &)> handler) {
        const std::type_index type = typeid(E);
        const std::size_t id = ++nextHandlerId_;
        HandlerEntry entry;
        entry.id = id;
        entry.active = true;
        entry.fn = [handler = std::move(handler)](const void *ev) { handler(*static_cast<const E *>(ev)); };
        handlers_[type].push_back(std::move(entry));
        return SubscriptionToken{type, id};
    }

    void unsubscribe(const SubscriptionToken &token) {
        if (!token.isValid())
            return;
        auto it = handlers_.find(token.eventType);
        if (it == handlers_.end())
            return;
        for (auto &entry : it->second) {
            if (entry.id == token.id) {
                entry.active = false; // lazy removal
                entry.fn = nullptr;
                break;
            }
        }
    }

    void pump() {
        const std::size_t n = queue_.size();
        for (std::size_t i = 0; i < n; ++i) {
            auto &ev = queue_.front();
            auto it = handlers_.find(ev->type());
            if (it != handlers_.end()) {
                for (auto &entry : it->second) {
                    if (entry.active && entry.fn)
                        entry.fn(ev->ptr());
                }
            }
            queue_.pop();
        }
    }

    void clear() {
        while (!queue_.empty())
            queue_.pop();
    }

  private:
    struct HandlerEntry {
        std::size_t id{0};
        bool active{false};
        std::function<void(const void *)> fn;
    };

    struct Base {
        virtual ~Base() = default;
        virtual std::type_index type() const = 0;
        virtual const void *ptr() const = 0;
    };

    template <typename E> struct Wrapper : Base {
        explicit Wrapper(const E &e) : ev(e) {}
        std::type_index type() const override { return typeid(E); }
        const void *ptr() const override { return &ev; }
        E ev;
    };

    std::unordered_map<std::type_index, std::vector<HandlerEntry>> handlers_;
    std::queue<std::unique_ptr<Base>> queue_;
    std::size_t nextHandlerId_{0};
};

#endif // DDD_CORE_EVENT_BUS_H

