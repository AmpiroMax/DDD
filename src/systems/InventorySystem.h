#ifndef DDD_SYSTEMS_INVENTORY_SYSTEM_H
#define DDD_SYSTEMS_INVENTORY_SYSTEM_H

#include "components/InventoryComponent.h"
#include "core/EntityManager.h"
#include "core/EventBus.h"
#include "core/System.h"
#include "events/InventoryEvents.h"
#include "systems/InputSystem.h"
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class InventorySystem : public System {
  public:
    struct ItemDefinition {
        int id{0};
        int maxStack{99};
        int placeTileId{-1}; // tile id to place when used; -1 if not placeable
        std::string iconRegion;
        std::string iconTexture;
    };

    InventorySystem(InputSystem &inputSys, EntityManager &entityMgr, EventBus &eventBus);
    ~InventorySystem() override { shutdown(); }

    void loadConfigFromFile(const std::string &path);
    void attachToEntity(Entity &entity);

    void update(float dt) override;
    void shutdown() override;

    int addItem(Entity::Id entityId, int itemId, int amount);
    bool consumeFromSlot(Entity::Id entityId, int slotIndex, int amount);
    bool consumeActive(Entity::Id entityId, int amount);
    bool setActiveSlot(Entity::Id entityId, int slotIndex);
    bool cycleActiveSlot(Entity::Id entityId, int delta);

    struct ActiveItemInfo {
        int slotIndex{-1};
        int itemId{-1};
        int count{0};
        int placeTileId{-1};
    };

    std::optional<ActiveItemInfo> getActiveItem(Entity::Id entityId) const;

    const std::unordered_map<int, ItemDefinition> &getDefinitions() const { return itemDefs; }
    int getSlotCount() const { return slotCount; }
    int getHotbarSize() const { return hotbarSize; }

  private:
    InputSystem &inputSystem;
    EntityManager &entityManager;
    EventBus &eventBus;
    EventBus::SubscriptionToken addItemSub_{};

    int slotCount{5};
    int hotbarSize{5};
    std::unordered_map<int, ItemDefinition> itemDefs;
    std::vector<ItemSlot> initialSlots;

    InventoryComponent *findInventory(Entity::Id entityId) const;
    Entity *findOwnerEntity() const;
    const ItemDefinition *findDefinition(int itemId) const;
    ItemDefinition &ensureDefinition(int itemId);

    void handleInput();
    void clampActive(InventoryComponent &inv);
    void emitActiveChanged(Entity::Id entityId, const InventoryComponent &inv, int previousSlot);
    void emitStateChanged(Entity::Id entityId, const InventoryComponent &inv);
};

#endif // DDD_SYSTEMS_INVENTORY_SYSTEM_H

