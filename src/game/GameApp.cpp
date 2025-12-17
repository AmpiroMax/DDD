#include "game/GameApp.h"
#include "systems/InputSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/RenderSystem.h"
#include "systems/UIRenderSystem.h"
#include "systems/PlayerControlSystem.h"
#include "systems/CameraFollowSystem.h"
#include "systems/TileInteractionSystem.h"
#include "systems/DropPickupSystem.h"
#include "systems/DebugSystem.h"
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
#include <box2d/box2d.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <utility>

GameApp::GameApp() {
    loadConfig();

    windowManager.create(config.windowWidth, config.windowHeight, config.windowTitle);
    cameraManager.setViewportSize({static_cast<float>(config.windowWidth), static_cast<float>(config.windowHeight)});
    resourceManager.setBasePaths(config.resourcesPath, config.texturesPath, config.fontsPath);

    // Optional debug font; prefer existing fonts in resources/.
    const std::vector<std::string> debugFontCandidates = {"ArialRegular.ttf", "RobotoMono-VariableFont_wght.ttf", "debug.ttf"};
    for (const auto &fontFile : debugFontCandidates) {
        try {
            resourceManager.loadFont("debug", resourceManager.resolveFontPath(fontFile));
            break;
        } catch (const std::exception &) {
            // Try next candidate
        }
    }

    loadResources();
    initSystems();
    refreshMapList();
    currentMapPath = std::filesystem::path("config") / config.mapFile;
    hasSaveFile = std::filesystem::exists(std::filesystem::path("saves") / "latest.json");
    setMenuVisible(AppScreen::MainMenu);
    buildMenuButtons();
}

void GameApp::loadConfig() {
    std::ifstream file("config/game.json");
    if (!file.is_open())
        return;

    try {
        nlohmann::json j;
        file >> j;

        if (j.contains("window")) {
            const auto &w = j["window"];
            if (w.contains("size") && w["size"].is_array() && w["size"].size() >= 2) {
                config.windowWidth = w["size"][0].get<int>();
                config.windowHeight = w["size"][1].get<int>();
            }
            if (w.contains("title"))
                config.windowTitle = w["title"].get<std::string>();
        }

        if (j.contains("resources_path"))
            config.resourcesPath = j["resources_path"].get<std::string>();
        if (j.contains("textures_path"))
            config.texturesPath = j["textures_path"].get<std::string>();
        if (j.contains("fonts_path"))
            config.fontsPath = j["fonts_path"].get<std::string>();
        if (j.contains("world")) {
            const auto &w = j["world"];
            if (w.contains("tile_size"))
                config.tileSize = w["tile_size"].get<float>() / RENDER_SCALE;
            if (w.contains("map_file"))
                config.mapFile = w["map_file"].get<std::string>();
        }
        if (j.contains("inventory_file"))
            config.inventoryFile = j["inventory_file"].get<std::string>();
        if (j.contains("player")) {
            const auto &p = j["player"];
            if (p.contains("speed"))
                config.playerSpeed = p["speed"].get<float>();
            if (p.contains("jump_impulse"))
                config.playerJump = p["jump_impulse"].get<float>();
            if (p.contains("pickup_radius"))
                config.pickupRadius = p["pickup_radius"].get<float>() / RENDER_SCALE;
        }
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse config/game.json: " << e.what() << "\n";
    }
}

