#ifndef DDD_SYSTEMS_SPELL_SYSTEM_H
#define DDD_SYSTEMS_SPELL_SYSTEM_H

#include "components/SpellbookComponent.h"
#include "core/EntityManager.h"
#include "core/EventBus.h"
#include "core/System.h"
#include "events/SpellEvents.h"
#include "systems/InputSystem.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class SpellSystem : public System {
  public:
    struct SpellDefinition {
        std::string id;
        std::vector<std::string> tags;
        float manaCost{0.0f};
        float cooldown{0.0f};
        float castTime{0.0f};
        float range{0.0f};
        std::string iconTexture;
        std::string iconRegion;
        std::string tooltip;
    };

    explicit SpellSystem(InputSystem &inputSys, EntityManager &entityMgr, EventBus &eventBus);

    void loadConfigFromFile(const std::string &path);
    void attachToEntity(Entity &entity);

    void update(float dt) override;

    const std::unordered_map<std::string, SpellDefinition> &getDefinitions() const { return spellDefs; }

  private:
    InputSystem &inputSystem;
    EntityManager &entityManager;
    EventBus &eventBus;

    std::unordered_map<std::string, SpellDefinition> spellDefs;
    std::vector<std::string> initialSlots;
    float defaultMana{100.0f};
    float defaultManaRegen{5.0f};

    SpellbookComponent *findSpellbook(Entity::Id entityId) const;
    Entity *findPlayer() const;

    void tickCooldowns(SpellbookComponent &book, float dt, Entity::Id id);
    void regenMana(SpellbookComponent &book, float dt, Entity::Id id);
    void handleInput(SpellbookComponent &book, Entity::Id id);
    bool tryCast(Entity::Id entityId, SpellbookComponent &book, int slotIndex);
    void emitSnapshot(Entity::Id entityId, const SpellbookComponent &book);
};

#endif // DDD_SYSTEMS_SPELL_SYSTEM_H

