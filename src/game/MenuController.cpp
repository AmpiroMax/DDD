#include "game/MenuController.h"

#include "components/InputComponent.h"
#include "core/EventBus.h"
#include "events/AppEvents.h"
#include "managers/DebugManager.h"
#include "managers/WindowManager.h"
#include "systems/InputSystem.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <algorithm>

MenuController::MenuController() {
    menuRenderState.visible = true;
    menuRenderState.showMenuButton = false;
    menuRenderState.menuButtonRect = menuButtonRect;
    menuRenderState.screen = UIRenderSystem::MenuScreen::Main;
}

void MenuController::refreshMapList() {
    mapFiles.clear();
    const std::filesystem::path mapsDir = std::filesystem::path("config") / "maps";
    if (!std::filesystem::exists(mapsDir))
        return;
    for (const auto &entry : std::filesystem::directory_iterator(mapsDir)) {
        if (!entry.is_regular_file())
            continue;
        if (entry.path().extension() == ".json") {
            mapFiles.push_back(entry.path());
        }
    }
    std::sort(mapFiles.begin(), mapFiles.end());
    if (!mapFiles.empty() && selectedMapIndex < 0)
        selectedMapIndex = 0;
}

void MenuController::setScreen(Screen newScreen, WindowManager &windowManager, const InputSystem *inputSystem) {
    screen = newScreen;
    menuRenderState.visible = (screen != Screen::Playing);
    menuRenderState.showMenuButton = (screen == Screen::Playing);
    menuRenderState.menuButtonRect = menuButtonRect;

    switch (screen) {
    case Screen::MainMenu:
        menuRenderState.screen = UIRenderSystem::MenuScreen::Main;
        break;
    case Screen::MapSelect:
        refreshMapList();
        menuRenderState.screen = UIRenderSystem::MenuScreen::MapSelect;
        break;
    case Screen::PauseMenu:
        menuRenderState.screen = UIRenderSystem::MenuScreen::Pause;
        break;
    case Screen::Playing:
        break;
    case Screen::Settings:
        menuRenderState.screen = UIRenderSystem::MenuScreen::Settings;
        break;
    }
    buildMenuButtons(windowManager, inputSystem);
}

