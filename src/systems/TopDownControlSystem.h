#ifndef DDD_SYSTEMS_TOP_DOWN_CONTROL_SYSTEM_H
#define DDD_SYSTEMS_TOP_DOWN_CONTROL_SYSTEM_H

#include "components/PhysicsBodyComponent.h"
#include "components/Tags.h"
#include "core/EntityManager.h"
#include "core/System.h"
#include "systems/InputSystem.h"

// Top-down character controller (Diablo-like):
// - Movement in XY plane with accel/decel ("feel")
// - Uses Box2D dynamic body; applies impulses to reach desired velocity
class TopDownControlSystem : public System {
  public:
    struct Params {
        float maxSpeed{6.0f};     // world units per second
        float accel{30.0f};       // world units per second^2
        float decel{40.0f};       // world units per second^2 (when no input)
        float linearDamping{6.0f}; // Box2D damping (helps smoothing)
        float inputDeadzone{0.001f};
        float arriveRadius{0.2f};  // stop within this distance
    };

    TopDownControlSystem(InputSystem &inputSystem, EntityManager &entityManager, Params params);

    void update(float dt) override;

    void setParams(const Params &p) { params = p; }
    const Params &getParams() const { return params; }

  private:
    InputSystem &inputSystem;
    EntityManager &entityManager;
    Params params;
    bool hasClickTarget{false};
    Vec2 clickTarget{0.0f, 0.0f};
};

#endif // DDD_SYSTEMS_TOP_DOWN_CONTROL_SYSTEM_H

