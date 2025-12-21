#include "systems/SpawnSystem.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <random>

SpawnSystem::SpawnSystem(EntityManager &entityMgr, EventBus &evtBus, PhysicsManager &physMgr, ResourceManager &resMgr)
    : entityManager(entityMgr), eventBus(evtBus), physicsManager(physMgr), resourceManager(resMgr) {}

void SpawnSystem::update(float dt) { (void)dt; }

void SpawnSystem::loadSpawns(const std::string &path, float tileSize) {
    points.clear();
    std::ifstream in(path);
    if (!in.is_open())
        return;
    try {
        nlohmann::json j;
        in >> j;
        if (j.contains("spawns") && j["spawns"].is_array()) {
            for (const auto &s : j["spawns"]) {
                if (!s.is_array() || s.size() < 2)
                    continue;
                SpawnPoint p;
                p.pos.x = s[0].get<float>() * tileSize;
                p.pos.y = s[1].get<float>() * tileSize;
                points.push_back(p);
            }
        }
    } catch (...) {
    }
}

std::vector<Entity::Id> SpawnSystem::spawnDummies(int count, float tileSize, float playerRadius) {
    std::vector<Entity::Id> ids;
    if (points.empty())
        return ids;
    std::vector<Vec2> avoid;
    for (int i = 0; i < count; ++i) {
        const auto &pt = pickSpawn(avoid, tileSize * 1.5f);
        Entity &ent = createDummy(pt.pos, tileSize, playerRadius);
        ids.push_back(ent.getId());
        avoid.push_back(pt.pos);
    }
    return ids;
}

void SpawnSystem::respawnEntity(Entity &ent) {
    if (points.empty())
        return;
    std::vector<Vec2> avoid;
    for (auto &e : entityManager.all()) {
        if (e.get() == &ent)
            continue;
        if (auto *tr = e->get<TransformComponent>())
            avoid.push_back(tr->position);
    }
    const auto &pt = pickSpawn(avoid, 0.5f);
    if (auto *tr = ent.get<TransformComponent>()) {
        tr->position = pt.pos;
    }
    if (auto *body = ent.get<PhysicsBodyComponent>(); body && body->body) {
        body->body->SetTransform(worldToPhysics(pt.pos), body->body->GetAngle());
        body->body->SetLinearVelocity(b2Vec2_zero);
        body->body->SetAngularVelocity(0.0f);
        body->body->SetAwake(true);
    }
}

Entity &SpawnSystem::createDummy(const Vec2 &pos, float tileSize, float playerRadius) {
    Entity &ent = entityManager.create();
    ent.addComponent<DummyPlayerTag>();
    ent.addComponent<HealthComponent>();
    auto *tr = ent.addComponent<TransformComponent>();
    tr->position = pos;
    auto *body = ent.addComponent<PhysicsBodyComponent>();
    body->bodyType = b2_dynamicBody;
    body->position = pos;
    body->fixture.shape = PhysicsShapeType::Circle;
    body->fixture.radius = playerRadius;
    body->fixture.density = 1.0f;
    body->fixture.friction = 0.0f;
    body->fixture.restitution = 0.0f;
    body->fixture.linearDamping = 6.0f;
    body->fixture.canRotate = false;

    // Top-down; grounded is not used.

    auto *sprite = ent.addComponent<SpriteComponent>();
    sprite->textureName = "tiles";
    sprite->atlasRegion = "ground";
    sprite->origin = Vec2{16.0f, 16.0f};
    const float d = playerRadius * 2.0f;
    sprite->scale = Vec2{d, d};
    sprite->z = 0;

    return ent;
}

const SpawnSystem::SpawnPoint &SpawnSystem::pickSpawn(const std::vector<Vec2> &avoid, float minDistance) const {
    if (points.size() == 1)
        return points.front();
    const float minDist2 = minDistance * minDistance;
    for (const auto &p : points) {
        bool ok = true;
        for (const auto &a : avoid) {
            const float dx = p.pos.x - a.x;
            const float dy = p.pos.y - a.y;
            if (dx * dx + dy * dy < minDist2) {
                ok = false;
                break;
            }
        }
        if (ok)
            return p;
    }
    // fallback: farthest from avoid
    const SpawnPoint *best = &points.front();
    float bestDist = -1.0f;
    for (const auto &p : points) {
        float dmin = 0.0f;
        for (const auto &a : avoid) {
            const float dx = p.pos.x - a.x;
            const float dy = p.pos.y - a.y;
            const float d2 = dx * dx + dy * dy;
            dmin = std::max(dmin, d2);
        }
        if (dmin > bestDist) {
            bestDist = dmin;
            best = &p;
        }
    }
    return *best;
}

void SpawnSystem::buildArenaWalls(const Vec2 &minCorner, const Vec2 &maxCorner, float thickness) {
    const float w = maxCorner.x - minCorner.x;
    const float h = maxCorner.y - minCorner.y;
    const Vec2 center{(minCorner.x + maxCorner.x) * 0.5f, (minCorner.y + maxCorner.y) * 0.5f};

    auto makeWall = [&](const Vec2 &pos, const Vec2 &size) {
        Entity &e = entityManager.create();
        auto *tr = e.addComponent<TransformComponent>();
        tr->position = pos;
        auto *pb = e.addComponent<PhysicsBodyComponent>();
        pb->bodyType = b2_staticBody;
        pb->position = pos;
        pb->fixture.shape = PhysicsShapeType::Box;
        pb->fixture.size = size;
        pb->fixture.density = 0.0f;
        pb->fixture.friction = 0.0f;
        pb->fixture.restitution = 0.0f;
        pb->fixture.canRotate = false;
    };

    // Top/bottom
    makeWall(Vec2{center.x, maxCorner.y + thickness * 0.5f}, Vec2{w + thickness * 2.0f, thickness});
    makeWall(Vec2{center.x, minCorner.y - thickness * 0.5f}, Vec2{w + thickness * 2.0f, thickness});
    // Left/right
    makeWall(Vec2{minCorner.x - thickness * 0.5f, center.y}, Vec2{thickness, h});
    makeWall(Vec2{maxCorner.x + thickness * 0.5f, center.y}, Vec2{thickness, h});
}


