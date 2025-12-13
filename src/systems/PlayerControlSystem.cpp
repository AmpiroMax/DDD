#include "systems/PlayerControlSystem.h"

#include <box2d/box2d.h>
#include "utils/CoordinateUtils.h"

PlayerControlSystem::PlayerControlSystem(InputSystem &inputSys, EntityManager &entityMgr, EventBus &eventBus, float moveSpd,
                                         float jumpImp)
    : inputSystem(inputSys), entityManager(entityMgr), eventBus(eventBus), moveSpeed(moveSpd), jumpImpulse(jumpImp) {
    eventBus.subscribe<GroundedEvent>([this](const GroundedEvent &ev) { onGrounded(ev); });
}

void PlayerControlSystem::onGrounded(const GroundedEvent &ev) {
    if (Entity *ent = entityManager.find(ev.entityId)) {
        if (auto *g = ent->get<GroundedComponent>())
            g->grounded = ev.grounded;
    }
}

void PlayerControlSystem::update(float dt) {
    (void)dt;
    InputComponent *input = inputSystem.getInput();
    if (!input)
        return;

    const auto getAction = [&](const std::string &name) -> const ButtonState * {
        auto it = input->actions.find(name);
        return it != input->actions.end() ? &it->second : nullptr;
    };

    const ButtonState *left = getAction("move_left");
    const ButtonState *right = getAction("move_right");
    const ButtonState *jump = getAction("jump");

    for (auto &entPtr : entityManager.all()) {
        if (!entPtr->has<PlayerTag>())
            continue;

        auto *bodyComp = entPtr->get<PhysicsBodyComponent>();
        if (!bodyComp || !bodyComp->body)
            continue;

        auto *grounded = entPtr->get<GroundedComponent>();
        auto *transform = entPtr->get<TransformComponent>();

        float dir = 0.0f;
        if (left && left->held)
            dir -= 1.0f;
        if (right && right->held)
            dir += 1.0f;

        b2Body *body = bodyComp->body;
        b2Vec2 vel = body->GetLinearVelocity();
        vel.x = dir * moveSpeed;
        body->SetLinearVelocity(vel);

        if (jump && jump->pressed && grounded && grounded->grounded) {
            const float impulse = jumpImpulse * body->GetMass();
            body->ApplyLinearImpulseToCenter(b2Vec2(0.0f, impulse), true);
            grounded->grounded = false;
        }

        // Keep transform in sync for rendering; physics is authoritative.
        if (transform) {
            const b2Vec2 pos = body->GetPosition();
            transform->position = physicsToWorld(Vec2{pos.x, pos.y});
            transform->rotationDeg = physicsAngleToWorld(body->GetAngle());
        }
    }
}

