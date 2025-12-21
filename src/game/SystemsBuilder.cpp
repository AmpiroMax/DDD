#include "game/SystemsBuilder.h"

#include "game/GameConfig.h"
#include "game/MenuController.h"

#include "systems/CameraFollowSystem.h"
#include "systems/DebugSystem.h"
#include "systems/DropPickupSystem.h"
#include "systems/InputSystem.h"
#include "systems/InventorySystem.h"
#include "systems/MatchSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/PlayerControlSystem.h"
#include "systems/RenderSystem.h"
#include "systems/RespawnSystem.h"
#include "systems/SpawnSystem.h"
#include "systems/SpellSystem.h"
#include "systems/TileInteractionSystem.h"
#include "systems/TopDownControlSystem.h"
#include "systems/UIRenderSystem.h"

BuiltSystems SystemsBuilder::build(const GameConfig &config, WindowManager &windowManager, CameraManager &cameraManager,
                                  PhysicsManager &physicsManager, ResourceManager &resourceManager, EntityManager &entityManager,
                                  EventBus &eventBus, DebugManager &debugManager, MenuController *menuController,
                                  GameplayCommandBuffer &commandBuffer) {
    BuiltSystems out;

    auto inputPtr = std::make_unique<InputSystem>(windowManager, cameraManager, entityManager);
    inputPtr->loadBindingsFromFile("config/input.json");
    inputPtr->setIsoConfig(config.iso);
    out.inputSystem = inputPtr.get();

    auto inventoryPtr = std::make_unique<InventorySystem>(*out.inputSystem, entityManager, eventBus);
    const std::filesystem::path inventoryPath = std::filesystem::path("config") / config.inventoryFile;
    inventoryPtr->loadConfigFromFile(inventoryPath.string());
    out.inventorySystem = inventoryPtr.get();

    auto spellPtr = std::make_unique<SpellSystem>(*out.inputSystem, entityManager, eventBus);
    const std::filesystem::path spellsPath = std::filesystem::path("config") / config.spellsFile;
    spellPtr->loadConfigFromFile(spellsPath.string());
    out.spellSystem = spellPtr.get();

    // Configure gravity per mode before stepping physics.
    if (config.mode == "diablo") {
        physicsManager.setGravity(b2Vec2(0.0f, 0.0f));
    } else {
        physicsManager.setGravity(b2Vec2(0.0f, -9.8f));
    }
    out.physicsSystem = std::make_unique<PhysicsSystem>(physicsManager, entityManager, eventBus,
                                                        /*enableTilemapColliders=*/config.mode != "diablo",
                                                        /*enablePlayerPlayerCollision=*/config.playerPlayerCollision);

    out.updateSystems.push_back(std::move(inputPtr));
    out.updateSystems.push_back(std::move(inventoryPtr));
    out.updateSystems.push_back(std::move(spellPtr));

    if (config.mode != "diablo") {
        out.updateSystems.push_back(
            std::make_unique<DropPickupSystem>(entityManager, physicsManager, commandBuffer, config.pickupRadius));
        out.updateSystems.push_back(std::make_unique<PlayerControlSystem>(*out.inputSystem, entityManager, eventBus, config.playerSpeed,
                                                                          config.playerJump));
        out.updateSystems.push_back(
            std::make_unique<TileInteractionSystem>(*out.inputSystem, entityManager, eventBus, out.inventorySystem, commandBuffer));
    } else {
        TopDownControlSystem::Params p{};
        p.maxSpeed = config.playerSpeed;
        p.accel = config.playerAccel;
        p.decel = config.playerDecel;
        p.linearDamping = config.playerLinearDamping;
        out.updateSystems.push_back(std::make_unique<TopDownControlSystem>(*out.inputSystem, entityManager, p));

        // PvP scaffolding for diablo mode.
        auto spawnPtr = std::make_unique<SpawnSystem>(entityManager, eventBus, physicsManager, resourceManager);
        out.spawnSystem = spawnPtr.get();
        out.updateSystems.push_back(std::move(spawnPtr));

        auto matchPtr = std::make_unique<MatchSystem>(eventBus, debugManager, config.matchDurationSec);
        out.matchSystem = matchPtr.get();
        out.updateSystems.push_back(std::move(matchPtr));

        auto respawnPtr = std::make_unique<RespawnSystem>(entityManager, eventBus, commandBuffer);
        out.respawnSystem = respawnPtr.get();
        out.updateSystems.push_back(std::move(respawnPtr));
    }

    out.updateSystems.push_back(std::make_unique<CameraFollowSystem>(cameraManager, entityManager));
    out.updateSystems.push_back(std::make_unique<DebugSystem>(entityManager, debugManager, *out.inputSystem));

    out.renderSystems.push_back(
        std::make_unique<RenderSystem>(windowManager, cameraManager, resourceManager, entityManager, &debugManager, config.iso));
    auto uiPtr = std::make_unique<UIRenderSystem>(windowManager, resourceManager, debugManager, eventBus);
    out.uiRenderSystem = uiPtr.get();
    if (menuController)
        out.uiRenderSystem->setMenuState(menuController->getMenuStatePtr());
    out.renderSystems.push_back(std::move(uiPtr));

    return out;
}


