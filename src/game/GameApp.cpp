#include "game/GameApp.h"
#include "game/ConfigLoader.h"
#include "game/MenuController.h"
#include "game/ResourceLoader.h"
#include "game/SaveSystem.h"
#include "game/SystemsBuilder.h"
#include "game/WorldLoader.h"
#include "systems/InventorySystem.h"
#include "events/AppEvents.h"
#include "systems/InputSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/RenderSystem.h"
#include "systems/UIRenderSystem.h"
#include "systems/PlayerControlSystem.h"
#include "systems/CameraFollowSystem.h"
#include "systems/TileInteractionSystem.h"
#include "systems/SpellSystem.h"
#include "systems/TopDownControlSystem.h"
#include "systems/DropPickupSystem.h"
#include "systems/DebugSystem.h"
#include "systems/SpawnSystem.h"
#include "systems/MatchSystem.h"
#include "systems/RespawnSystem.h"
#include "utils/Constants.h"

#include <SFML/Graphics.hpp>
#include "components/TilemapComponent.h"
#include "components/TransformComponent.h"
#include "components/Tags.h"
#include "components/PhysicsBodyComponent.h"
#include "components/GroundedComponent.h"
#include "components/SpriteComponent.h"
#include "components/DropComponent.h"
#include "utils/CoordinateUtils.h"
#include "utils/IsoCoordinate.h"
#include "utils/IsoConfig.h"
#include <box2d/box2d.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <utility>

GameApp::GameApp() {
    // Menu controller owns menu/UI state machine; GameApp wires it to actions.
    static MenuController menuControllerStorage;
    menuController = &menuControllerStorage;

    loadConfig();

    windowManager.create(config.windowWidth, config.windowHeight, config.windowTitle);
    cameraManager.setViewportSize({static_cast<float>(config.windowWidth), static_cast<float>(config.windowHeight)});
    resourceManager.setBasePaths(config.resourcesPath, config.texturesPath, config.fontsPath);

    // Optional debug font; prefer existing fonts in resources/.
    ResourceLoader::loadOptionalDebugFontCandidates(
        resourceManager, {"ArialRegular.ttf", "RobotoMono-VariableFont_wght.ttf", "debug.ttf"});

    loadResources();
    initSystems();
    currentMapPath = std::filesystem::path("config") / config.mapFile;
    hasSaveFile = std::filesystem::exists(std::filesystem::path("saves") / "latest.json");

    if (menuController) {
        menuController->setCurrentMapPath(currentMapPath);
        menuController->setHasSaveFile(hasSaveFile);
        menuController->refreshMapList();
        menuController->setScreen(MenuController::Screen::MainMenu, windowManager, inputSystem);
    }

    // App-level command handlers (UI/menu -> GameApp) via EventBus.
    appEventSubs_.push_back(eventBus.subscribeWithToken<ExitAppRequestedEvent>(
        [this](const ExitAppRequestedEvent &) { windowManager.getWindow().close(); }));
    appEventSubs_.push_back(eventBus.subscribeWithToken<PauseRequestedEvent>(
        [this](const PauseRequestedEvent &) { pauseGame(); }));
    appEventSubs_.push_back(eventBus.subscribeWithToken<ResumeRequestedEvent>(
        [this](const ResumeRequestedEvent &) { resumeGame(); }));
    appEventSubs_.push_back(eventBus.subscribeWithToken<ExitToMainRequestedEvent>(
        [this](const ExitToMainRequestedEvent &) { exitToMain(); }));
    appEventSubs_.push_back(eventBus.subscribeWithToken<StartGameRequestedEvent>(
        [this](const StartGameRequestedEvent &ev) {
            currentMapPath = ev.mapPath;
            startGame(ev.mapPath);
        }));
    appEventSubs_.push_back(eventBus.subscribeWithToken<ContinueRequestedEvent>(
        [this](const ContinueRequestedEvent &) {
            // Preserve previous behavior: try latest save, fallback to starting current map.
            if (!loadSave(std::filesystem::path("saves") / "latest.json")) {
                startGame(currentMapPath);
            }
        }));
}

GameApp::~GameApp() {
    for (const auto &tok : appEventSubs_)
        eventBus.unsubscribe(tok);
    appEventSubs_.clear();
}

void GameApp::loadConfig() {
    config = ConfigLoader::loadGameConfig("config/game.json");
}

void GameApp::loadResources() {
    ResourceLoader::loadTilesAndAtlasRegions(resourceManager);
    ResourceLoader::loadPreferredDebugFont(resourceManager);
}

void GameApp::initSystems() {
    BuiltSystems built = SystemsBuilder::build(config, windowManager, cameraManager, physicsManager, resourceManager,
                                              entityManager, eventBus, debugManager, menuController, gameplayCommands);

    updateSystems = std::move(built.updateSystems);
    renderSystems = std::move(built.renderSystems);
    physicsSystem = std::move(built.physicsSystem);

    inputSystem = built.inputSystem;
    inventorySystem = built.inventorySystem;
    spellSystem = built.spellSystem;
    uiRenderSystem = built.uiRenderSystem;
    spawnSystem = built.spawnSystem;
    matchSystem = built.matchSystem;
    respawnSystem = built.respawnSystem;
}

