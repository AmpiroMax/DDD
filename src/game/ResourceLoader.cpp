#include "game/ResourceLoader.h"

#include "managers/ResourceManager.h"

#include <SFML/Graphics/Rect.hpp>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>

void ResourceLoader::loadOptionalDebugFontCandidates(ResourceManager &resourceManager,
                                                     const std::vector<std::string> &candidates) {
    for (const auto &fontFile : candidates) {
        try {
            resourceManager.loadFont("debug", resourceManager.resolveFontPath(fontFile));
            break;
        } catch (const std::exception &) {
            // Try next candidate
        }
    }
}

void ResourceLoader::loadTilesAndAtlasRegions(ResourceManager &resourceManager) {
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
                {"ground", {0, 0}},           {"path", {1, 0}},       {"grass_alt", {6, 0}}, {"grass_dark", {7, 0}},
                {"water", {10, 0}},           {"stone_brick", {11, 0}}, {"dirt", {12, 0}}, {"roof", {13, 0}},
                {"trunk", {1, 1}},            {"leaves", {6, 1}},
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
}

void ResourceLoader::loadPreferredDebugFont(ResourceManager &resourceManager) {
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


