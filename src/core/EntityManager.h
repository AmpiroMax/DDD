#ifndef DDD_CORE_ENTITY_MANAGER_H
#define DDD_CORE_ENTITY_MANAGER_H

#include "Entity.h"
#include <memory>
#include <unordered_map>
#include <vector>

class EntityManager {
  public:
    Entity &create() {
        auto ent = std::make_unique<Entity>();
        Entity &ref = *ent;
        entities.push_back(std::move(ent));
        return ref;
    }

    void remove(Entity::Id id) {
        auto it = std::remove_if(entities.begin(), entities.end(), [id](const auto &ptr) { return ptr->getId() == id; });
        entities.erase(it, entities.end());
    }

    const std::vector<std::unique_ptr<Entity>> &all() const { return entities; }
    std::vector<std::unique_ptr<Entity>> &all() { return entities; }

    Entity *find(Entity::Id id) {
        for (auto &e : entities) {
            if (e->getId() == id)
                return e.get();
        }
        return nullptr;
    }

    void clear() { entities.clear(); }

  private:
    std::vector<std::unique_ptr<Entity>> entities;
};

#endif // DDD_CORE_ENTITY_MANAGER_H

