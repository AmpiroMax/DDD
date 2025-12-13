#ifndef DDD_SYSTEMS_DEBUG_SYSTEM_H
#define DDD_SYSTEMS_DEBUG_SYSTEM_H

#include "components/DropComponent.h"
#include "components/GroundedComponent.h"
#include "components/PhysicsBodyComponent.h"
#include "components/Tags.h"
#include "components/TilemapComponent.h"
#include "components/TransformComponent.h"
#include "core/EntityManager.h"
#include "core/System.h"
#include "managers/DebugManager.h"
#include "systems/InputSystem.h"
#include <string>

class DebugSystem : public System {
  public:
    DebugSystem(EntityManager &entityMgr, DebugManager &debugMgr, InputSystem &inputSys);
    void update(float dt) override;

  private:
    void maybeLogToFile(const std::vector<std::string> &mechanicsLines,
                        const std::vector<std::string> &physicsLines);

    EntityManager &entityManager;
    DebugManager &debugManager;
    InputSystem &inputSystem;
    std::size_t frameCounter{0};
    std::string logPath{"debug_log.txt"};
};

#endif // DDD_SYSTEMS_DEBUG_SYSTEM_H