void GameApp::loadResources() {
    // Load tileset: prefer tileMap.png if present, otherwise tiles.png.
    const std::string tileMapPath = resourceManager.resolveTexturePath("tileMap.png");
    const std::string tilesPath = resourceManager.resolveTexturePath("tiles.png");
    const std::string *chosenTexture = nullptr;
    if (std::filesystem::exists(tileMapPath)) {
        chosenTexture = &tileMapPath;
    } else if (std::filesystem::exists(tilesPath)) {
        chosenTexture = &tilesPath;
    }

    if (chosenTexture) {
        try {
            resourceManager.loadTexture("tiles", *chosenTexture);
            const int tilePx = 32;
            const std::pair<std::string, std::pair<int, int>> regions[] = {
                {"ground", {0, 0}},       {"path", {1, 0}},      {"grass_alt", {6, 0}},   {"grass_dark", {7, 0}},
                {"water", {10, 0}},       {"stone_brick", {11, 0}}, {"dirt", {12, 0}}, {"roof", {13, 0}},
                {"trunk", {1, 1}},        {"leaves", {6, 1}},
            };
            for (const auto &r : regions) {
                const auto [name, pos] = r;
                const int x = pos.first * tilePx;
                const int y = pos.second * tilePx;
                resourceManager.registerAtlasRegion(name, "tiles", sf::IntRect{x, y, tilePx, tilePx});
            }
        } catch (const std::exception &e) {
            std::cerr << "Resource load failed: " << e.what() << "\n";
        }
    }

    // Load a visible debug font from available assets.
    const std::string arialPath = resourceManager.resolveFontPath("ArialRegular.ttf");
    const std::string robotoPath = resourceManager.resolveFontPath("RobotoMono-VariableFont_wght.ttf");
    const std::string *chosenFont = nullptr;
    if (std::filesystem::exists(arialPath)) {
        chosenFont = &arialPath;
    } else if (std::filesystem::exists(robotoPath)) {
        chosenFont = &robotoPath;
    }
    if (chosenFont) {
        try {
            resourceManager.loadFont("debug", *chosenFont);
        } catch (const std::exception &e) {
            std::cerr << "Debug font load failed: " << e.what() << "\n";
        }
    }
}

