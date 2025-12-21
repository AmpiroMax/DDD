#include "systems/TileInteractionSystem.h"

#include <cmath>
#include <optional>

#include "components/Tags.h"
#include "events/InventoryEvents.h"
#include "systems/InventorySystem.h"

TileInteractionSystem::TileInteractionSystem(InputSystem &inputSys, EntityManager &entityMgr, EventBus &eventBus,
                                             InventorySystem *inventorySys, GameplayCommandBuffer &cmdBuffer)
    : inputSystem(inputSys), entityManager(entityMgr), eventBus(eventBus), inventorySystem(inventorySys),
      commandBuffer(cmdBuffer) {}

bool TileInteractionSystem::pickTile(const TilemapComponent &map, const Vec2 &worldPos, int &outX, int &outY) const {
    const float localX = (worldPos.x - map.origin.x) / map.tileSize;
    const float localY = (map.origin.y - worldPos.y) / map.tileSize; // data y grows downward
    outX = static_cast<int>(std::floor(localX));
    outY = static_cast<int>(std::floor(localY));
    return map.inBounds(outX, outY);
}

void TileInteractionSystem::update(float dt) {
    (void)dt;
    InputComponent *input = inputSystem.getInput();
    if (!input)
        return;

    const auto getAction = [&](const std::string &name) -> const ButtonState * {
        auto it = input->actions.find(name);
        return it != input->actions.end() ? &it->second : nullptr;
    };
    const ButtonState *breakAction = getAction("break_block");
    const ButtonState *placeAction = getAction("place_block");

    const bool wantBreak = breakAction && (breakAction->pressed || breakAction->held);
    const bool wantPlace = placeAction && (placeAction->pressed || placeAction->held);

    if (!wantBreak && !wantPlace)
        return;

    TilemapComponent *tilemap = nullptr;
    Entity *player = nullptr;
    for (auto &entPtr : entityManager.all()) {
        if (!tilemap)
            tilemap = entPtr->get<TilemapComponent>();
        if (!player && entPtr->has<PlayerTag>())
            player = entPtr.get();
        if (tilemap && player)
            break;
    }
    if (!tilemap || !player)
        return;

    int tx = 0, ty = 0;
    if (!pickTile(*tilemap, input->mouseWorld, tx, ty))
        return;

    const int current = tilemap->get(tx, ty);

    if (wantBreak && current != tilemap->emptyId) {
        tilemap->tiles[tilemap->index(tx, ty)] = tilemap->emptyId;
        eventBus.emit(BreakBlockEvent{tx, ty, current});
    } else if (wantPlace && current == tilemap->emptyId) {
        int chosenTileId = placeTileFallback;
        std::optional<InventorySystem::ActiveItemInfo> activeItem;
        if (inventorySystem) {
            activeItem = inventorySystem->getActiveItem(player->getId());
            if (!activeItem || activeItem->placeTileId < 0 || activeItem->count <= 0) {
                return; // no placeable item selected
            }
            chosenTileId = activeItem->placeTileId;
        }

        tilemap->tiles[tilemap->index(tx, ty)] = chosenTileId;
        eventBus.emit(PlaceBlockEvent{tx, ty, chosenTileId});

        if (inventorySystem && activeItem) {
            // Defer consumption to gameplay command buffer to keep mutations in one phase.
            commandBuffer.push(ConsumeItemCommand{player->getId(), activeItem->slotIndex, 1, activeItem->itemId,
                                                  chosenTileId, tx, ty, true});
        }
    }
}

