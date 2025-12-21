#ifndef DDD_COMPONENTS_SPRITE_ANIMATION_COMPONENT_H
#define DDD_COMPONENTS_SPRITE_ANIMATION_COMPONENT_H

#include "core/Component.h"
#include <string>
#include <vector>

// Minimal sprite animation driven by movement speed.
// Frames refer to atlas region names (ResourceManager).
struct SpriteAnimationComponent : Component {
    std::vector<std::string> idleFrames;
    std::vector<std::string> runFrames;
    float fps{8.0f};
    float runSpeedThreshold{0.15f}; // world units per second

    // Runtime state
    float time{0.0f};
    bool running{false};
};

#endif // DDD_COMPONENTS_SPRITE_ANIMATION_COMPONENT_H