void GameApp::initSystems() {
    updateSystems.clear();
    renderSystems.clear();

    auto inputPtr = std::make_unique<InputSystem>(windowManager, cameraManager, entityManager);
    inputPtr->loadBindingsFromFile("config/input.json");
    inputSystem = inputPtr.get();

    auto inventoryPtr = std::make_unique<InventorySystem>(*inputSystem, entityManager, eventBus);
    const std::filesystem::path inventoryPath = std::filesystem::path("config") / config.inventoryFile;
    inventoryPtr->loadConfigFromFile(inventoryPath.string());
    inventorySystem = inventoryPtr.get();

    physicsSystem = std::make_unique<PhysicsSystem>(physicsManager, entityManager, eventBus);

    updateSystems.push_back(std::move(inputPtr));
    updateSystems.push_back(std::move(inventoryPtr));
    updateSystems.push_back(
        std::make_unique<DropPickupSystem>(entityManager, *inventorySystem, physicsManager, config.pickupRadius));
    updateSystems.push_back(std::make_unique<PlayerControlSystem>(*inputSystem, entityManager, eventBus, config.playerSpeed,
                                                                  config.playerJump));
    updateSystems.push_back(std::make_unique<CameraFollowSystem>(cameraManager, entityManager));
    updateSystems.push_back(std::make_unique<TileInteractionSystem>(*inputSystem, entityManager, eventBus, inventorySystem));
    updateSystems.push_back(std::make_unique<DebugSystem>(entityManager, debugManager, *inputSystem));

    renderSystems.push_back(std::make_unique<RenderSystem>(windowManager, cameraManager, resourceManager, entityManager));
    auto uiPtr = std::make_unique<UIRenderSystem>(windowManager, resourceManager, debugManager, eventBus);
    uiRenderSystem = uiPtr.get();
    uiRenderSystem->setMenuState(&menuRenderState);
    renderSystems.push_back(std::move(uiPtr));
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

        updateMenu(dt);
        const bool menuActive = (screen != AppScreen::Playing);

        for (auto &sys : updateSystems) {
            if (sys.get() == inputSystem)
                continue; // already updated
            if (!menuActive || sys.get() == inputSystem)
            sys->update(dt);
        }

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
    nlohmann::json j;
    bool loaded = false;

    if (std::filesystem::exists(mapPath)) {
        std::ifstream in(mapPath);
        if (in.is_open()) {
            try {
                in >> j;
                loaded = true;
            } catch (const std::exception &e) {
                std::cerr << "Failed to parse map file " << mapPath << ": " << e.what() << "\n";
            }
        }
    }

    // Defaults / fallback.
    int width = 16;
    int height = 12;
    float tileSize = config.tileSize;
    int emptyId = -1;
    std::vector<int> tiles;
    std::unordered_map<int, std::string> idToRegion;
    std::vector<int> solidIds{1};
    Vec2 origin{0.0f, 0.0f};
    Vec2 playerSpawn{1.5f, 2.0f};

    if (loaded) {
        width = j.value("width", width);
        height = j.value("height", height);
        tileSize = j.value("tile_size", tileSize);
        emptyId = j.value("empty_id", emptyId);
        if (j.contains("origin") && j["origin"].is_array() && j["origin"].size() >= 2) {
            origin.x = j["origin"][0].get<float>();
            origin.y = j["origin"][1].get<float>();
        }
        if (j.contains("player_spawn") && j["player_spawn"].is_array() && j["player_spawn"].size() >= 2) {
            playerSpawn.x = j["player_spawn"][0].get<float>();
            playerSpawn.y = j["player_spawn"][1].get<float>();
        }
        if (j.contains("solid_ids") && j["solid_ids"].is_array()) {
            solidIds.clear();
            for (const auto &v : j["solid_ids"]) {
                solidIds.push_back(v.get<int>());
            }
        }
        if (j.contains("tile_id_to_region") && j["tile_id_to_region"].is_object()) {
            for (auto &[k, v] : j["tile_id_to_region"].items()) {
                idToRegion[std::stoi(k)] = v.get<std::string>();
            }
        }

        const auto readTilesFlat = [&]() {
            if (!j.contains("tiles"))
                return false;
            const auto &arr = j["tiles"];
            if (!arr.is_array())
                return false;
            if (!arr.empty() && arr[0].is_array()) {
                // 2D array
                for (const auto &row : arr) {
                    for (const auto &cell : row)
                        tiles.push_back(cell.get<int>());
                }
            } else {
                for (const auto &cell : arr)
                    tiles.push_back(cell.get<int>());
            }
            return true;
        };

        if (!readTilesFlat()) {
            tiles.assign(width * height, emptyId);
        }
    } else {
        tiles.assign(width * height, emptyId);
        // Simple ground layer on the bottom row.
        const int groundY = height - 1;
        for (int x = 0; x < width; ++x) {
            tiles[groundY * width + x] = 1;
        }
    }

    // Ensure tile buffer size.
    if (static_cast<int>(tiles.size()) < width * height)
        tiles.resize(width * height, emptyId);

    Entity &tileEnt = entityManager.create();
    auto *tileTransform = tileEnt.addComponent<TransformComponent>();
    tileTransform->position = {0.0f, 0.0f};

    auto *tilemap = tileEnt.addComponent<TilemapComponent>();
    tilemap->width = width;
    tilemap->height = height;
    tilemap->tileSize = tileSize;
    tilemap->origin = origin;
    tilemap->emptyId = emptyId;
    tilemap->solidIds = solidIds;
    tilemap->tiles = std::move(tiles);
    tilemap->originalTiles = tilemap->tiles;
    tilemap->tileIdToRegion = std::move(idToRegion);

    // Register atlas regions on demand if missing, using default rect derived from tile size.
    const int defaultTilePx = static_cast<int>(tileSize * RENDER_SCALE);
    for (const auto &kv : tilemap->tileIdToRegion) {
        const std::string &region = kv.second;
        if (!resourceManager.hasAtlasRegion(region) && resourceManager.hasTexture("tiles")) {
            resourceManager.registerAtlasRegion(region, "tiles", sf::IntRect{0, 0, defaultTilePx, defaultTilePx});
        }
    }

    Entity &player = entityManager.create();
    player.addComponent<PlayerTag>();
    player.addComponent<CameraTargetTag>();
    player.addComponent<GroundedComponent>();
    auto *pTransform = player.addComponent<TransformComponent>();
    pTransform->position = playerSpawn;

    auto *body = player.addComponent<PhysicsBodyComponent>();
    body->bodyType = b2_dynamicBody;
    body->position = playerSpawn;
    body->fixture.shape = PhysicsShapeType::Box;
    body->fixture.size = Vec2{tileSize * 0.8f, tileSize * 1.6f};
    body->fixture.density = 1.0f;
    body->fixture.friction = 0.2f;
    body->fixture.restitution = 0.0f;
    body->fixture.canRotate = false;
    body->fixture.isFootSensor = true; // use main fixture to detect grounded for now

    // Temporary player sprite using tiles texture; scaled to collider size.
    auto *sprite = player.addComponent<SpriteComponent>();
    sprite->textureName = "tiles";
    sprite->useTextureRect = true;
    sprite->textureRect = sf::IntRect{0, 0, 32, 32};
    sprite->origin = Vec2{16.0f, 16.0f};
    sprite->scale = Vec2{0.8f, 1.6f}; // world 0.8x1.6 assuming 32px tile
    sprite->z = 0;

    if (inventorySystem)
        inventorySystem->attachToEntity(player);

    // Basic camera focus
    cameraManager.setCenter(playerSpawn);
}

