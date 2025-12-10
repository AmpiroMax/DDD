#include "systems/UIRenderSystem.h"

#include <iomanip>
#include <sstream>

UIRenderSystem::UIRenderSystem(WindowManager &windowMgr, ResourceManager &resourceMgr, DebugManager &debugMgr)
    : windowManager(windowMgr), resourceManager(resourceMgr), debugManager(debugMgr) {}

void UIRenderSystem::update(float dt) {
    sf::RenderWindow &window = windowManager.getWindow();
    window.setView(window.getDefaultView());

    drawDebugOverlay(dt);
    window.display();
}

void UIRenderSystem::drawDebugOverlay(float dt) {
    if (!debugManager.isVisible())
        return;
    if (!resourceManager.hasFont(debugFontName))
        return;

    sf::Text text;
    text.setFont(resourceManager.getFont(debugFontName));
    text.setCharacterSize(debugCharacterSize);
    text.setFillColor(debugTextColor);

    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss << std::setprecision(1);
    const float fps = dt > 0.0001f ? 1.0f / dt : 0.0f;
    ss << "FPS: " << fps;
    const std::string &debugStr = debugManager.getString();
    if (!debugStr.empty()) {
        ss << "\n" << debugStr;
    }

    text.setString(ss.str());
    text.setPosition(8.0f, 8.0f);

    window.draw(text);
}

