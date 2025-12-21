#ifndef DDD_GAME_GAME_CONFIG_H
#define DDD_GAME_GAME_CONFIG_H

#include "utils/IsoConfig.h"
#include <string>

struct GameConfig {
    std::string mode{"terraria"}; // "terraria" (legacy) or "diablo"
    int windowWidth{1280};
    int windowHeight{720};
    std::string windowTitle{"DDD Terraria-like MVP"};
    std::string resourcesPath{"resources"};
    std::string texturesPath{"textures"};
    std::string fontsPath{"fonts"};
    std::string mapFile{"maps/level01.json"};
    std::string inventoryFile{"inventory.json"};
    std::string spellsFile{"spells.json"};
    std::string spawnsFile{"spawns.json"};
    float matchDurationSec{180.0f};
    struct {
        bool has{false};
        float minX{-10.0f};
        float maxX{40.0f};
        float minY{-10.0f};
        float maxY{30.0f};
    } arena;
    float tileSize{1.0f};
    float playerSpeed{6.0f};         // max speed (top-down) or horizontal speed (legacy)
    float playerJump{8.0f};          // legacy
    float playerRadius{0.45f};       // top-down circle radius (world units)
    float playerAccel{30.0f};        // top-down accel (wu/s^2)
    float playerDecel{40.0f};        // top-down decel (wu/s^2)
    float playerLinearDamping{6.0f}; // top-down damping
    float pickupRadius{1.5f};
    int maxPlayers{8};
    bool playerPlayerCollision{true}; // true = soft push (Box2D), false = no collision
    IsoConfig iso{};
};

#endif // DDD_GAME_GAME_CONFIG_H