void GameApp::refreshMapList() {
    mapFiles.clear();
    const std::filesystem::path mapsDir = std::filesystem::path("config") / "maps";
    if (!std::filesystem::exists(mapsDir))
        return;
    for (const auto &entry : std::filesystem::directory_iterator(mapsDir)) {
        if (!entry.is_regular_file())
            continue;
        if (entry.path().extension() == ".json") {
            mapFiles.push_back(entry.path());
        }
    }
    std::sort(mapFiles.begin(), mapFiles.end());
    if (!mapFiles.empty() && selectedMapIndex < 0)
        selectedMapIndex = 0;
}

void GameApp::resetWorld() {
    physicsManager.resetWorld();
    entityManager.clear();
}

void GameApp::setMenuVisible(AppScreen newScreen) {
    screen = newScreen;
    menuRenderState.visible = (screen != AppScreen::Playing);
    menuRenderState.showMenuButton = (screen == AppScreen::Playing);
    menuRenderState.menuButtonRect = menuButtonRect;
    switch (screen) {
    case AppScreen::MainMenu:
        menuRenderState.screen = MenuScreen::Main;
        break;
    case AppScreen::MapSelect:
        refreshMapList();
        menuRenderState.screen = MenuScreen::MapSelect;
        break;
    case AppScreen::PauseMenu:
        menuRenderState.screen = MenuScreen::Pause;
        break;
    case AppScreen::Playing:
        break;
    case AppScreen::Settings:
        menuRenderState.screen = MenuScreen::Settings;
        break;
    }
    buildMenuButtons();
}

