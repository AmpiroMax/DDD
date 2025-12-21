#ifndef DDD_GAME_SAVE_DATA_H
#define DDD_GAME_SAVE_DATA_H

#include "components/InventoryComponent.h"
#include <filesystem>
#include <vector>

struct SaveData {
    std::filesystem::path baseMap;

    struct TileChange {
        int x{0};
        int y{0};
        int tileId{-1};
    };
    std::vector<TileChange> placed;
    std::vector<TileChange> removed;

    struct DropInfo {
        int itemId{-1};
        int count{0};
        float px{0};
        float py{0};
        float vx{0};
        float vy{0};
    };
    std::vector<DropInfo> drops;

    struct PlayerInfo {
        float px{0};
        float py{0};
        float vx{0};
        float vy{0};
        int hp{100};
        int maxHp{100};
        int activeSlot{0};
        std::vector<ItemSlot> slots;
    } player;
};

#endif // DDD_GAME_SAVE_DATA_H


