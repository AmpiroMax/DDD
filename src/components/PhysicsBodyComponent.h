#ifndef DDD_COMPONENTS_PHYSICS_BODY_COMPONENT_H
#define DDD_COMPONENTS_PHYSICS_BODY_COMPONENT_H

#include "core/Component.h"
#include "physics/PhysicsDefs.h"
#include "utils/Vec2.h"
#include <box2d/box2d.h>
#include <memory>
#include <vector>

struct PhysicsBodyComponent : Component {
    b2BodyType bodyType{b2_dynamicBody};
    PhysicsFixtureConfig fixture{};

    Vec2 position{0.0f, 0.0f}; // world space
    float angleDeg{0.0f};      // world space, counter-clockwise

    b2Body *body{nullptr};
    bool pendingDestroy{false};

    // Owns user data that is attached to Box2D fixtures.
    std::vector<std::unique_ptr<FixtureTag>> fixtureTags;
};

#endif // DDD_COMPONENTS_PHYSICS_BODY_COMPONENT_H
