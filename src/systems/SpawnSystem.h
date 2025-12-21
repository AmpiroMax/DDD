#ifndef DDD_SYSTEMS_SPAWN_SYSTEM_H
#define DDD_SYSTEMS_SPAWN_SYSTEM_H

#include "components/Tags.h"
#include "components/TransformComponent.h"
#include "components/PhysicsBodyComponent.h"
#include "components/GroundedComponent.h"
#include "components/SpriteComponent.h"
#include "components/HealthComponent.h"
#include "core/EntityManager.h"
#include "core/EventBus.h"
#include "core/System.h"
#include "managers/PhysicsManager.h"
#include "managers/ResourceManager.h"
#include "utils/Constants.h"
#include "utils/Vec2.h"
#include <vector>

class SpawnSystem : public System {
  public:
    struct SpawnPoint {
        Vec2 pos{0.0f, 0.0f};
    };

    SpawnSystem(EntityManager &entityMgr, EventBus &eventBus, PhysicsManager &physicsMgr, ResourceManager &resourceMgr);

    void update(float dt) override;

    void loadSpawns(const std::string &path, float tileSize);

    // Spawns dummy players (non-controlled). Returns created entity ids.
    std::vector<Entity::Id> spawnDummies(int count, float tileSize, float playerRadius);

    // Respawn existing entity (teleport + reset velocity)
    void respawnEntity(Entity &ent);

  private:
    Entity &createDummy(const Vec2 &pos, float tileSize, float playerRadius);
    const SpawnPoint &pickSpawn(const std::vector<Vec2> &avoid, float minDistance) const;

    // Arena walls
  public:
    void buildArenaWalls(const Vec2 &minCorner, const Vec2 &maxCorner, float thickness);

    EntityManager &entityManager;
    EventBus &eventBus;
    PhysicsManager &physicsManager;
    ResourceManager &resourceManager;
    std::vector<SpawnPoint> points;
  };

#endif // DDD_SYSTEMS_SPAWN_SYSTEM_H

