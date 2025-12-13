#ifndef DDD_EVENTS_INVENTORY_EVENTS_H
#define DDD_EVENTS_INVENTORY_EVENTS_H

#include "core/Entity.h"
#include <string>
#include <unordered_map>
#include <vector>

struct InventoryAddItemEvent {
    Entity::Id entityId{0};
    int itemId{0};
    int amount{0};
};

struct InventoryDropAddedEvent {
    Entity::Id entityId{0};
    int itemId{0};
    int added{0};     // how many items were stored
    int leftover{0};  // how many could not be stored
};

struct InventoryActiveSlotChangedEvent {
    Entity::Id entityId{0};
    int previous{-1};
    int current{-1};
    int itemId{-1};
    int count{0};
};

struct InventoryUseItemEvent {
    Entity::Id entityId{0};
    int slotIndex{-1};
    int itemId{-1};
    int placeTileId{-1};
    int tileX{0};
    int tileY{0};
    bool hasTile{false};
};

struct InventorySlotState {
    int itemId{-1};
    int count{0};
};

struct InventoryIconRef {
    std::string texture;
    std::string region;
};

struct InventoryStateChangedEvent {
    Entity::Id entityId{0};
    int activeIndex{0};
    std::vector<InventorySlotState> slots;
    std::unordered_map<int, InventoryIconRef> itemMeta;
};

#endif // DDD_EVENTS_INVENTORY_EVENTS_H

