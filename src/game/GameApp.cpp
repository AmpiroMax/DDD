#include "game/GameApp.h"
#include "systems/InputSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/RenderSystem.h"
#include "systems/UIRenderSystem.h"
#include "utils/Constants.h"

#include <SFML/Graphics.hpp>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

GameApp::GameApp() {
    loadConfig();

    windowManager.create(config.windowWidth, config.windowHeight, config.windowTitle);
    cameraManager.setViewportSize({static_cast<float>(config.windowWidth), static_cast<float>(config.windowHeight)});
    resourceManager.setBasePaths(config.resourcesPath, config.texturesPath, config.fontsPath);

    // Optional debug font; skip failure silently to avoid hard dependency during bring-up
    try {
        resourceManager.loadFont("debug", resourceManager.resolveFontPath("debug.ttf"));
    } catch (const std::exception &) {
    }

    initSystems();
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
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse config/game.json: " << e.what() << "\n";
    }
}

void GameApp::initSystems() {
    systems.clear();

    auto inputPtr = std::make_unique<InputSystem>(windowManager, cameraManager, entityManager);
    inputPtr->loadBindingsFromFile("config/input.json");
    inputSystem = inputPtr.get();

    physicsSystem = std::make_unique<PhysicsSystem>(physicsManager, entityManager, eventBus);

    systems.push_back(std::move(inputPtr));
    systems.push_back(std::make_unique<RenderSystem>(windowManager, cameraManager, resourceManager, entityManager));
    systems.push_back(std::make_unique<UIRenderSystem>(windowManager, resourceManager, debugManager));
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

        for (auto &sys : systems)
            sys->update(dt);

        if (physicsSystem) {
            while (physicsAccumulator >= PHYSICS_TIMESTEP) {
                physicsSystem->update(PHYSICS_TIMESTEP);
                physicsAccumulator -= PHYSICS_TIMESTEP;
            }
        } else {
            while (physicsAccumulator >= PHYSICS_TIMESTEP)
                physicsAccumulator -= PHYSICS_TIMESTEP;
        }

        eventBus.pump();
    }

    if (window.isOpen())
        window.close();
}