void MenuController::buildMenuButtons(WindowManager &windowManager, const InputSystem *inputSystem) {
    menuRenderState.buttons.clear();
    menuRenderState.maps.clear();
    for (const auto &p : mapFiles) {
        menuRenderState.maps.push_back(p.filename().string());
    }
    menuRenderState.selectedMap = selectedMapIndex;
    menuRenderState.hasSave = hasSaveFile;
    menuRenderState.canResume = (screen == Screen::PauseMenu);
    menuRenderState.showMenuButton = (screen == Screen::Playing);
    menuRenderState.menuButtonRect = menuButtonRect;
    menuRenderState.showDebugButton = false;

    const sf::Vector2u ws = windowManager.getWindow().getSize();
    const float btnWidth = 320.0f;
    const float btnHeight = 44.0f;
    const float startX = 0.5f * (ws.x - btnWidth);
    float startY = 0.0f;

    auto pushBtn = [&](const std::string &id, const std::string &label) {
        menuRenderState.buttons.push_back(
            {id, label, sf::FloatRect{startX, startY + btnHeight * static_cast<float>(menuRenderState.buttons.size()),
                                      btnWidth, btnHeight},
             false});
    };

    if (screen == Screen::MainMenu) {
        startY = 140.0f;
        if (hasSaveFile)
            pushBtn("continue", "Continue");
        pushBtn("play", "Play");
        pushBtn("exit", "Exit");
    } else if (screen == Screen::MapSelect) {
        startY = 100.0f;
        float y = startY;
        for (size_t i = 0; i < menuRenderState.maps.size(); ++i) {
            menuRenderState.buttons.push_back(
                {"map_" + std::to_string(i), menuRenderState.maps[i], sf::FloatRect{startX, y, btnWidth, btnHeight},
                 static_cast<int>(i) == selectedMapIndex});
            y += btnHeight + 10.0f;
        }
        menuRenderState.buttons.push_back({"back", "Back", sf::FloatRect{startX, y + 10.0f, btnWidth, btnHeight}, false});
    } else if (screen == Screen::PauseMenu) {
        startY = 140.0f;
        pushBtn("resume", "Continue");
        pushBtn("settings", "Settings");
        pushBtn("save_exit", "Save & Exit");
    } else if (screen == Screen::Settings) {
        startY = 140.0f;
        pushBtn("back_pause", "Back");
        pushBtn("resume", "Continue");
        // Build settings lines from bindings.
        menuRenderState.settingsLines.clear();
        if (inputSystem) {
            const auto &bindings = inputSystem->getBindings();
            for (const auto &kv : bindings) {
                const auto &action = kv.first;
                const auto &binding = kv.second;
                std::string line = action + ":";
                auto appendKey = [&](const std::string &s) {
                    if (!line.empty())
                        line += " ";
                    line += s;
                };
                auto keyName = [](sf::Keyboard::Key k) -> std::string {
                    switch (k) {
                    case sf::Keyboard::Escape:
                        return "Esc";
                    case sf::Keyboard::A:
                        return "A";
                    case sf::Keyboard::B:
                        return "B";
                    case sf::Keyboard::C:
                        return "C";
                    case sf::Keyboard::D:
                        return "D";
                    case sf::Keyboard::E:
                        return "E";
                    case sf::Keyboard::Q:
                        return "Q";
                    case sf::Keyboard::S:
                        return "S";
                    case sf::Keyboard::W:
                        return "W";
                    case sf::Keyboard::Left:
                        return "Left";
                    case sf::Keyboard::Right:
                        return "Right";
                    case sf::Keyboard::Up:
                        return "Up";
                    case sf::Keyboard::Down:
                        return "Down";
                    case sf::Keyboard::Space:
                        return "Space";
                    case sf::Keyboard::Num0:
                        return "0";
                    case sf::Keyboard::Num1:
                        return "1";
                    case sf::Keyboard::Num2:
                        return "2";
                    case sf::Keyboard::Num3:
                        return "3";
                    case sf::Keyboard::Num4:
                        return "4";
                    case sf::Keyboard::Num5:
                        return "5";
                    case sf::Keyboard::Num6:
                        return "6";
                    case sf::Keyboard::Num7:
                        return "7";
                    case sf::Keyboard::Num8:
                        return "8";
                    case sf::Keyboard::Num9:
                        return "9";
                    default:
                        return "Key";
                    }
                };
                auto mouseName = [](sf::Mouse::Button b) -> std::string {
                    switch (b) {
                    case sf::Mouse::Left:
                        return "MouseLeft";
                    case sf::Mouse::Right:
                        return "MouseRight";
                    case sf::Mouse::Middle:
                        return "MouseMiddle";
                    default:
                        return "Mouse";
                    }
                };
                for (auto k : binding.keys)
                    appendKey(keyName(k));
                for (auto m : binding.mouseButtons)
                    appendKey(mouseName(m));
                for (auto w : binding.wheelDirections)
                    appendKey(w > 0 ? "WheelUp" : "WheelDown");
                menuRenderState.settingsLines.push_back(line);
            }
        }
    }
}

