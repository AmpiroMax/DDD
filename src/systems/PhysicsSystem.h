#ifndef DDD_SYSTEMS_PHYSICS_SYSTEM_H
#define DDD_SYSTEMS_PHYSICS_SYSTEM_H

#include "components/PhysicsBodyComponent.h"
#include "core/EntityManager.h"
#include "core/EventBus.h"
#include "core/System.h"
#include "events/PhysicsEvents.h"
#include "managers/PhysicsManager.h"
#include "utils/Constants.h"
#include "utils/CoordinateUtils.h"
#include <box2d/box2d.h>
#include <memory>

class PhysicsSystem : public System {
  public:
    PhysicsSystem(PhysicsManager &physicsManager, EntityManager &entityManager, EventBus &eventBus)
        : physicsManager(physicsManager), entityManager(entityManager), eventBus(eventBus),
          contactListener(eventBus) {
        physicsManager.getWorld().SetContactListener(&contactListener);
    }

    ~PhysicsSystem() override { shutdown(); }

    void shutdown() override { physicsManager.getWorld().SetContactListener(nullptr); }

    void update(float dt) override {
        ensureBodies();
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

                physicsManager.createFixture(*bodyComp->body, bodyComp->fixture, rawTag);
            } else {
                bodyComp->body->SetFixedRotation(!bodyComp->fixture.canRotate);
                bodyComp->body->SetLinearDamping(bodyComp->fixture.linearDamping);
                bodyComp->body->SetAngularDamping(bodyComp->fixture.angularDamping);
            }
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
        }
    }

    PhysicsManager &physicsManager;
    EntityManager &entityManager;
    EventBus &eventBus;
    ContactListener contactListener;
};

#endif // DDD_SYSTEMS_PHYSICS_SYSTEM_H
