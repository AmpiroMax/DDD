#ifndef DDD_SYSTEMS_PLAYER_CONTROL_SYSTEM_H
#define DDD_SYSTEMS_PLAYER_CONTROL_SYSTEM_H

#include "components/GroundedComponent.h"
#include "components/InputComponent.h"
#include "components/PhysicsBodyComponent.h"
#include "components/Tags.h"
#include "components/TransformComponent.h"
#include "core/EntityManager.h"
#include "core/EventBus.h"
#include "core/System.h"
#include "events/PhysicsEvents.h"
#include "systems/InputSystem.h"
#include <unordered_set>

class PlayerControlSystem : public System {
  public:
    PlayerControlSystem(InputSystem &inputSystem, EntityManager &entityManager, EventBus &eventBus, float moveSpeed,
                        float jumpImpulse);
    ~PlayerControlSystem() override { shutdown(); }

    void update(float dt) override;
    void shutdown() override;

  private:
    void onGrounded(const GroundedEvent &ev);

    InputSystem &inputSystem;
    EntityManager &entityManager;
    EventBus &eventBus;
    EventBus::SubscriptionToken groundedSub_{};
    float moveSpeed{6.0f};
    float jumpImpulse{8.0f};
};

#endif // DDD_SYSTEMS_PLAYER_CONTROL_SYSTEM_H

