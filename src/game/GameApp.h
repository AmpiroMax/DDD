#ifndef DDD_GAME_GAME_APP_H
#define DDD_GAME_GAME_APP_H

#include "game/GameConfig.h"
#include "core/EntityManager.h"
#include "core/EventBus.h"
#include "core/System.h"
#include "managers/CameraManager.h"
#include "managers/DebugManager.h"
#include "managers/PhysicsManager.h"
#include "managers/ResourceManager.h"
#include "managers/TimeManager.h"
#include "managers/WindowManager.h"
#include "game/GameplayCommandBuffer.h"
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

class InputSystem;
class InventorySystem;
class SpellSystem;
class PhysicsSystem;
class SpawnSystem;
class MatchSystem;
class RespawnSystem;
class UIRenderSystem;
class MenuController;

class GameApp {
  public:
    GameApp();
    ~GameApp();
    void run();

  private:
    void initSystems();
    void loadConfig();
    void loadResources();
    void loadMapAndEntities(const std::filesystem::path &mapPath);

    GameConfig config;
    WindowManager windowManager;
    CameraManager cameraManager;
    PhysicsManager physicsManager;
    ResourceManager resourceManager;
    TimeManager timeManager;
    DebugManager debugManager;
    EntityManager entityManager;
    EventBus eventBus;

    InputSystem *inputSystem{nullptr}; // owned by updateSystems
    InventorySystem *inventorySystem{nullptr}; // owned by updateSystems
    SpellSystem *spellSystem{nullptr};       // owned by updateSystems
    UIRenderSystem *uiRenderSystem{nullptr};   // owned by renderSystems
    std::unique_ptr<PhysicsSystem> physicsSystem;
    SpawnSystem *spawnSystem{nullptr};   // owned by updateSystems
    MatchSystem *matchSystem{nullptr};   // owned by updateSystems
    RespawnSystem *respawnSystem{nullptr}; // owned by updateSystems

    std::vector<std::unique_ptr<System>> updateSystems; // logic (input/player/camera/tile/debug)
    std::vector<std::unique_ptr<System>> renderSystems; // render & UI

    bool hasSaveFile{false};
    std::filesystem::path currentMapPath;
    GameplayCommandBuffer gameplayCommands;

    MenuController *menuController{nullptr}; // owned by this (see ctor)
    std::vector<EventBus::SubscriptionToken> appEventSubs_;

    void startGame(const std::filesystem::path &mapPath);
    void resumeGame();
    void pauseGame();
    void exitToMain();
    bool saveGame(const std::filesystem::path &savePath);
    bool loadSave(const std::filesystem::path &savePath);
    void resetWorld();
    void processGameplayCommands();
};

#endif // DDD_GAME_GAME_APP_H

