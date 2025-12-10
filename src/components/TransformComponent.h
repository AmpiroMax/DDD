#ifndef DDD_COMPONENTS_TRANSFORM_COMPONENT_H
#define DDD_COMPONENTS_TRANSFORM_COMPONENT_H

#include "core/Component.h"
#include "utils/Vec2.h"

struct TransformComponent : Component {
    Vec2 position{0.0f, 0.0f};
    float rotationDeg{0.0f};
    Vec2 scale{1.0f, 1.0f};
};

#endif // DDD_COMPONENTS_TRANSFORM_COMPONENT_H

