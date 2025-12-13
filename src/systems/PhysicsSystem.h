#ifndef DDD_SYSTEMS_PHYSICS_SYSTEM_H
#define DDD_SYSTEMS_PHYSICS_SYSTEM_H

#include "components/DropComponent.h"
#include "components/PhysicsBodyComponent.h"
#include "components/SpriteComponent.h"
#include "components/TilemapComponent.h"
#include "components/TransformComponent.h"
#include "core/Entity.h"
#include "core/EntityManager.h"
#include "core/EventBus.h"
#include "core/System.h"
#include "events/PhysicsEvents.h"
#include "events/TileEvents.h"
#include "managers/PhysicsManager.h"
#include "utils/Constants.h"
#include "utils/CoordinateUtils.h"
#include <SFML/Graphics/Rect.hpp>
#include <box2d/box2d.h>
#include <memory>
#include <unordered_map>
#include <utility>

class PhysicsSystem : public System {
  public:
    PhysicsSystem(PhysicsManager &physicsManager, EntityManager &entityManager, EventBus &eventBus)
        : physicsManager(physicsManager), entityManager(entityManager), eventBus(eventBus),
          contactListener(eventBus) {
        physicsManager.getWorld().SetContactListener(&contactListener);

        eventBus.subscribe<PlaceBlockEvent>([this](const PlaceBlockEvent &ev) { handlePlace(ev); });
        eventBus.subscribe<BreakBlockEvent>([this](const BreakBlockEvent &ev) { handleBreak(ev); });
    }

    ~PhysicsSystem() override { shutdown(); }

    void shutdown() override { physicsManager.getWorld().SetContactListener(nullptr); }

    void reset() {
        shutdown();
        clearTilemapColliders();
        tilemapInitialized = false;
        tilemapEntityId = kInvalidEntityId;
        mapOwnerId = kInvalidEntityId;
    }

    void update(float dt) override {
        // Re-attach listener after potential world reset.
        physicsManager.getWorld().SetContactListener(&contactListener);

        ensureBodies();
        ensureTilemapColliders();
        physicsManager.getWorld().Step(dt, PHYSICS_VELOCITY_ITER, PHYSICS_POSITION_ITER);
        syncTransforms();
    }

  private:
    class ContactListener : public b2ContactListener {
      public:
        explicit ContactListener(EventBus &bus) : eventBus(bus) {}

        void BeginContact(b2Contact *contact) override { handle(contact, true); }
        void EndContact(b2Contact *contact) override { handle(contact, false); }

      private:
        static FixtureTag *getTag(const b2Fixture *fixture) {
            if (!fixture)
                return nullptr;
            return reinterpret_cast<FixtureTag *>(fixture->GetUserData().pointer);
        }

        void handle(b2Contact *contact, bool isBegin) {
            auto *tagA = getTag(contact->GetFixtureA());
            auto *tagB = getTag(contact->GetFixtureB());
            if (!tagA || !tagB)
                return;

            eventBus.emit(ContactEvent{tagA->entityId, tagB->entityId, isBegin});

            if (tagA->isFootSensor)
                eventBus.emit(GroundedEvent{tagA->entityId, isBegin});
            if (tagB->isFootSensor)
                eventBus.emit(GroundedEvent{tagB->entityId, isBegin});
        }

        EventBus &eventBus;
    };

    void ensureBodies() {
        for (auto &entPtr : entityManager.all()) {
            auto *bodyComp = entPtr->get<PhysicsBodyComponent>();
            auto *dropComp = entPtr->get<DropComponent>();
            if (!bodyComp)
                continue;

            if (bodyComp->pendingDestroy && bodyComp->body) {
                physicsManager.destroyBody(bodyComp->body);
                bodyComp->body = nullptr;
                bodyComp->fixtureTags.clear();
                bodyComp->pendingDestroy = false;
                continue;
            }

            if (!bodyComp->body) {
                bodyComp->body = physicsManager.createBody(bodyComp->bodyType, bodyComp->position, bodyComp->angleDeg,
                                                           bodyComp->fixture.canRotate, bodyComp->fixture.linearDamping,
                                                           bodyComp->fixture.angularDamping);

                bodyComp->fixtureTags.clear();
                auto tag = std::make_unique<FixtureTag>();
                tag->entityId = entPtr->getId();
                tag->isSensor = bodyComp->fixture.isSensor;
                tag->isFootSensor = bodyComp->fixture.isFootSensor;
                FixtureTag *rawTag = tag.get();
                bodyComp->fixtureTags.push_back(std::move(tag));

                b2Fixture *fixture = physicsManager.createFixture(*bodyComp->body, bodyComp->fixture, rawTag);
                if (dropComp && fixture) {
                b2Filter filter = fixture->GetFilterData();
                filter.categoryBits = 0x0002;
                filter.maskBits = 0xFFFF; // collide with everything, включая другие дропы
                fixture->SetFilterData(filter);
                bodyComp->body->SetSleepingAllowed(false);
                bodyComp->body->SetAwake(true);
                bodyComp->body->SetBullet(true);
                bodyComp->body->SetGravityScale(1.0f);
                }
            } else {
                bodyComp->body->SetFixedRotation(!bodyComp->fixture.canRotate);
                bodyComp->body->SetLinearDamping(bodyComp->fixture.linearDamping);
                bodyComp->body->SetAngularDamping(bodyComp->fixture.angularDamping);

                if (dropComp) {
                    for (b2Fixture *f = bodyComp->body->GetFixtureList(); f; f = f->GetNext()) {
                        b2Filter filter = f->GetFilterData();
                        filter.categoryBits = 0x0002;
                        filter.maskBits = 0xFFFF;
                        f->SetFilterData(filter);
                    }
                bodyComp->body->SetSleepingAllowed(false);
                bodyComp->body->SetAwake(true);
                bodyComp->body->SetBullet(true);
                bodyComp->body->SetGravityScale(1.0f);
                    bodyComp->body->SetSleepingAllowed(false);
                }
            }
        }
    }

