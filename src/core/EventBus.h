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
    template <typename E> void emit(const E &e) { queue_.push(std::make_unique<Wrapper<E>>(e)); }

    template <typename E> void subscribe(std::function<void(const E &)> handler) {
        handlers_[typeid(E)].push_back([handler](const void *ev) { handler(*static_cast<const E *>(ev)); });
    }

    void pump() {
        const std::size_t n = queue_.size();
        for (std::size_t i = 0; i < n; ++i) {
            auto &ev = queue_.front();
            auto it = handlers_.find(ev->type());
            if (it != handlers_.end()) {
                for (auto &fn : it->second)
                    fn(ev->ptr());
            }
            queue_.pop();
        }
    }

    void clear() {
        while (!queue_.empty())
            queue_.pop();
    }

  private:
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

    std::unordered_map<std::type_index, std::vector<std::function<void(const void *)>>> handlers_;
    std::queue<std::unique_ptr<Base>> queue_;
};

#endif // DDD_CORE_EVENT_BUS_H

