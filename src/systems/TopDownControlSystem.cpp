#include "systems/TopDownControlSystem.h"

#include <box2d/box2d.h>
#include <cmath>

namespace {
float length(const b2Vec2 &v) { return std::sqrt(v.x * v.x + v.y * v.y); }

b2Vec2 normalized(const b2Vec2 &v) {
    const float len = length(v);
    if (len <= 0.000001f)
        return b2Vec2_zero;
    return b2Vec2{v.x / len, v.y / len};
}

b2Vec2 clampLen(const b2Vec2 &v, float maxLen) {
    const float len = length(v);
    if (len <= maxLen)
        return v;
    const b2Vec2 n = normalized(v);
    return b2Vec2{n.x * maxLen, n.y * maxLen};
}
} // namespace

TopDownControlSystem::TopDownControlSystem(InputSystem &inputSys, EntityManager &entityMgr, Params p)
    : inputSystem(inputSys), entityManager(entityMgr), params(p) {}

void TopDownControlSystem::update(float dt) {
    if (dt <= 0.0f)
        return;

    InputComponent *input = inputSystem.getInput();
    if (!input)
        return;

    // Click-to-move: left mouse sets target.
    const auto itMb = input->mouseButtons.find(static_cast<int>(sf::Mouse::Left));
    if (itMb != input->mouseButtons.end() && itMb->second.pressed) {
        clickTarget = input->mouseWorld;
        hasClickTarget = true;
    }

    const auto held = [&](const std::string &name) -> bool {
        const auto it = input->actions.find(name);
        return it != input->actions.end() && it->second.held;
    };

    b2Vec2 move{0.0f, 0.0f};
    if (held("move_left"))
        move.x -= 1.0f;
    if (held("move_right"))
        move.x += 1.0f;
    if (held("move_up"))
        move.y += 1.0f;
    if (held("move_down"))
        move.y -= 1.0f;

    const float inputLen = length(move);
    const bool hasInput = inputLen > params.inputDeadzone;
    const b2Vec2 inputDir = hasInput ? b2Vec2{move.x / inputLen, move.y / inputLen} : b2Vec2_zero;

    for (auto &entPtr : entityManager.all()) {
        if (!entPtr->has<PlayerTag>())
            continue;

        auto *bodyComp = entPtr->get<PhysicsBodyComponent>();
        if (!bodyComp || !bodyComp->body)
            continue;

        b2Body *body = bodyComp->body;
        body->SetLinearDamping(params.linearDamping);

        b2Vec2 desiredVel = b2Vec2_zero;
        bool wantAccel = false;
        if (hasClickTarget) {
            const b2Vec2 pos = body->GetPosition();
            const b2Vec2 toTarget{clickTarget.x - pos.x, clickTarget.y - pos.y};
            const float dist = length(toTarget);
            if (dist <= params.arriveRadius) {
                hasClickTarget = false;
                desiredVel = b2Vec2_zero;
            } else {
                const b2Vec2 dir = b2Vec2{toTarget.x / dist, toTarget.y / dist};
                desiredVel = b2Vec2{dir.x * params.maxSpeed, dir.y * params.maxSpeed};
                wantAccel = true;
            }
        } else if (hasInput) {
            desiredVel = b2Vec2{inputDir.x * params.maxSpeed, inputDir.y * params.maxSpeed};
            wantAccel = true;
        } else {
            desiredVel = b2Vec2_zero;
        }

        const b2Vec2 v = body->GetLinearVelocity();
        const b2Vec2 dv = b2Vec2{desiredVel.x - v.x, desiredVel.y - v.y};

        // Acceleration clamp:
        // - with input: limit by accel
        // - without input: limit by decel (braking)
        const float maxDelta = (wantAccel ? params.accel : params.decel) * dt;
        const b2Vec2 dvClamped = clampLen(dv, maxDelta);

        const float mass = body->GetMass();
        const b2Vec2 impulse{mass * dvClamped.x, mass * dvClamped.y};
        body->ApplyLinearImpulseToCenter(impulse, true);
    }
}