void GameApp::buildMenuButtons() {
    menuRenderState.buttons.clear();
    menuRenderState.maps.clear();
    for (const auto &p : mapFiles) {
        menuRenderState.maps.push_back(p.filename().string());
    }
    menuRenderState.selectedMap = selectedMapIndex;
    menuRenderState.hasSave = hasSaveFile;
    menuRenderState.canResume = (screen == AppScreen::PauseMenu);
    menuRenderState.showMenuButton = (screen == AppScreen::Playing);
    menuRenderState.menuButtonRect = menuButtonRect;
    menuRenderState.showDebugButton = false;

    const sf::Vector2u ws = windowManager.getWindow().getSize();
    const float btnWidth = 320.0f;
    const float btnHeight = 44.0f;
    const float startX = 0.5f * (ws.x - btnWidth);
    float startY = 0.0f;

    auto pushBtn = [&](const std::string &id, const std::string &label) {
        menuRenderState.buttons.push_back(
            {id, label, sf::FloatRect{startX, startY + btnHeight * static_cast<float>(menuRenderState.buttons.size()),
                                      btnWidth, btnHeight},
             false});
    };

    if (screen == AppScreen::MainMenu) {
        startY = 140.0f;
        if (hasSaveFile)
            pushBtn("continue", "Continue");
        pushBtn("play", "Play");
        pushBtn("exit", "Exit");
    } else if (screen == AppScreen::MapSelect) {
        startY = 100.0f;
        float y = startY;
        for (size_t i = 0; i < menuRenderState.maps.size(); ++i) {
            menuRenderState.buttons.push_back(
                {"map_" + std::to_string(i), menuRenderState.maps[i], sf::FloatRect{startX, y, btnWidth, btnHeight},
                 static_cast<int>(i) == selectedMapIndex});
            y += btnHeight + 10.0f;
        }
        menuRenderState.buttons.push_back({"back", "Back", sf::FloatRect{startX, y + 10.0f, btnWidth, btnHeight}, false});
    } else if (screen == AppScreen::PauseMenu) {
        startY = 140.0f;
        pushBtn("resume", "Continue");
        pushBtn("settings", "Settings");
        pushBtn("save_exit", "Save & Exit");
    } else if (screen == AppScreen::Settings) {
        startY = 140.0f;
        pushBtn("back_pause", "Back");
        pushBtn("resume", "Continue");
        // Build settings lines from bindings.
        menuRenderState.settingsLines.clear();
        if (inputSystem) {
            const auto &bindings = inputSystem->getBindings();
            for (const auto &kv : bindings) {
                const auto &action = kv.first;
                const auto &binding = kv.second;
                std::string line = action + ":";
                auto appendKey = [&](const std::string &s) {
                    if (!line.empty())
                        line += " ";
                    line += s;
                };
                auto keyName = [](sf::Keyboard::Key k) -> std::string {
                    switch (k) {
                    case sf::Keyboard::Escape:
                        return "Esc";
                    case sf::Keyboard::A:
                        return "A";
                    case sf::Keyboard::B:
                        return "B";
                    case sf::Keyboard::C:
                        return "C";
                    case sf::Keyboard::D:
                        return "D";
                    case sf::Keyboard::E:
                        return "E";
                    case sf::Keyboard::Q:
                        return "Q";
                    case sf::Keyboard::S:
                        return "S";
                    case sf::Keyboard::W:
                        return "W";
                    case sf::Keyboard::Left:
                        return "Left";
                    case sf::Keyboard::Right:
                        return "Right";
                    case sf::Keyboard::Up:
                        return "Up";
                    case sf::Keyboard::Down:
                        return "Down";
                    case sf::Keyboard::Space:
                        return "Space";
                    case sf::Keyboard::Num0:
                        return "0";
                    case sf::Keyboard::Num1:
                        return "1";
                    case sf::Keyboard::Num2:
                        return "2";
                    case sf::Keyboard::Num3:
                        return "3";
                    case sf::Keyboard::Num4:
                        return "4";
                    case sf::Keyboard::Num5:
                        return "5";
                    case sf::Keyboard::Num6:
                        return "6";
                    case sf::Keyboard::Num7:
                        return "7";
                    case sf::Keyboard::Num8:
                        return "8";
                    case sf::Keyboard::Num9:
                        return "9";
                    default:
                        return "Key";
                    }
                };
                auto mouseName = [](sf::Mouse::Button b) -> std::string {
                    switch (b) {
                    case sf::Mouse::Left:
                        return "MouseLeft";
                    case sf::Mouse::Right:
                        return "MouseRight";
                    case sf::Mouse::Middle:
                        return "MouseMiddle";
                    default:
                        return "Mouse";
                    }
                };
                for (auto k : binding.keys)
                    appendKey(keyName(k));
                for (auto m : binding.mouseButtons)
                    appendKey(mouseName(m));
                for (auto w : binding.wheelDirections)
                    appendKey(w > 0 ? "WheelUp" : "WheelDown");
                menuRenderState.settingsLines.push_back(line);
            }
        }
    }
}

