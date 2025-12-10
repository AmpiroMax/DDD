#include "systems/InputSystem.h"

#include "utils/CoordinateUtils.h"
#include <cctype>
#include <fstream>
#include <iostream>
#include <string_view>

namespace {
ButtonState &getOrMake(std::unordered_map<int, ButtonState> &map, int key) { return map[key]; }

void markPressed(ButtonState &state) {
    if (!state.held) {
        state.pressed = true;
    }
    state.held = true;
    state.released = false;
}

void markReleased(ButtonState &state) {
    if (state.held) {
        state.released = true;
    }
    state.held = false;
    state.pressed = false;
}
} // namespace

InputSystem::InputSystem(WindowManager &windowMgr, CameraManager &cameraMgr, EntityManager &entityMgr)
    : windowManager(windowMgr), cameraManager(cameraMgr), entityManager(entityMgr) {
    // Create a dedicated input entity.
    Entity &ent = entityManager.create();
    input = ent.addComponent<InputComponent>();
}

void InputSystem::resetFrameStates() {
    if (!input)
        return;

    for (auto &[_, state] : input->keys) {
        state.pressed = false;
        state.released = false;
    }
    for (auto &[_, state] : input->mouseButtons) {
        state.pressed = false;
        state.released = false;
    }
    for (auto &[_, state] : input->actions) {
        state.pressed = false;
        state.released = false;
    }
}

void InputSystem::handleEvent(const sf::Event &evt) {
    if (!input)
        return;

    switch (evt.type) {
    case sf::Event::KeyPressed: {
        ButtonState &state = getOrMake(input->keys, static_cast<int>(evt.key.code));
        markPressed(state);
        break;
    }
    case sf::Event::KeyReleased: {
        ButtonState &state = getOrMake(input->keys, static_cast<int>(evt.key.code));
        markReleased(state);
        break;
    }
    case sf::Event::MouseButtonPressed: {
        ButtonState &state = getOrMake(input->mouseButtons, static_cast<int>(evt.mouseButton.button));
        markPressed(state);
        break;
    }
    case sf::Event::MouseButtonReleased: {
        ButtonState &state = getOrMake(input->mouseButtons, static_cast<int>(evt.mouseButton.button));
        markReleased(state);
        break;
    }
    default:
        break;
    }
}

void InputSystem::beginFrame() { resetFrameStates(); }

void InputSystem::update(float dt) {
    (void)dt;
    if (!input)
        return;

    updateMouse();
    refreshActions();
}

void InputSystem::updateMouse() {
    if (!input)
        return;

    sf::RenderWindow &window = windowManager.getWindow();
    input->mousePixel = sf::Mouse::getPosition(window);
    const sf::Vector2f renderPos = window.mapPixelToCoords(input->mousePixel, windowManager.getView());
    input->mouseWorld = renderToWorld(Vec2{renderPos.x, renderPos.y});
}

void InputSystem::refreshActions() {
    if (!input)
        return;

    for (const auto &[actionName, binding] : bindings) {
        ButtonState state{};

        for (auto key : binding.keys) {
            const auto it = input->keys.find(static_cast<int>(key));
            if (it == input->keys.end())
                continue;
            state.pressed = state.pressed || it->second.pressed;
            state.released = state.released || it->second.released;
            state.held = state.held || it->second.held;
        }

        for (auto btn : binding.mouseButtons) {
            const auto it = input->mouseButtons.find(static_cast<int>(btn));
            if (it == input->mouseButtons.end())
                continue;
            state.pressed = state.pressed || it->second.pressed;
            state.released = state.released || it->second.released;
            state.held = state.held || it->second.held;
        }

        input->actions[actionName] = state;
    }
}

static std::string toUpper(std::string s) {
    for (auto &c : s)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

sf::Keyboard::Key InputSystem::parseKey(const std::string &name) {
    const std::string n = toUpper(name);
    using K = sf::Keyboard::Key;
    if (n == "A")
        return K::A;
    if (n == "B")
        return K::B;
    if (n == "C")
        return K::C;
    if (n == "D")
        return K::D;
    if (n == "S")
        return K::S;
    if (n == "W")
        return K::W;
    if (n == "SPACE")
        return K::Space;
    if (n == "LEFT")
        return K::Left;
    if (n == "RIGHT")
        return K::Right;
    if (n == "UP")
        return K::Up;
    if (n == "DOWN")
        return K::Down;
    if (n == "LCTRL" || n == "CTRL" || n == "CONTROL")
        return K::LControl;
    if (n == "LSHIFT" || n == "SHIFT")
        return K::LShift;
    return K::Unknown;
}

sf::Mouse::Button InputSystem::parseMouseButton(const std::string &name) {
    const std::string n = toUpper(name);
    using B = sf::Mouse::Button;
    if (n == "LEFT" || n == "MOUSELEFT" || n == "LMB")
        return B::Left;
    if (n == "RIGHT" || n == "MOUSERIGHT" || n == "RMB")
        return B::Right;
    if (n == "MIDDLE" || n == "MIDDLEMOUSE")
        return B::Middle;
    return B::ButtonCount;
}

void InputSystem::loadBindingsFromFile(const std::string &path) {
    bindings.clear();

    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "Failed to open input config: " << path << ", using defaults\n";
    } else {
        try {
            nlohmann::json j;
            in >> j;
            if (j.contains("actions")) {
                for (auto &[name, val] : j["actions"].items()) {
                    ActionBinding binding;
                    if (val.contains("keys")) {
                        for (const auto &k : val["keys"]) {
                            if (!k.is_string())
                                continue;
                            const auto key = parseKey(k.get<std::string>());
                            if (key != sf::Keyboard::Unknown)
                                binding.keys.push_back(key);
                        }
                    }
                    if (val.contains("mouse")) {
                        for (const auto &btn : val["mouse"]) {
                            if (!btn.is_string())
                                continue;
                            const auto mb = parseMouseButton(btn.get<std::string>());
                            if (mb != sf::Mouse::ButtonCount)
                                binding.mouseButtons.push_back(mb);
                        }
                    }
                    bindings[name] = binding;
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "Failed to parse input config: " << e.what() << "\n";
        }
    }

    // Provide reasonable defaults if nothing loaded.
    if (bindings.empty()) {
        bindings["move_left"] = ActionBinding{{sf::Keyboard::A, sf::Keyboard::Left}, {}};
        bindings["move_right"] = ActionBinding{{sf::Keyboard::D, sf::Keyboard::Right}, {}};
        bindings["move_up"] = ActionBinding{{sf::Keyboard::W}, {}};
        bindings["move_down"] = ActionBinding{{sf::Keyboard::S}, {}};
        bindings["jump"] = ActionBinding{{sf::Keyboard::Space}, {}};
        bindings["break_block"] = ActionBinding{{}, {sf::Mouse::Left}};
        bindings["place_block"] = ActionBinding{{}, {sf::Mouse::Right}};
    }
}

