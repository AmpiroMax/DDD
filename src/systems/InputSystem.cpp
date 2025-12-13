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
    resetInputEntity();
}

void InputSystem::resetInputEntity() {
    input = nullptr;
    // Create a dedicated input entity.
    Entity &ent = entityManager.create();
    input = ent.addComponent<InputComponent>();
}

void InputSystem::resetFrameStates() {
    if (!input)
        return;

    wheelDelta = 0;
    input->mouseWheelDelta = 0.0f;

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
    case sf::Event::MouseWheelScrolled: {
        wheelDelta += static_cast<int>(evt.mouseWheelScroll.delta);
        input->mouseWheelDelta += evt.mouseWheelScroll.delta;
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

        for (auto dir : binding.wheelDirections) {
            if (dir > 0 && wheelDelta > 0)
                state.pressed = true;
            if (dir < 0 && wheelDelta < 0)
                state.pressed = true;
        }

        input->actions[actionName] = state;
    }

    // Mirror slot_* actions to inventory_* aliases to keep consumers simple.
    const auto mirror = [this](const std::string &dst, const std::string &src) {
        auto it = input->actions.find(src);
        if (it != input->actions.end())
            input->actions[dst] = it->second;
    };
    mirror("inventory_prev", "slot_prev");
    mirror("inventory_next", "slot_next");
    for (int i = 1; i <= 10; ++i) {
        const std::string suffix = std::to_string(i);
        const std::string src = "slot_" + suffix;
        const std::string dst = "inventory_slot_" + suffix;
        mirror(dst, src);
    }
}

static std::string toUpper(std::string s) {
    for (auto &c : s)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

static int parseWheelDirection(const std::string &name) {
    const std::string n = toUpper(name);
    if (n == "UP" || n == "WHEELUP" || n == "SCROLLUP")
        return 1;
    if (n == "DOWN" || n == "WHEELDOWN" || n == "SCROLLDOWN")
        return -1;
    return 0;
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
    if (n == "E")
        return K::E;
    if (n == "Q")
        return K::Q;
    if (n == "S")
        return K::S;
    if (n == "W")
        return K::W;
    if (n == "ONE" || n == "1")
        return K::Num1;
    if (n == "TWO" || n == "2")
        return K::Num2;
    if (n == "THREE" || n == "3")
        return K::Num3;
    if (n == "FOUR" || n == "4")
        return K::Num4;
    if (n == "FIVE" || n == "5")
        return K::Num5;
    if (n == "SIX" || n == "6")
        return K::Num6;
    if (n == "SEVEN" || n == "7")
        return K::Num7;
    if (n == "EIGHT" || n == "8")
        return K::Num8;
    if (n == "NINE" || n == "9")
        return K::Num9;
    if (n == "ZERO" || n == "0")
        return K::Num0;
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
    if (n == "TILDE" || n == "GRAVE")
        return K::Tilde;
    if (n == "ESC" || n == "ESCAPE")
        return K::Escape;
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
                    if (val.contains("wheel")) {
                        for (const auto &w : val["wheel"]) {
                            if (!w.is_string())
                                continue;
                            const int dir = parseWheelDirection(w.get<std::string>());
                            if (dir != 0)
                                binding.wheelDirections.push_back(dir);
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
        bindings["slot_1"] = ActionBinding{{sf::Keyboard::Num1}, {}};
        bindings["slot_2"] = ActionBinding{{sf::Keyboard::Num2}, {}};
        bindings["slot_3"] = ActionBinding{{sf::Keyboard::Num3}, {}};
        bindings["slot_4"] = ActionBinding{{sf::Keyboard::Num4}, {}};
        bindings["slot_5"] = ActionBinding{{sf::Keyboard::Num5}, {}};
        bindings["slot_6"] = ActionBinding{{sf::Keyboard::Num6}, {}};
        bindings["slot_7"] = ActionBinding{{sf::Keyboard::Num7}, {}};
        bindings["slot_8"] = ActionBinding{{sf::Keyboard::Num8}, {}};
        bindings["slot_9"] = ActionBinding{{sf::Keyboard::Num9}, {}};
        bindings["slot_10"] = ActionBinding{{sf::Keyboard::Num0}, {}};
        bindings["slot_prev"] = ActionBinding{{sf::Keyboard::Q}, {}, {1}};
        bindings["slot_next"] = ActionBinding{{sf::Keyboard::E}, {}, {-1}};
        bindings["menu"] = ActionBinding{{sf::Keyboard::Escape}, {}};
        bindings["debug_toggle"] = ActionBinding{{sf::Keyboard::Tilde}, {}};
        bindings["inventory_prev"] = bindings["slot_prev"];
        bindings["inventory_next"] = bindings["slot_next"];
        bindings["inventory_slot_1"] = bindings["slot_1"];
        bindings["inventory_slot_2"] = bindings["slot_2"];
        bindings["inventory_slot_3"] = bindings["slot_3"];
        bindings["inventory_slot_4"] = bindings["slot_4"];
        bindings["inventory_slot_5"] = bindings["slot_5"];
        bindings["inventory_slot_6"] = bindings["slot_6"];
        bindings["inventory_slot_7"] = bindings["slot_7"];
        bindings["inventory_slot_8"] = bindings["slot_8"];
        bindings["inventory_slot_9"] = bindings["slot_9"];
        bindings["inventory_slot_10"] = bindings["slot_10"];
    }
}

