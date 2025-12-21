#include "game/WorldLoader.h"

#include "components/DropComponent.h"
#include "components/GroundedComponent.h"
#include "components/HealthComponent.h"
#include "components/PhysicsBodyComponent.h"
#include "components/SpriteComponent.h"
#include "components/Tags.h"
#include "components/TilemapComponent.h"
#include "components/TransformComponent.h"
#include "core/Entity.h"
#include "core/EntityManager.h"
#include "managers/CameraManager.h"
#include "managers/ResourceManager.h"
#include "systems/InventorySystem.h"
#include "systems/SpawnSystem.h"
#include "systems/SpellSystem.h"
#include "utils/Constants.h"
#include "utils/Vec2.h"

#include <SFML/Graphics/Rect.hpp>
#include <algorithm>
#include <box2d/box2d.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

void WorldLoader::loadMapAndEntities(const std::filesystem::path &mapPath, const GameConfig &config, EntityManager &entityManager,
                                    ResourceManager &resourceManager, CameraManager &cameraManager,
                                    InventorySystem *inventorySystem, SpellSystem *spellSystem, SpawnSystem *spawnSystem) {
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
    auto *pTransform = player.addComponent<TransformComponent>();
    pTransform->position = playerSpawn;

    auto *body = player.addComponent<PhysicsBodyComponent>();
    body->bodyType = b2_dynamicBody;
    body->position = playerSpawn;
    if (config.mode == "diablo") {
        body->fixture.shape = PhysicsShapeType::Circle;
        body->fixture.radius = config.playerRadius;
    } else {
        body->fixture.shape = PhysicsShapeType::Box;
        body->fixture.size = Vec2{tileSize * 0.8f, tileSize * 1.6f};
        player.addComponent<GroundedComponent>();
        body->fixture.isFootSensor = true; // legacy grounded via main fixture for now
    }
    body->fixture.density = 1.0f;
    body->fixture.friction = (config.mode == "diablo") ? 0.0f : 0.2f;
    body->fixture.restitution = 0.0f;
    if (config.mode == "diablo") {
        body->fixture.linearDamping = config.playerLinearDamping;
        body->fixture.angularDamping = 0.0f;
    }
    body->fixture.canRotate = false;

    // Temporary player sprite using tiles texture; scaled to collider size.
    auto *sprite = player.addComponent<SpriteComponent>();
    sprite->textureName = "tiles";
    sprite->useTextureRect = true;
    sprite->textureRect = sf::IntRect{0, 0, 32, 32};
    sprite->origin = Vec2{16.0f, 16.0f};
    if (config.mode == "diablo") {
        const float d = config.playerRadius * 2.0f;
        sprite->scale = Vec2{d, d};
    } else {
        sprite->scale = Vec2{0.8f, 1.6f}; // world 0.8x1.6 assuming 32px tile
    }
    sprite->z = 0;

    if (inventorySystem)
        inventorySystem->attachToEntity(player);
    if (spellSystem)
        spellSystem->attachToEntity(player);
    player.addComponent<HealthComponent>();

    // Basic camera focus
    cameraManager.setCenter(playerSpawn);

    if (config.mode == "diablo" && spawnSystem) {
        // Arena bounds as static colliders (simple walls).
        if (config.arena.has) {
            const float thickness = std::max(0.2f, config.playerRadius * 2.0f);
            spawnSystem->buildArenaWalls(Vec2{config.arena.minX, config.arena.minY}, Vec2{config.arena.maxX, config.arena.maxY},
                                         thickness);
        }

        // Spawn remaining local players (dummies) up to maxPlayers.
        const std::filesystem::path spawnsPath = std::filesystem::path("config") / config.spawnsFile;
        spawnSystem->loadSpawns(spawnsPath.string(), tileSize);
        const int dummies = std::max(0, config.maxPlayers - 1);
        spawnSystem->spawnDummies(dummies, tileSize, config.playerRadius);
    }
}


