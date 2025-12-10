#ifndef DDD_EVENTS_PHYSICS_EVENTS_H
#define DDD_EVENTS_PHYSICS_EVENTS_H

#include "core/Entity.h"

struct ContactEvent {
    Entity::Id entityA{0};
    Entity::Id entityB{0};
    bool isBegin{false};
};

struct GroundedEvent {
    Entity::Id entityId{0};
    bool grounded{false};
};

#endif // DDD_EVENTS_PHYSICS_EVENTS_H
