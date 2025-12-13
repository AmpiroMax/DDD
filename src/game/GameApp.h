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
#include "systems/CameraFollowSystem.h"
#include "systems/DebugSystem.h"
#include "systems/PlayerControlSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/UIRenderSystem.h"
#include "systems/TileInteractionSystem.h"
#include "systems/InventorySystem.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <filesystem>
#include <SFML/Graphics/Rect.hpp>

class GameApp {
  public:
    GameApp();
    void run();

  private:
    void initSystems();
    void loadConfig();
    void loadResources();
    void loadMapAndEntities(const std::filesystem::path &mapPath);

    struct GameConfig {
        int windowWidth{1280};
        int windowHeight{720};
        std::string windowTitle{"DDD Terraria-like MVP"};
        std::string resourcesPath{"resources"};
        std::string texturesPath{"textures"};
        std::string fontsPath{"fonts"};
        std::string mapFile{"maps/level01.json"};
        std::string inventoryFile{"inventory.json"};
        float tileSize{1.0f};
        float playerSpeed{6.0f};
        float playerJump{8.0f};
        float pickupRadius{1.5f};
    };

    enum class AppScreen { MainMenu, MapSelect, Playing, PauseMenu, Settings };

    using MenuRenderButton = UIRenderSystem::MenuRenderButton;
    using MenuRenderState = UIRenderSystem::MenuRenderState;
    using MenuScreen = UIRenderSystem::MenuScreen;

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
    UIRenderSystem *uiRenderSystem{nullptr};   // owned by renderSystems
    std::unique_ptr<PhysicsSystem> physicsSystem;

    std::vector<std::unique_ptr<System>> updateSystems; // logic (input/player/camera/tile/debug)
    std::vector<std::unique_ptr<System>> renderSystems; // render & UI

    AppScreen screen{AppScreen::MainMenu};
    MenuRenderState menuRenderState;
    std::vector<std::filesystem::path> mapFiles;
    int selectedMapIndex{-1};
    bool hasSaveFile{false};
    std::filesystem::path currentMapPath;
    sf::FloatRect menuButtonRect{12.0f, 72.0f, 96.0f, 32.0f};

    void refreshMapList();
    void updateMenu(float dt);
    void buildMenuButtons();
    void handleMenuClicks();
    void setMenuVisible(AppScreen newScreen);
    void startGame(const std::filesystem::path &mapPath);
    void resumeGame();
    void pauseGame();
    void exitToMain();
    bool saveGame(const std::filesystem::path &savePath);
    bool loadSave(const std::filesystem::path &savePath);
    std::optional<SaveData> collectSaveData();
    bool applySaveData(const SaveData &data);
    void resetWorld();
};

#endif // DDD_GAME_GAME_APP_H

