#ifndef DDD_SYSTEMS_INPUT_SYSTEM_H
#define DDD_SYSTEMS_INPUT_SYSTEM_H

#include "components/InputComponent.h"
#include "core/EntityManager.h"
#include "core/System.h"
#include "managers/CameraManager.h"
#include "managers/WindowManager.h"
#include "utils/IsoConfig.h"
#include <SFML/Window/Event.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class InputSystem : public System {
  public:
    struct ActionBinding {
        std::vector<sf::Keyboard::Key> keys;
        std::vector<sf::Mouse::Button> mouseButtons;
        std::vector<int> wheelDirections; // +1 for up, -1 for down
    };

    InputSystem(WindowManager &windowMgr, CameraManager &cameraMgr, EntityManager &entityMgr);

    void beginFrame();
    void update(float dt) override;

    void handleEvent(const sf::Event &evt);

    void loadBindingsFromFile(const std::string &path);
    const std::unordered_map<std::string, ActionBinding> &getBindings() const { return bindings; }
    InputComponent *getInput() const { return input; }
    void setIsoConfig(const IsoConfig &cfg) { isoConfig = cfg; isoConfig.computeDirections(); }
    void resetInputEntity();

  private:
    void resetFrameStates();
    void updateMouse();
    void refreshActions();

    static sf::Keyboard::Key parseKey(const std::string &name);
    static sf::Mouse::Button parseMouseButton(const std::string &name);

    WindowManager &windowManager;
    CameraManager &cameraManager;
    EntityManager &entityManager;
    InputComponent *input{nullptr};
    std::unordered_map<std::string, ActionBinding> bindings;
    int wheelDelta{0};
    IsoConfig isoConfig{};
};

#endif // DDD_SYSTEMS_INPUT_SYSTEM_H

