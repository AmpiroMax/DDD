#ifndef DDD_SYSTEMS_UI_RENDER_SYSTEM_H
#define DDD_SYSTEMS_UI_RENDER_SYSTEM_H

#include "core/System.h"
#include "managers/DebugManager.h"
#include "managers/ResourceManager.h"
#include "managers/WindowManager.h"
#include <SFML/Graphics.hpp>
#include <string>

class UIRenderSystem : public System {
  public:
    UIRenderSystem(WindowManager &windowMgr, ResourceManager &resourceMgr, DebugManager &debugMgr);
    void update(float dt) override;

  private:
    void drawDebugOverlay(float dt);

    WindowManager &windowManager;
    ResourceManager &resourceManager;
    DebugManager &debugManager;

    std::string debugFontName{"debug"};
    sf::Color debugTextColor{sf::Color::White};
    unsigned int debugCharacterSize{14};
};

#endif // DDD_SYSTEMS_UI_RENDER_SYSTEM_H

