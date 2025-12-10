#ifndef DDD_CORE_ENTITY_H
#define DDD_CORE_ENTITY_H

#include "Component.h"
#include <memory>
#include <typeindex>
#include <unordered_map>

class Entity {
  public:
    using Id = std::size_t;

    Entity() : id(nextId++) {}
    Id getId() const { return id; }

    template <typename T, typename... Args> T *addComponent(Args &&...args) {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        auto comp = std::make_unique<T>(std::forward<Args>(args)...);
        T *raw = comp.get();
        components[std::type_index(typeid(T))] = std::move(comp);
        return raw;
    }

    template <typename T> T *get() {
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end())
            return nullptr;
        return static_cast<T *>(it->second.get());
    }

    template <typename T> const T *get() const {
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end())
            return nullptr;
        return static_cast<const T *>(it->second.get());
    }

    template <typename T> bool has() const { return components.count(std::type_index(typeid(T))) > 0; }

    template <typename T> void remove() { components.erase(std::type_index(typeid(T))); }

  private:
    static Id nextId;
    Id id;
    std::unordered_map<std::type_index, std::unique_ptr<Component>> components;
};

inline Entity::Id Entity::nextId = 0;

#endif // DDD_CORE_ENTITY_H

