#ifndef DDD_PHYSICS_PHYSICS_DEFS_H
#define DDD_PHYSICS_PHYSICS_DEFS_H

#include "utils/Vec2.h"
#include <cstddef>
#include <vector>

enum class PhysicsShapeType { Box, Circle, Polygon };

struct PhysicsFixtureConfig {
    PhysicsShapeType shape{PhysicsShapeType::Box};
    Vec2 size{1.0f, 1.0f};        // full width/height in world units (used for boxes)
    float radius{0.5f};           // radius in world units (used for circles)
    std::vector<Vec2> vertices;   // world-space vertices (used for polygons)

    float density{1.0f};
    float friction{0.3f};
    float restitution{0.0f};
    float linearDamping{0.0f};
    float angularDamping{0.0f};
    bool canRotate{true};
    bool isSensor{false};
    bool isFootSensor{false};
};

struct FixtureTag {
    std::size_t entityId{0};
    bool isSensor{false};
    bool isFootSensor{false};
};

#endif // DDD_PHYSICS_PHYSICS_DEFS_H
