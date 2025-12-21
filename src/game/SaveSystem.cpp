#include "game/SaveSystem.h"

#include "components/DropComponent.h"
#include "components/InventoryComponent.h"
#include "components/PhysicsBodyComponent.h"
#include "components/SpriteComponent.h"
#include "components/Tags.h"
#include "components/TilemapComponent.h"
#include "components/TransformComponent.h"
#include "core/Entity.h"
#include "core/EntityManager.h"
#include "utils/CoordinateUtils.h"
#include "utils/Vec2.h"

#include <SFML/Graphics/Rect.hpp>
#include <algorithm>
#include <box2d/box2d.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

std::optional<SaveData> SaveSystem::collect(const EntityManager &entityManager, const std::filesystem::path &currentMapPath) {
    SaveData data{};
    if (currentMapPath.empty())
        return std::nullopt;
    data.baseMap = currentMapPath;

    const TilemapComponent *tilemap = nullptr;
    const Entity *player = nullptr;
    for (const auto &entPtr : entityManager.all()) {
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

    for (const auto &entPtr : entityManager.all()) {
        const auto *drop = entPtr->get<DropComponent>();
        const auto *transform = entPtr->get<TransformComponent>();
        if (!drop || !transform)
            continue;
        SaveData::DropInfo info{};
        info.itemId = drop->itemId;
        info.count = drop->count;
        info.px = transform->position.x;
        info.py = transform->position.y;
        if (const auto *body = entPtr->get<PhysicsBodyComponent>(); body && body->body) {
            const b2Vec2 vel = body->body->GetLinearVelocity();
            const Vec2 wv = physicsToWorld(Vec2{vel.x, vel.y});
            info.vx = wv.x;
            info.vy = wv.y;
        }
        data.drops.push_back(info);
    }

    if (const auto *t = player->get<TransformComponent>()) {
        data.player.px = t->position.x;
        data.player.py = t->position.y;
    }
    if (const auto *body = player->get<PhysicsBodyComponent>(); body && body->body) {
        const b2Vec2 vel = body->body->GetLinearVelocity();
        const Vec2 wv = physicsToWorld(Vec2{vel.x, vel.y});
        data.player.vx = wv.x;
        data.player.vy = wv.y;
    }
    if (const auto *inv = player->get<InventoryComponent>()) {
        data.player.activeSlot = inv->activeSlot;
        data.player.slots = inv->slots;
    }

    return data;
}

bool SaveSystem::writeToFile(const SaveData &data, const std::filesystem::path &savePath) {
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
        j["drops"].push_back({{"item_id", d.itemId}, {"count", d.count}, {"pos", {d.px, d.py}}, {"vel", {d.vx, d.vy}}});
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
    return true;
}

std::optional<SaveData> SaveSystem::readFromFile(const std::filesystem::path &savePath,
                                                 const std::filesystem::path &fallbackBaseMap) {
    std::ifstream in(savePath);
    if (!in.is_open())
        return std::nullopt;
    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse save: " << e.what() << "\n";
        return std::nullopt;
    }

    SaveData data{};
    if (j.contains("base_map"))
        data.baseMap = j["base_map"].get<std::string>();
    if (data.baseMap.empty())
        data.baseMap = fallbackBaseMap;

    if (j.contains("placed_tiles")) {
        for (const auto &t : j["placed_tiles"]) {
            data.placed.push_back({t.value("x", 0), t.value("y", 0), t.value("tile_id", -1)});
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

    return data;
}

bool SaveSystem::apply(EntityManager &entityManager, const SaveData &data) {
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