void GameApp::handleMenuClicks() {
    InputComponent *input = inputSystem ? inputSystem->getInput() : nullptr;
    if (!input)
        return;

    // While playing: only the menu button is interactive.
    if (screen == AppScreen::Playing) {
        auto itMb = input->mouseButtons.find(static_cast<int>(sf::Mouse::Left));
        if (itMb != input->mouseButtons.end() && itMb->second.pressed) {
            const sf::Vector2i mouse = input->mousePixel;
            const sf::Vector2f mf(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
            if (menuButtonRect.contains(mf)) {
                pauseGame();
            }
        }
        return;
    }

    auto it = input->mouseButtons.find(static_cast<int>(sf::Mouse::Left));
    if (it == input->mouseButtons.end() || !it->second.pressed)
        return;
    const sf::Vector2i mouse = input->mousePixel;
    const sf::Vector2f mf(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

    for (const auto &btn : menuRenderState.buttons) {
        if (!btn.rect.contains(mf))
            continue;
        if (btn.id == "play") {
            setMenuVisible(AppScreen::MapSelect);
        } else if (btn.id == "exit") {
            windowManager.getWindow().close();
        } else if (btn.id == "continue") {
            if (!loadSave(std::filesystem::path("saves") / "latest.json"))
                startGame(currentMapPath);
        } else if (btn.id.rfind("map_", 0) == 0) {
            const int idx = std::stoi(btn.id.substr(4));
            if (idx >= 0 && idx < static_cast<int>(mapFiles.size())) {
                selectedMapIndex = idx;
                currentMapPath = mapFiles[idx];
                startGame(currentMapPath);
            }
        } else if (btn.id == "back") {
            setMenuVisible(AppScreen::MainMenu);
        } else if (btn.id == "resume") {
            resumeGame();
        } else if (btn.id == "settings") {
            setMenuVisible(AppScreen::Settings);
        } else if (btn.id == "save_exit") {
            // exit without saving (per current iteration)
            exitToMain();
        } else if (btn.id == "back_pause") {
            setMenuVisible(AppScreen::PauseMenu);
        }
        break;
    }
}

void GameApp::updateMenu(float dt) {
    (void)dt;
    InputComponent *input = inputSystem ? inputSystem->getInput() : nullptr;
    if (!input)
        return;

    const auto getAction = [&](const std::string &name) -> const ButtonState * {
        auto it = input->actions.find(name);
        return it != input->actions.end() ? &it->second : nullptr;
    };
    const ButtonState *menuAction = getAction("menu");
    const ButtonState *debugToggle = getAction("debug_toggle");

    if (debugToggle && debugToggle->pressed)
        debugManager.setVisible(!debugManager.isVisible());

    // Hover highlight
    const sf::Vector2i mouse = input->mousePixel;
    const sf::Vector2f mf(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
    for (auto &btn : menuRenderState.buttons) {
        btn.focused = btn.rect.contains(mf);
    }

    if (screen == AppScreen::Playing) {
        if (menuAction && menuAction->pressed) {
            pauseGame();
            return;
        }
        handleMenuClicks();
        return;
    }

    // Menu visible states
    if (menuAction && menuAction->pressed) {
        if (screen == AppScreen::PauseMenu) {
            resumeGame();
        } else if (screen == AppScreen::MapSelect) {
            setMenuVisible(AppScreen::MainMenu);
        } else if (screen == AppScreen::Settings) {
            setMenuVisible(AppScreen::PauseMenu);
        }
        return;
    }

    handleMenuClicks();
}

void GameApp::pauseGame() {
    setMenuVisible(AppScreen::PauseMenu);
}

void GameApp::resumeGame() {
    screen = AppScreen::Playing;
    menuRenderState.visible = false;
}

void GameApp::exitToMain() {
    screen = AppScreen::MainMenu;
    menuRenderState.visible = true;
    buildMenuButtons();
}

void GameApp::startGame(const std::filesystem::path &mapPath) {
    currentMapPath = mapPath;
    if (physicsSystem)
        physicsSystem->reset();
    resetWorld();
    if (inputSystem)
        inputSystem->resetInputEntity();
    loadMapAndEntities(mapPath);
    screen = AppScreen::Playing;
    menuRenderState.visible = false;
}

bool GameApp::saveGame(const std::filesystem::path &savePath) {
    const auto dataOpt = collectSaveData();
    if (!dataOpt)
        return false;
    const auto &data = *dataOpt;
    nlohmann::json j;
    j["base_map"] = data.baseMap.string();
    j["placed_tiles"] = nlohmann::json::array();
    for (const auto &t : data.placed)
        j["placed_tiles"].push_back({{"x", t.x}, {"y", t.y}, {"tile_id", t.tileId}});
    j["removed_tiles"] = nlohmann::json::array();
    for (const auto &t : data.removed)
        j["removed_tiles"].push_back({{"x", t.x}, {"y", t.y}});
    j["drops"] = nlohmann::json::array();
    for (const auto &d : data.drops)
        j["drops"].push_back({{"item_id", d.itemId},
                              {"count", d.count},
                              {"pos", {d.px, d.py}},
                              {"vel", {d.vx, d.vy}}});
    j["player"] = {{"pos", {data.player.px, data.player.py}},
                   {"vel", {data.player.vx, data.player.vy}},
                   {"hp", data.player.hp},
                   {"max_hp", data.player.maxHp},
                   {"active_slot", data.player.activeSlot}};
    j["player"]["inventory"] = nlohmann::json::array();
    for (const auto &s : data.player.slots) {
        j["player"]["inventory"].push_back({{"item_id", s.itemId}, {"count", s.count}});
    }

    std::filesystem::create_directories(savePath.parent_path());
    std::ofstream out(savePath);
    if (!out.is_open())
        return false;
    out << j.dump(2);
    hasSaveFile = true;
    return true;
}

std::optional<GameApp::SaveData> GameApp::collectSaveData() {
    SaveData data{};
    if (currentMapPath.empty())
        return std::nullopt;
    data.baseMap = currentMapPath;

    TilemapComponent *tilemap = nullptr;
    Entity *player = nullptr;
    for (auto &entPtr : entityManager.all()) {
        if (!tilemap)
            tilemap = entPtr->get<TilemapComponent>();
        if (!player && entPtr->has<PlayerTag>())
            player = entPtr.get();
    }
    if (!tilemap || !player)
        return std::nullopt;

    const int w = tilemap->width;
    const int h = tilemap->height;
    if (!tilemap->originalTiles.empty()) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const int idx = tilemap->index(x, y);
                if (idx < 0 || idx >= static_cast<int>(tilemap->tiles.size()) ||
                    idx >= static_cast<int>(tilemap->originalTiles.size()))
                    continue;
                const int base = tilemap->originalTiles[idx];
                const int cur = tilemap->tiles[idx];
                if (cur == base)
                    continue;
                if (cur == tilemap->emptyId) {
                    data.removed.push_back({x, y, cur});
                } else {
                    data.placed.push_back({x, y, cur});
                }
            }
        }
    }

    for (auto &entPtr : entityManager.all()) {
        auto *drop = entPtr->get<DropComponent>();
        auto *transform = entPtr->get<TransformComponent>();
        if (!drop || !transform)
            continue;
        SaveData::DropInfo info{};
        info.itemId = drop->itemId;
        info.count = drop->count;
        info.px = transform->position.x;
        info.py = transform->position.y;
        if (auto *body = entPtr->get<PhysicsBodyComponent>(); body && body->body) {
            const b2Vec2 vel = body->body->GetLinearVelocity();
            const Vec2 wv = physicsToWorld(Vec2{vel.x, vel.y});
            info.vx = wv.x;
            info.vy = wv.y;
        }
        data.drops.push_back(info);
    }

    if (auto *t = player->get<TransformComponent>()) {
        data.player.px = t->position.x;
        data.player.py = t->position.y;
    }
    if (auto *body = player->get<PhysicsBodyComponent>(); body && body->body) {
        const b2Vec2 vel = body->body->GetLinearVelocity();
        const Vec2 wv = physicsToWorld(Vec2{vel.x, vel.y});
        data.player.vx = wv.x;
        data.player.vy = wv.y;
    }
    if (auto *inv = player->get<InventoryComponent>()) {
        data.player.activeSlot = inv->activeSlot;
        data.player.slots = inv->slots;
    }

    return data;
}