void MenuController::handleMenuClicks(InputSystem &inputSystem, WindowManager &windowManager, EventBus &eventBus) {
    InputComponent *input = inputSystem.getInput();
    if (!input)
        return;

    // While playing: only the menu button is interactive.
    if (screen == Screen::Playing) {
        auto itMb = input->mouseButtons.find(static_cast<int>(sf::Mouse::Left));
        if (itMb != input->mouseButtons.end() && itMb->second.pressed) {
            const sf::Vector2i mouse = input->mousePixel;
            const sf::Vector2f mf(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
            if (menuButtonRect.contains(mf)) {
                eventBus.emit(PauseRequestedEvent{});
            }
        }
        return;
    }

    auto it = input->mouseButtons.find(static_cast<int>(sf::Mouse::Left));
    if (it == input->mouseButtons.end() || !it->second.pressed)
        return;
    const sf::Vector2i mouse = input->mousePixel;
    const sf::Vector2f mf(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

    for (const auto &btn : menuRenderState.buttons) {
        if (!btn.rect.contains(mf))
            continue;
        if (btn.id == "play") {
            setScreen(Screen::MapSelect, windowManager, &inputSystem);
        } else if (btn.id == "exit") {
            eventBus.emit(ExitAppRequestedEvent{});
        } else if (btn.id == "continue") {
            eventBus.emit(ContinueRequestedEvent{});
        } else if (btn.id.rfind("map_", 0) == 0) {
            const int idx = std::stoi(btn.id.substr(4));
            if (idx >= 0 && idx < static_cast<int>(mapFiles.size())) {
                selectedMapIndex = idx;
                currentMapPath = mapFiles[idx];
                eventBus.emit(StartGameRequestedEvent{currentMapPath});
            }
        } else if (btn.id == "back") {
            setScreen(Screen::MainMenu, windowManager, &inputSystem);
        } else if (btn.id == "resume") {
            eventBus.emit(ResumeRequestedEvent{});
        } else if (btn.id == "settings") {
            setScreen(Screen::Settings, windowManager, &inputSystem);
        } else if (btn.id == "save_exit") {
            // Per current iteration this does not save; it's an exit-to-main action.
            eventBus.emit(ExitToMainRequestedEvent{});
        } else if (btn.id == "back_pause") {
            setScreen(Screen::PauseMenu, windowManager, &inputSystem);
        }
        break;
    }
}

void MenuController::update(float dt, InputSystem &inputSystem, WindowManager &windowManager, DebugManager &debugManager,
                            EventBus &eventBus, UIRenderSystem *uiRenderSystem) {
    (void)dt;
    InputComponent *input = inputSystem.getInput();
    if (!input)
        return;

    const auto getAction = [&](const std::string &name) -> const ButtonState * {
        auto it = input->actions.find(name);
        return it != input->actions.end() ? &it->second : nullptr;
    };
    const ButtonState *menuAction = getAction("menu");
    const ButtonState *debugToggle = getAction("debug_toggle");
    const ButtonState *scoreboardAction = getAction("scoreboard");

    if (debugToggle && debugToggle->pressed)
        debugManager.setVisible(!debugManager.isVisible());

    if (uiRenderSystem && screen != Screen::Playing) {
        uiRenderSystem->setShowScoreboard(false);
    }

    // Hover highlight
    const sf::Vector2i mouse = input->mousePixel;
    const sf::Vector2f mf(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
    for (auto &btn : menuRenderState.buttons) {
        btn.focused = btn.rect.contains(mf);
    }

    if (screen == Screen::Playing) {
        if (menuAction && menuAction->pressed) {
            eventBus.emit(PauseRequestedEvent{});
            return;
        }
        if (uiRenderSystem) {
            const bool show = scoreboardAction && (scoreboardAction->held || scoreboardAction->pressed);
            uiRenderSystem->setShowScoreboard(show);
        }
        handleMenuClicks(inputSystem, windowManager, eventBus);
        return;
    }

    // Menu visible states
    if (menuAction && menuAction->pressed) {
        if (screen == Screen::PauseMenu) {
            eventBus.emit(ResumeRequestedEvent{});
        } else if (screen == Screen::MapSelect) {
            setScreen(Screen::MainMenu, windowManager, &inputSystem);
        } else if (screen == Screen::Settings) {
            setScreen(Screen::PauseMenu, windowManager, &inputSystem);
        }
        return;
    }

    handleMenuClicks(inputSystem, windowManager, eventBus);
}


