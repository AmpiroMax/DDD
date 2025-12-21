#ifndef DDD_COMPONENTS_SPELLBOOK_COMPONENT_H
#define DDD_COMPONENTS_SPELLBOOK_COMPONENT_H

#include "core/Component.h"
#include <string>
#include <vector>

struct SpellSlotState {
    std::string spellId;
    float cooldownRemaining{0.0f};
    float cooldownTotal{0.0f};
    int charges{-1}; // -1 means unlimited
};

struct SpellbookComponent : Component {
    std::vector<SpellSlotState> slots;
    float mana{100.0f};
    float manaMax{100.0f};
    float manaRegen{5.0f};

    bool isValidSlot(int idx) const { return idx >= 0 && idx < static_cast<int>(slots.size()); }
};

#endif // DDD_COMPONENTS_SPELLBOOK_COMPONENT_H