    void ensureTilemapColliders() {
        TilemapComponent *map = findTilemap();
        if (!map) {
            clearTilemapColliders();
            tilemapInitialized = false;
            mapOwnerId = kInvalidEntityId;
            return;
        }

        if (!tilemapInitialized || tilemapEntityId != mapOwnerId) {
            rebuildTilemapColliders(*map);
            tilemapInitialized = true;
        }
    }

    TilemapComponent *findTilemap() {
        // If we already cached and still valid, return.
        if (tilemapEntityId != kInvalidEntityId) {
            if (Entity *e = entityManager.find(tilemapEntityId)) {
                return e->get<TilemapComponent>();
            }
        }

        for (auto &entPtr : entityManager.all()) {
            if (auto *map = entPtr->get<TilemapComponent>()) {
                tilemapEntityId = entPtr->getId();
                return map;
            }
        }
        tilemapEntityId = kInvalidEntityId;
        return nullptr;
    }

    void rebuildTilemapColliders(TilemapComponent &map) {
        clearTilemapColliders();
        mapOwnerId = tilemapEntityId;
        for (int y = 0; y < map.height; ++y) {
            for (int x = 0; x < map.width; ++x) {
                const int tileId = map.get(x, y);
                createTileBody(map, x, y, tileId);
            }
        }
    }

    void clearTilemapColliders() {
        for (auto &entry : tileBodies) {
            if (entry.second.body)
                physicsManager.destroyBody(entry.second.body);
        }
        tileBodies.clear();
    }

    void handlePlace(const PlaceBlockEvent &ev) {
        if (TilemapComponent *map = findTilemap()) {
            if (mapOwnerId == kInvalidEntityId)
                mapOwnerId = tilemapEntityId;
            createTileBody(*map, ev.x, ev.y, ev.tileId, /*replace=*/true);
        }
    }

    void handleBreak(const BreakBlockEvent &ev) {
        auto it = tileBodies.find({ev.x, ev.y});
        if (it != tileBodies.end()) {
            if (it->second.body)
                physicsManager.destroyBody(it->second.body);
            tileBodies.erase(it);
        }

        TilemapComponent *map = findTilemap();
        if (!map)
            return;

        const int tileId = ev.previousTileId;
        if (tileId == map->emptyId)
            return;

        wakeBodiesAroundTile(*map, ev.x, ev.y);
        spawnDrop(*map, ev.x, ev.y, tileId);
    }

    void createTileBody(TilemapComponent &map, int x, int y, int tileId, bool replace = false) {
        if (!map.inBounds(x, y))
            return;

        const auto key = std::make_pair(x, y);
        auto it = tileBodies.find(key);
        if (it != tileBodies.end()) {
            if (!replace)
                return;
            if (it->second.body)
                physicsManager.destroyBody(it->second.body);
            tileBodies.erase(it);
        }

        if (!map.isSolid(tileId))
            return;

        Vec2 center{map.origin.x + (static_cast<float>(x) + 0.5f) * map.tileSize,
                    map.origin.y - (static_cast<float>(y) + 0.5f) * map.tileSize};

        PhysicsFixtureConfig cfg;
        cfg.shape = PhysicsShapeType::Box;
        cfg.size = Vec2{map.tileSize, map.tileSize};
        cfg.density = 0.0f;
        cfg.friction = 0.6f;
        cfg.restitution = 0.0f;
        cfg.canRotate = false;
        cfg.isSensor = false;

        TileBody bodyInfo;
        bodyInfo.tag = std::make_unique<FixtureTag>();
        bodyInfo.tag->entityId = mapOwnerId == kInvalidEntityId ? 0 : mapOwnerId;
        bodyInfo.tag->isSensor = false;
        bodyInfo.tag->isFootSensor = false;

        bodyInfo.body =
            physicsManager.createBody(b2_staticBody, center, 0.0f, false, 0.0f, 0.0f);
        physicsManager.createFixture(*bodyInfo.body, cfg, bodyInfo.tag.get());

        tileBodies.emplace(key, std::move(bodyInfo));
    }

