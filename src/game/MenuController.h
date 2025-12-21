#ifndef DDD_GAME_MENU_CONTROLLER_H
#define DDD_GAME_MENU_CONTROLLER_H

#include "systems/UIRenderSystem.h"
#include <SFML/Graphics/Rect.hpp>
#include <filesystem>
#include <string>
#include <vector>

class EventBus;
class DebugManager;
class InputSystem;
class WindowManager;

class MenuController {
  public:
    enum class Screen { MainMenu, MapSelect, Playing, PauseMenu, Settings };

    MenuController();

    void setHasSaveFile(bool v) { hasSaveFile = v; }
    bool getHasSaveFile() const { return hasSaveFile; }

    void setCurrentMapPath(const std::filesystem::path &p) { currentMapPath = p; }
    const std::filesystem::path &getCurrentMapPath() const { return currentMapPath; }

    void refreshMapList();
    void setScreen(Screen newScreen, WindowManager &windowManager, const InputSystem *inputSystem);
    Screen getScreen() const { return screen; }
    bool isMenuVisible() const { return menuRenderState.visible; }

    // Called every frame after inputSystem->update().
    void update(float dt, InputSystem &inputSystem, WindowManager &windowManager, DebugManager &debugManager,
                EventBus &eventBus, UIRenderSystem *uiRenderSystem);

    UIRenderSystem::MenuRenderState *getMenuStatePtr() { return &menuRenderState; }

  private:
    void buildMenuButtons(WindowManager &windowManager, const InputSystem *inputSystem);
    void handleMenuClicks(InputSystem &inputSystem, WindowManager &windowManager, EventBus &eventBus);

    Screen screen{Screen::MainMenu};
    UIRenderSystem::MenuRenderState menuRenderState;

    std::vector<std::filesystem::path> mapFiles;
    int selectedMapIndex{-1};
    bool hasSaveFile{false};
    std::filesystem::path currentMapPath;

    sf::FloatRect menuButtonRect{12.0f, 72.0f, 96.0f, 32.0f};
};

#endif // DDD_GAME_MENU_CONTROLLER_H


