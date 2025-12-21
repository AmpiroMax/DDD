#ifndef DDD_EVENTS_GAMEPLAY_COMMANDS_H
#define DDD_EVENTS_GAMEPLAY_COMMANDS_H

#include "core/Entity.h"

// Deferred commands applied in a defined gameplay phase.
struct RespawnCommand {
    Entity::Id entityId{0};
};

struct PickupCommand {
    Entity::Id playerId{0};
    Entity::Id dropEntityId{0};
    int itemId{-1};
    int count{0};
};

struct ConsumeItemCommand {
    Entity::Id entityId{0};
    int slotIndex{-1};
    int amount{0};
    // Optional context for use-event
    int itemId{-1};
    int placeTileId{-1};
    int tileX{0};
    int tileY{0};
    bool hasTile{false};
};

#endif // DDD_EVENTS_GAMEPLAY_COMMANDS_H


