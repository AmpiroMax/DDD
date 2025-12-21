#ifndef DDD_SYSTEMS_TILE_INTERACTION_SYSTEM_H
#define DDD_SYSTEMS_TILE_INTERACTION_SYSTEM_H

#include "components/InputComponent.h"
#include "components/TilemapComponent.h"
#include "core/EntityManager.h"
#include "core/EventBus.h"
#include "core/System.h"
#include "events/TileEvents.h"
#include "game/GameplayCommandBuffer.h"
#include "systems/InputSystem.h"

class InventorySystem;

class TileInteractionSystem : public System {
  public:
    TileInteractionSystem(InputSystem &inputSys, EntityManager &entityMgr, EventBus &eventBus,
                          InventorySystem *inventorySys, GameplayCommandBuffer &cmdBuffer);
    void update(float dt) override;
    void setInventorySystem(InventorySystem *inventorySys) { inventorySystem = inventorySys; }

  private:
    bool pickTile(const TilemapComponent &map, const Vec2 &worldPos, int &outX, int &outY) const;

    InputSystem &inputSystem;
    EntityManager &entityManager;
    EventBus &eventBus;
    InventorySystem *inventorySystem{nullptr};
    GameplayCommandBuffer &commandBuffer;
    int placeTileFallback{1}; // default block id when inventory is missing
};

#endif // DDD_SYSTEMS_TILE_INTERACTION_SYSTEM_H

