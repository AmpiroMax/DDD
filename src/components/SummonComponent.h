#ifndef DDD_COMPONENTS_SUMMON_COMPONENT_H
#define DDD_COMPONENTS_SUMMON_COMPONENT_H

#include "core/Component.h"
#include <string>

struct SummonComponent : Component {
    std::string spellId;
    Entity::Id ownerId{0};
    float lifetime{0.0f};
    float lifetimeRemaining{0.0f};
    float maxHp{0.0f};
    float hp{0.0f};
    float moveSpeed{0.0f};
    float damage{0.0f};
};

#endif // DDD_COMPONENTS_SUMMON_COMPONENT_H

