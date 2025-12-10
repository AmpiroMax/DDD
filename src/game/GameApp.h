#ifndef DDD_GAME_GAME_APP_H
#define DDD_GAME_GAME_APP_H

#include "core/EntityManager.h"
#include "core/EventBus.h"
#include "core/System.h"
#include "managers/CameraManager.h"
#include "managers/DebugManager.h"
#include "managers/PhysicsManager.h"
#include "managers/ResourceManager.h"
#include "managers/TimeManager.h"
#include "managers/WindowManager.h"
#include "systems/InputSystem.h"
#include "systems/PhysicsSystem.h"
#include <memory>
#include <string>
#include <vector>

class GameApp {
  public:
    GameApp();
    void run();

  private:
    void initSystems();
    void loadConfig();

    struct GameConfig {
        int windowWidth{1280};
        int windowHeight{720};
        std::string windowTitle{"DDD Terraria-like MVP"};
        std::string resourcesPath{"resources"};
        std::string texturesPath{"textures"};
        std::string fontsPath{"fonts"};
    };

    GameConfig config;
    WindowManager windowManager;
    CameraManager cameraManager;
    PhysicsManager physicsManager;
    ResourceManager resourceManager;
    TimeManager timeManager;
    DebugManager debugManager;
    EntityManager entityManager;
    EventBus eventBus;

    InputSystem *inputSystem{nullptr};
    std::unique_ptr<PhysicsSystem> physicsSystem;
    std::vector<std::unique_ptr<System>> systems;
};

#endif // DDD_GAME_GAME_APP_H