bool GameApp::loadSave(const std::filesystem::path &savePath) {
    std::ifstream in(savePath);
    if (!in.is_open())
        return false;
    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse save: " << e.what() << "\n";
        return false;
    }
    SaveData data{};
    if (j.contains("base_map"))
        data.baseMap = j["base_map"].get<std::string>();
    if (data.baseMap.empty())
        data.baseMap = currentMapPath;

    if (j.contains("placed_tiles")) {
        for (const auto &t : j["placed_tiles"]) {
            data.placed.push_back(
                {t.value("x", 0), t.value("y", 0), t.value("tile_id", -1)});
        }
    }
    if (j.contains("removed_tiles")) {
        for (const auto &t : j["removed_tiles"]) {
            data.removed.push_back({t.value("x", 0), t.value("y", 0), -1});
        }
    }
    if (j.contains("drops")) {
        for (const auto &d : j["drops"]) {
            SaveData::DropInfo info{};
            info.itemId = d.value("item_id", -1);
            info.count = d.value("count", 0);
            if (d.contains("pos") && d["pos"].is_array() && d["pos"].size() >= 2) {
                info.px = d["pos"][0].get<float>();
                info.py = d["pos"][1].get<float>();
            }
            if (d.contains("vel") && d["vel"].is_array() && d["vel"].size() >= 2) {
                info.vx = d["vel"][0].get<float>();
                info.vy = d["vel"][1].get<float>();
            }
            data.drops.push_back(info);
        }
    }
    if (j.contains("player")) {
        const auto &p = j["player"];
        if (p.contains("pos") && p["pos"].is_array() && p["pos"].size() >= 2) {
            data.player.px = p["pos"][0].get<float>();
            data.player.py = p["pos"][1].get<float>();
        }
        if (p.contains("vel") && p["vel"].is_array() && p["vel"].size() >= 2) {
            data.player.vx = p["vel"][0].get<float>();
            data.player.vy = p["vel"][1].get<float>();
        }
        data.player.hp = p.value("hp", 100);
        data.player.maxHp = p.value("max_hp", 100);
        data.player.activeSlot = p.value("active_slot", 0);
        if (p.contains("inventory") && p["inventory"].is_array()) {
            for (const auto &slot : p["inventory"]) {
                ItemSlot s{};
                s.itemId = slot.value("item_id", -1);
                s.count = slot.value("count", 0);
                data.player.slots.push_back(s);
            }
        }
    }

    startGame(data.baseMap);
    hasSaveFile = true;
    return applySaveData(data);
}

