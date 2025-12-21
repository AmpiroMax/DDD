#ifndef DDD_EVENTS_SPELL_EVENTS_H
#define DDD_EVENTS_SPELL_EVENTS_H

#include "core/Entity.h"
#include <string>
#include <vector>

enum class SpellCastFailReason {
    None = 0,
    NotEnoughMana,
    OnCooldown,
    NoSpell,
    NoSlot,
    NoTarget,
    Blocked
};

struct SpellCastStartedEvent {
    Entity::Id entityId{0};
    std::string spellId;
    int slot{-1};
    int castId{0};
};

struct SpellCastFailedEvent {
    Entity::Id entityId{0};
    std::string spellId;
    int slot{-1};
    SpellCastFailReason reason{SpellCastFailReason::None};
    float cooldownRemaining{0.0f};
    float mana{0.0f};
    float manaCost{0.0f};
};

struct SpellCooldownChangedEvent {
    Entity::Id entityId{0};
    std::string spellId;
    int slot{-1};
    float remaining{0.0f};
    float total{0.0f};
};

struct ManaChangedEvent {
    Entity::Id entityId{0};
    float current{0.0f};
    float max{0.0f};
    float delta{0.0f};
};

struct SpellSlotSnapshot {
    std::string spellId;
    float cooldownRemaining{0.0f};
    float cooldownTotal{0.0f};
    int charges{-1};
};

struct SpellStateChangedEvent {
    Entity::Id entityId{0};
    std::vector<SpellSlotSnapshot> slots;
    float mana{0.0f};
    float manaMax{0.0f};
};

struct SummonSpawnedEvent {
    Entity::Id ownerId{0};
    Entity::Id summonEntity{0};
    std::string spellId;
    float lifetime{0.0f};
};

struct SummonDespawnedEvent {
    Entity::Id ownerId{0};
    Entity::Id summonEntity{0};
    std::string spellId;
    std::string reason; // e.g., "expired", "killed", "recalled"
};

#endif // DDD_EVENTS_SPELL_EVENTS_H

