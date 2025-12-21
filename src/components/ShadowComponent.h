#ifndef DDD_COMPONENTS_SHADOW_COMPONENT_H
#define DDD_COMPONENTS_SHADOW_COMPONENT_H

#include "core/Component.h"
#include <SFML/Graphics/Color.hpp>

// Simple ground shadow rendered under an entity sprite.
struct ShadowComponent : Component {
    float radiusX{14.0f}; // pixels in screen space
    float radiusY{7.0f};  // pixels in screen space
    float offsetY{10.0f}; // pixels down from anchor
    sf::Color color{0, 0, 0, 120};
    bool visible{true};
};

#endif // DDD_COMPONENTS_SHADOW_COMPONENT_H