bool GameApp::applySaveData(const SaveData &data) {
    TilemapComponent *tilemap = nullptr;
    Entity *player = nullptr;
    for (auto &entPtr : entityManager.all()) {
        if (!tilemap)
            tilemap = entPtr->get<TilemapComponent>();
        if (!player && entPtr->has<PlayerTag>())
            player = entPtr.get();
    }
    if (!tilemap || !player)
        return false;

    for (const auto &t : data.removed) {
        if (tilemap->inBounds(t.x, t.y))
            tilemap->tiles[tilemap->index(t.x, t.y)] = tilemap->emptyId;
    }
    for (const auto &t : data.placed) {
        if (tilemap->inBounds(t.x, t.y))
            tilemap->tiles[tilemap->index(t.x, t.y)] = t.tileId;
    }

    if (auto *t = player->get<TransformComponent>()) {
        t->position = Vec2{data.player.px, data.player.py};
    }
    if (auto *body = player->get<PhysicsBodyComponent>(); body && body->body) {
        const Vec2 pv = worldToPhysics(Vec2{data.player.vx, data.player.vy});
        body->body->SetLinearVelocity(b2Vec2{pv.x, pv.y});
        body->body->SetTransform(worldToPhysics(Vec2{data.player.px, data.player.py}), body->body->GetAngle());
    }
    if (auto *inv = player->get<InventoryComponent>()) {
        const int limit = std::min(static_cast<int>(inv->slots.size()), static_cast<int>(data.player.slots.size()));
        for (int i = 0; i < limit; ++i) {
            inv->slots[i] = data.player.slots[i];
        }
        inv->activeSlot = std::clamp(data.player.activeSlot, 0, static_cast<int>(inv->slots.size()) - 1);
    }

    // Drops
    for (const auto &d : data.drops) {
        Entity &dropEnt = entityManager.create();
        auto *transform = dropEnt.addComponent<TransformComponent>();
        transform->position = Vec2{d.px, d.py};

        auto *body = dropEnt.addComponent<PhysicsBodyComponent>();
        body->bodyType = b2_dynamicBody;
        body->position = transform->position;
        body->fixture.shape = PhysicsShapeType::Box;
        const float dropSize = tilemap->tileSize / 3.0f;
        body->fixture.size = Vec2{dropSize, dropSize};
        body->fixture.density = 0.5f;
        body->fixture.friction = 0.8f;
        body->fixture.restitution = 0.0f;
        body->fixture.linearDamping = 1.0f;
        body->fixture.angularDamping = 1.0f;
        body->fixture.canRotate = true;
        body->fixture.isSensor = false;

        auto *dropComp = dropEnt.addComponent<DropComponent>();
        dropComp->itemId = d.itemId;
        dropComp->count = d.count;

        auto *sprite = dropEnt.addComponent<SpriteComponent>();
        sprite->textureName = "tiles";
        sprite->useTextureRect = true;
        sprite->textureRect = sf::IntRect{0, 0, 32, 32};
        sprite->origin = Vec2{16.0f, 16.0f};
        const float dropScale = tilemap->tileSize / 3.0f;
        sprite->scale = Vec2{dropScale, dropScale};

        if (tilemap) {
            auto itRegion = tilemap->tileIdToRegion.find(dropComp->itemId);
            if (itRegion != tilemap->tileIdToRegion.end()) {
                sprite->atlasRegion = itRegion->second;
                sprite->useTextureRect = false;
            }
        }
    }

    return true;
}

