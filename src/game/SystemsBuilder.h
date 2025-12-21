#ifndef DDD_GAME_SYSTEMS_BUILDER_H
#define DDD_GAME_SYSTEMS_BUILDER_H

#include "core/System.h"
#include "game/GameplayCommandBuffer.h"
#include <memory>
#include <vector>

class CameraManager;
class DebugManager;
class EntityManager;
class EventBus;
class MenuController;
class PhysicsManager;
class ResourceManager;
class WindowManager;
struct GameConfig;

class InputSystem;
class InventorySystem;
class SpellSystem;
class PhysicsSystem;
class SpawnSystem;
class MatchSystem;
class RespawnSystem;
class UIRenderSystem;

struct BuiltSystems {
    std::vector<std::unique_ptr<System>> updateSystems;
    std::vector<std::unique_ptr<System>> renderSystems;
    std::unique_ptr<PhysicsSystem> physicsSystem;

    InputSystem *inputSystem{nullptr};
    InventorySystem *inventorySystem{nullptr};
    SpellSystem *spellSystem{nullptr};
    UIRenderSystem *uiRenderSystem{nullptr};
    SpawnSystem *spawnSystem{nullptr};
    MatchSystem *matchSystem{nullptr};
    RespawnSystem *respawnSystem{nullptr};
};

class SystemsBuilder {
  public:
    static BuiltSystems build(const GameConfig &config, WindowManager &windowManager, CameraManager &cameraManager,
                              PhysicsManager &physicsManager, ResourceManager &resourceManager, EntityManager &entityManager,
                              EventBus &eventBus, DebugManager &debugManager, MenuController *menuController,
                              GameplayCommandBuffer &commandBuffer);
};

#endif // DDD_GAME_SYSTEMS_BUILDER_H


