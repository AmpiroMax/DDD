#ifndef DDD_COMPONENTS_INPUT_COMPONENT_H
#define DDD_COMPONENTS_INPUT_COMPONENT_H

#include "core/Component.h"
#include "utils/Vec2.h"
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <string>
#include <unordered_map>
#include <vector>

struct ButtonState {
    bool pressed{false};  // true only on the frame the button went down
    bool held{false};     // true while the button stays down
    bool released{false}; // true only on the frame the button went up
};

struct InputComponent : Component {
    std::unordered_map<int, ButtonState> keys;         // sf::Keyboard::Key as int
    std::unordered_map<int, ButtonState> mouseButtons; // sf::Mouse::Button as int
    std::unordered_map<std::string, ButtonState> actions;

    sf::Vector2i mousePixel{0, 0};
    Vec2 mouseWorld{0.0f, 0.0f};
    float mouseWheelDelta{0.0f}; // accumulated wheel delta for the frame
};

#endif // DDD_COMPONENTS_INPUT_COMPONENT_H

