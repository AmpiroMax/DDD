#ifndef DDD_EVENTS_MATCH_EVENTS_H
#define DDD_EVENTS_MATCH_EVENTS_H

#include "core/Entity.h"

struct MatchStartEvent {
    float durationSec{0.0f};
};

struct MatchEndEvent {
    bool timeUp{false};
};

struct PlayerDiedEvent {
    Entity::Id victim{0};
    Entity::Id killer{0};
};

struct RespawnRequestEvent {
    Entity::Id entityId{0};
    bool force{false};
};

struct PlayerScoredEvent {
    Entity::Id scorer{0};
    int delta{1};
};

struct RespawnedEvent {
    Entity::Id entityId{0};
};

#endif // DDD_EVENTS_MATCH_EVENTS_H