void GameApp::run() {
    sf::RenderWindow &window = windowManager.getWindow();

    float physicsAccumulator = 0.0f;
    bool running = true;

    while (running && window.isOpen()) {
        if (inputSystem)
            inputSystem->beginFrame();

        sf::Event evt;
        while (window.pollEvent(evt)) {
            if (evt.type == sf::Event::Closed)
                running = false;
            if (inputSystem)
                inputSystem->handleEvent(evt);
        }

        const float dt = timeManager.tick();
        physicsAccumulator += dt;

        if (inputSystem)
            inputSystem->update(dt);

        // Menu controller consumes input state and drives screen transitions.
        if (menuController && inputSystem) {
            menuController->update(dt, *inputSystem, windowManager, debugManager, eventBus, uiRenderSystem);
            // Process app commands immediately (start/pause/resume/exit) in the same frame.
            eventBus.pump();
        }
        const bool menuActive = menuController ? menuController->isMenuVisible() : true;

        for (auto &sys : updateSystems) {
            if (sys.get() == inputSystem)
                continue; // already updated
            if (!menuActive || sys.get() == inputSystem)
            sys->update(dt);
        }

        // Apply gameplay commands (deferred) before physics to preserve previous ordering.
        processGameplayCommands();

        if (physicsSystem) {
        while (physicsAccumulator >= PHYSICS_TIMESTEP) {
                physicsSystem->update(PHYSICS_TIMESTEP);
            physicsAccumulator -= PHYSICS_TIMESTEP;
        }
        } else {
            physicsAccumulator = 0.0f;
        }

        eventBus.pump();

        for (auto &sys : renderSystems)
            sys->update(dt);
    }

    if (window.isOpen())
        window.close();
}

void GameApp::loadMapAndEntities(const std::filesystem::path &mapPath) {
    WorldLoader::loadMapAndEntities(mapPath, config, entityManager, resourceManager, cameraManager, inventorySystem, spellSystem,
                                   spawnSystem);
}

void GameApp::resetWorld() {
    physicsManager.resetWorld();
    entityManager.clear();
}

void GameApp::pauseGame() {
    if (menuController)
        menuController->setScreen(MenuController::Screen::PauseMenu, windowManager, inputSystem);
}

void GameApp::resumeGame() {
    if (menuController)
        menuController->setScreen(MenuController::Screen::Playing, windowManager, inputSystem);
}

void GameApp::exitToMain() {
    if (menuController)
        menuController->setScreen(MenuController::Screen::MainMenu, windowManager, inputSystem);
}

void GameApp::startGame(const std::filesystem::path &mapPath) {
    currentMapPath = mapPath;
    if (menuController)
        menuController->setCurrentMapPath(currentMapPath);
    if (physicsSystem)
        physicsSystem->reset();
    resetWorld();
    if (inputSystem)
        inputSystem->resetInputEntity();
    loadMapAndEntities(mapPath);
    if (menuController)
        menuController->setScreen(MenuController::Screen::Playing, windowManager, inputSystem);
}

void GameApp::processGameplayCommands() {
    gameplayCommands.consumeAll([&](const GameplayCommand &cmd) {
        std::visit(
            [&](auto &&concrete) {
                using T = std::decay_t<decltype(concrete)>;
                if constexpr (std::is_same_v<T, RespawnCommand>) {
                    Entity *e = entityManager.find(concrete.entityId);
                    if (!e || !spawnSystem)
                        return;
                    if (auto *hp = e->get<HealthComponent>())
                        hp->reset();
                    spawnSystem->respawnEntity(*e);
                    eventBus.emit(RespawnedEvent{concrete.entityId});
                } else if constexpr (std::is_same_v<T, PickupCommand>) {
                    Entity *player = entityManager.find(concrete.playerId);
                    Entity *dropEnt = entityManager.find(concrete.dropEntityId);
                    if (!player || !dropEnt || !inventorySystem)
                        return;
                    auto *drop = dropEnt->get<DropComponent>();
                    auto *transform = dropEnt->get<TransformComponent>();
                    if (!drop || !transform)
                        return;

                    const int remaining = inventorySystem->addItem(concrete.playerId, concrete.itemId, concrete.count);
                    if (remaining <= 0) {
                        // Destroy physics body if any, then remove entity.
                        if (auto *body = dropEnt->get<PhysicsBodyComponent>()) {
                            if (body->body) {
                                physicsManager.destroyBody(body->body);
                                body->body = nullptr;
                            }
                        }
                        entityManager.remove(concrete.dropEntityId);
                    } else {
                        drop->count = remaining;
                    }
                } else if constexpr (std::is_same_v<T, ConsumeItemCommand>) {
                    if (!inventorySystem)
                        return;
                    inventorySystem->consumeFromSlot(concrete.entityId, concrete.slotIndex, concrete.amount);
                    if (concrete.hasTile) {
                        InventoryUseItemEvent useEv{};
                        useEv.entityId = concrete.entityId;
                        useEv.slotIndex = concrete.slotIndex;
                        useEv.itemId = concrete.itemId;
                        useEv.placeTileId = concrete.placeTileId;
                        useEv.tileX = concrete.tileX;
                        useEv.tileY = concrete.tileY;
                        useEv.hasTile = true;
                        eventBus.emit(useEv);
                    }
                }
            },
            cmd);
    });
}

bool GameApp::saveGame(const std::filesystem::path &savePath) {
    const auto dataOpt = SaveSystem::collect(entityManager, currentMapPath);
    if (!dataOpt)
        return false;
    if (!SaveSystem::writeToFile(*dataOpt, savePath))
        return false;
    hasSaveFile = true;
    return true;
}

bool GameApp::loadSave(const std::filesystem::path &savePath) {
    const auto dataOpt = SaveSystem::readFromFile(savePath, currentMapPath);
    if (!dataOpt)
        return false;
    const SaveData &data = *dataOpt;

    startGame(data.baseMap);
    hasSaveFile = true;
    return SaveSystem::apply(entityManager, data);
}