    void wakeBodiesAroundTile(const TilemapComponent &map, int x, int y) {
        Vec2 center{map.origin.x + (static_cast<float>(x) + 0.5f) * map.tileSize,
                    map.origin.y - (static_cast<float>(y) + 0.5f) * map.tileSize};
        const float half = map.tileSize * 0.5f;
        const float margin = map.tileSize * 0.1f;
        Vec2 wmin{center.x - half - margin, center.y - half - margin};
        Vec2 wmax{center.x + half + margin, center.y + half + margin};

        b2AABB aabb;
        b2Vec2 pmin = worldToPhysics(wmin);
        b2Vec2 pmax = worldToPhysics(wmax);
        aabb.lowerBound = {std::min(pmin.x, pmax.x), std::min(pmin.y, pmax.y)};
        aabb.upperBound = {std::max(pmin.x, pmax.x), std::max(pmin.y, pmax.y)};

        struct WakeQuery : b2QueryCallback {
            bool ReportFixture(b2Fixture *fixture) override {
                b2Body *b = fixture->GetBody();
                if (b && b->GetType() != b2_staticBody)
                    b->SetAwake(true);
                return true;
            }
        };

        WakeQuery query;
        physicsManager.getWorld().QueryAABB(&query, aabb);
    }

    void spawnDrop(const TilemapComponent &map, int x, int y, int tileId) {
        Vec2 center{map.origin.x + (static_cast<float>(x) + 0.5f) * map.tileSize,
                    map.origin.y - (static_cast<float>(y) + 0.5f) * map.tileSize};

        Entity &drop = entityManager.create();

        auto *transform = drop.addComponent<TransformComponent>();
        transform->position = center + Vec2{0.0f, 0.1f * map.tileSize};
        transform->scale = {1.0f, 1.0f};

        auto *body = drop.addComponent<PhysicsBodyComponent>();
        body->bodyType = b2_dynamicBody;
        body->position = transform->position;
        body->fixture.shape = PhysicsShapeType::Box;
        const float dropSize = map.tileSize / 3.0f;
        body->fixture.size = Vec2{dropSize, dropSize};
        body->fixture.density = 1.0f;
        body->fixture.friction = 0.8f;
        body->fixture.restitution = 0.0f;
        body->fixture.linearDamping = 0.05f;  // allow falling
        body->fixture.angularDamping = 0.1f;  // slight damping
        body->fixture.canRotate = true;
        body->fixture.isSensor = false;

        auto *dropComp = drop.addComponent<DropComponent>();
        dropComp->itemId = tileId;
        dropComp->count = 1;

        auto *sprite = drop.addComponent<SpriteComponent>();
        sprite->textureName = "tiles";
        sprite->useTextureRect = true;
        sprite->textureRect = sf::IntRect{0, 0, 32, 32};
        sprite->origin = Vec2{16.0f, 16.0f};
        const float dropScale = map.tileSize / 3.0f; // render drops at 1/3 tile size
        sprite->scale = Vec2{dropScale, dropScale};

        // Optionally reuse atlas region if provided.
        auto itRegion = map.tileIdToRegion.find(tileId);
        if (itRegion != map.tileIdToRegion.end()) {
            sprite->atlasRegion = itRegion->second;
            sprite->useTextureRect = false;
        }

        // Apply initial impulse downward to avoid resting in air if spawned without contacts.
        if (body->body) {
            b2Vec2 impulse{0.0f, -2.0f * dropSize};
            body->body->ApplyLinearImpulseToCenter(impulse, true);
            body->body->SetSleepingAllowed(false);
            body->body->SetAwake(true);
            body->body->SetBullet(true);
        }
    }

    void syncTransforms() {
        for (auto &entPtr : entityManager.all()) {
            auto *bodyComp = entPtr->get<PhysicsBodyComponent>();
            if (!bodyComp || !bodyComp->body)
                continue;

            const b2Vec2 pos = bodyComp->body->GetPosition();
            bodyComp->position = physicsToWorld(Vec2{pos.x, pos.y});
            bodyComp->angleDeg = physicsAngleToWorld(bodyComp->body->GetAngle());

            if (auto *transform = entPtr->get<TransformComponent>()) {
                transform->position = bodyComp->position;
                transform->rotationDeg = bodyComp->angleDeg;
            }
        }
    }

    PhysicsManager &physicsManager;
    EntityManager &entityManager;
    EventBus &eventBus;
    ContactListener contactListener;

    struct TileBody {
        b2Body *body{nullptr};
        std::unique_ptr<FixtureTag> tag;
    };

    struct PairHash {
        std::size_t operator()(const std::pair<int, int> &p) const noexcept {
            return (static_cast<std::size_t>(static_cast<uint32_t>(p.first)) << 32) ^
                   static_cast<uint32_t>(p.second);
        }
    };

    std::unordered_map<std::pair<int, int>, TileBody, PairHash> tileBodies;
    bool tilemapInitialized{false};
    Entity::Id tilemapEntityId{static_cast<Entity::Id>(-1)};
    Entity::Id mapOwnerId{static_cast<Entity::Id>(-1)};
    static constexpr Entity::Id kInvalidEntityId = static_cast<Entity::Id>(-1);
};

#endif // DDD_SYSTEMS_PHYSICS_SYSTEM_H
