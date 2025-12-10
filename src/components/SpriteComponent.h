#ifndef DDD_COMPONENTS_SPRITE_COMPONENT_H
#define DDD_COMPONENTS_SPRITE_COMPONENT_H

#include "core/Component.h"
#include "utils/Vec2.h"
#include <SFML/Graphics.hpp>
#include <string>

struct SpriteComponent : Component {
    std::string textureName;
    std::string atlasRegion;
    sf::IntRect textureRect{0, 0, 0, 0};
    Vec2 origin{0.0f, 0.0f};
    Vec2 scale{1.0f, 1.0f};
    int z{0};
    bool visible{true};
    bool useTextureRect{false};
};

#endif // DDD_COMPONENTS_SPRITE_COMPONENT_H

