#ifndef DDD_GAME_WORLD_LOADER_H
#define DDD_GAME_WORLD_LOADER_H

#include "game/GameConfig.h"
#include <filesystem>

class CameraManager;
class EntityManager;
class ResourceManager;
class InventorySystem;
class SpellSystem;
class SpawnSystem;

class WorldLoader {
  public:
    // Builds tilemap + player entity (and optional PvP scaffolding) into the given EntityManager.
    // Behavior is intentionally kept identical to the previous GameApp::loadMapAndEntities().
    static void loadMapAndEntities(const std::filesystem::path &mapPath, const GameConfig &config, EntityManager &entityManager,
                                   ResourceManager &resourceManager, CameraManager &cameraManager,
                                   InventorySystem *inventorySystem, SpellSystem *spellSystem, SpawnSystem *spawnSystem);
};

#endif // DDD_GAME_WORLD_LOADER_H


