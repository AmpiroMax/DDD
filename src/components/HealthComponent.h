#ifndef DDD_COMPONENTS_HEALTH_COMPONENT_H
#define DDD_COMPONENTS_HEALTH_COMPONENT_H

#include "core/Component.h"

struct HealthComponent : Component {
    int hp{100};
    int maxHp{100};

    bool isDead() const { return hp <= 0; }
    void applyDamage(int dmg) { hp -= dmg; }
    void reset() { hp = maxHp; }
};

#endif // DDD_COMPONENTS_HEALTH_COMPONENT_H

