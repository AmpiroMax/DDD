#ifndef DDD_SYSTEMS_UI_RENDER_SYSTEM_H
#define DDD_SYSTEMS_UI_RENDER_SYSTEM_H

#include "core/System.h"
#include "core/EventBus.h"
#include "managers/DebugManager.h"
#include "managers/ResourceManager.h"
#include "managers/WindowManager.h"
#include "events/InventoryEvents.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class UIRenderSystem : public System {
  public:
    struct MenuRenderButton {
        std::string id;
        std::string label;
        sf::FloatRect rect;
        bool focused{false};
    };

    enum class MenuScreen { Main, MapSelect, Pause, Settings };

    struct MenuRenderState {
        bool visible{false};
        MenuScreen screen{MenuScreen::Main};
        std::vector<MenuRenderButton> buttons;
        std::vector<std::string> maps;
        int selectedMap{-1};
        bool hasSave{false};
        bool canResume{false};
        bool showMenuButton{false};
        sf::FloatRect menuButtonRect;
        bool showDebugButton{false};
        sf::FloatRect debugButtonRect;
        std::vector<std::string> settingsLines;
    };

    UIRenderSystem(WindowManager &windowMgr, ResourceManager &resourceMgr, DebugManager &debugMgr, EventBus &eventBus);
    void update(float dt) override;
    void setMenuState(const MenuRenderState *state) { menuState = state; }

  private:
    struct UISlot {
        int itemId{-1};
        int count{0};
        std::string iconTexture;
        std::string iconRegion;
    };

    void drawDebugOverlay(float dt);
    void drawInventoryUI();
    void drawMenuUI();
    void drawMenuButton();
    void drawDebugButton();
    void handleInventoryStateChanged(const InventoryStateChangedEvent &ev);

    WindowManager &windowManager;
    ResourceManager &resourceManager;
    DebugManager &debugManager;
    EventBus &eventBus;

    std::string debugFontName{"debug"};
    sf::Color debugTextColor{sf::Color::White};
    unsigned int debugCharacterSize{14};

    std::vector<UISlot> slots;
    int activeIndex{0};
    bool hasInventory{false};

    const MenuRenderState *menuState{nullptr};
};

#endif // DDD_SYSTEMS_UI_RENDER_SYSTEM_H

