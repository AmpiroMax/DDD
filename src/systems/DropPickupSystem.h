#ifndef DDD_SYSTEMS_DROP_PICKUP_SYSTEM_H
#define DDD_SYSTEMS_DROP_PICKUP_SYSTEM_H

#include "components/DropComponent.h"
#include "components/PhysicsBodyComponent.h"
#include "components/Tags.h"
#include "components/TransformComponent.h"
#include "core/EntityManager.h"
#include "core/System.h"
#include "managers/PhysicsManager.h"
#include "systems/InventorySystem.h"
#include "utils/Vec2.h"
#include <vector>

class DropPickupSystem : public System {
  public:
    DropPickupSystem(EntityManager &entityMgr, InventorySystem &inventorySys, PhysicsManager &physicsMgr, float radius)
        : entityManager(entityMgr), inventorySystem(inventorySys), physicsManager(physicsMgr), pickupRadius(radius) {}

    void update(float dt) override;

  private:
    Entity *findPlayer() const;
    void destroyBodyIfAny(Entity &ent);

    EntityManager &entityManager;
    InventorySystem &inventorySystem;
    PhysicsManager &physicsManager;
    float pickupRadius{1.5f};
};

#endif // DDD_SYSTEMS_DROP_PICKUP_SYSTEM_H

