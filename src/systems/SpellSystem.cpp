#include "systems/SpellSystem.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include "components/Tags.h"

namespace {
SpellCastFailReason failReason(bool condition, SpellCastFailReason reason) {
    return condition ? reason : SpellCastFailReason::None;
}
} // namespace

SpellSystem::SpellSystem(InputSystem &inputSys, EntityManager &entityMgr, EventBus &eventBus)
    : inputSystem(inputSys), entityManager(entityMgr), eventBus(eventBus) {}

void SpellSystem::loadConfigFromFile(const std::string &path) {
    spellDefs.clear();
    initialSlots.clear();
    defaultMana = 100.0f;
    defaultManaRegen = 5.0f;

    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "Spell config not found, using defaults: " << path << "\n";
    } else {
        try {
            nlohmann::json j;
            in >> j;

            if (j.contains("mana")) {
                const auto &m = j["mana"];
                defaultMana = m.value("max", defaultMana);
                defaultManaRegen = m.value("regen", defaultManaRegen);
            }

            if (j.contains("spells") && j["spells"].is_object()) {
                for (auto &[id, val] : j["spells"].items()) {
                    SpellDefinition def{};
                    def.id = id;
                    if (val.contains("tags") && val["tags"].is_array()) {
                        for (const auto &t : val["tags"])
                            def.tags.push_back(t.get<std::string>());
                    }
                    def.manaCost = val.value("mana_cost", def.manaCost);
                    def.cooldown = val.value("cooldown", def.cooldown);
                    def.castTime = val.value("cast_time", def.castTime);
                    def.range = val.value("range", def.range);
                    if (val.contains("icon") && val["icon"].is_object()) {
                        const auto &ic = val["icon"];
                        def.iconTexture = ic.value("texture", "");
                        def.iconRegion = ic.value("region", "");
                    }
                    def.tooltip = val.value("tooltip", "");
                    spellDefs[def.id] = def;
                }
            }

            if (j.contains("slots") && j["slots"].is_array()) {
                for (const auto &s : j["slots"]) {
                    if (s.is_string())
                        initialSlots.push_back(s.get<std::string>());
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "Failed to parse spell config: " << e.what() << "\n";
        }
    }

    // Defaults if none provided.
    if (spellDefs.empty()) {
        SpellDefinition def{};
        def.id = "fireball";
        def.manaCost = 20.0f;
        def.cooldown = 3.0f;
        def.iconRegion = "fireball";
        def.iconTexture = "ui_spells";
        def.tooltip = "Simple fireball.";
        spellDefs[def.id] = def;
    }
    if (initialSlots.empty()) {
        initialSlots = {"fireball"};
    }
}

void SpellSystem::attachToEntity(Entity &entity) {
    auto *book = entity.get<SpellbookComponent>();
    if (!book)
        book = entity.addComponent<SpellbookComponent>();

    const int slotCount = static_cast<int>(initialSlots.size());
    book->slots.clear();
    book->slots.resize(slotCount);
    for (int i = 0; i < slotCount; ++i) {
        book->slots[i].spellId = initialSlots[i];
        const auto it = spellDefs.find(initialSlots[i]);
        const float cd = (it != spellDefs.end()) ? it->second.cooldown : 0.0f;
        book->slots[i].cooldownTotal = cd;
        book->slots[i].cooldownRemaining = 0.0f;
    }
    book->manaMax = defaultMana;
    book->mana = defaultMana;
    book->manaRegen = defaultManaRegen;

    emitSnapshot(entity.getId(), *book);
}

SpellbookComponent *SpellSystem::findSpellbook(Entity::Id entityId) const {
    if (Entity *ent = entityManager.find(entityId)) {
        return ent->get<SpellbookComponent>();
    }
    return nullptr;
}

Entity *SpellSystem::findPlayer() const {
    for (auto &entPtr : entityManager.all()) {
        if (entPtr->has<PlayerTag>())
            return entPtr.get();
    }
    return nullptr;
}

void SpellSystem::tickCooldowns(SpellbookComponent &book, float dt, Entity::Id id) {
    bool changed = false;
    for (auto &slot : book.slots) {
        if (slot.cooldownRemaining > 0.0f) {
            const float before = slot.cooldownRemaining;
            slot.cooldownRemaining = std::max(0.0f, slot.cooldownRemaining - dt);
            if (slot.cooldownRemaining != before) {
                changed = true;
                eventBus.emit(SpellCooldownChangedEvent{id, slot.spellId, static_cast<int>(&slot - &book.slots[0]),
                                                        slot.cooldownRemaining, slot.cooldownTotal});
            }
        }
    }
    if (changed)
        emitSnapshot(id, book);
}

void SpellSystem::regenMana(SpellbookComponent &book, float dt, Entity::Id id) {
    const float before = book.mana;
    book.mana = std::min(book.manaMax, book.mana + book.manaRegen * dt);
    if (book.mana != before) {
        eventBus.emit(ManaChangedEvent{id, book.mana, book.manaMax, book.mana - before});
        emitSnapshot(id, book);
    }
}

void SpellSystem::handleInput(SpellbookComponent &book, Entity::Id id) {
    InputComponent *input = inputSystem.getInput();
    if (!input)
        return;

    // Direct slot triggers.
    for (int i = 0; i < static_cast<int>(book.slots.size()); ++i) {
        const std::string action = "spell_" + std::to_string(i + 1);
        const auto it = input->actions.find(action);
        if (it != input->actions.end() && it->second.pressed) {
            tryCast(id, book, i);
        }
    }
}

bool SpellSystem::tryCast(Entity::Id entityId, SpellbookComponent &book, int slotIndex) {
    if (!book.isValidSlot(slotIndex))
        return false;
    SpellSlotState &slot = book.slots[slotIndex];
    auto it = spellDefs.find(slot.spellId);
    if (it == spellDefs.end()) {
        eventBus.emit(SpellCastFailedEvent{entityId, slot.spellId, slotIndex, SpellCastFailReason::NoSpell,
                                           slot.cooldownRemaining, book.mana, 0.0f});
        return false;
    }
    const SpellDefinition &def = it->second;

    if (slot.cooldownRemaining > 0.0f) {
        eventBus.emit(SpellCastFailedEvent{entityId, def.id, slotIndex, SpellCastFailReason::OnCooldown,
                                           slot.cooldownRemaining, book.mana, def.manaCost});
        return false;
    }
    if (book.mana < def.manaCost) {
        eventBus.emit(SpellCastFailedEvent{entityId, def.id, slotIndex, SpellCastFailReason::NotEnoughMana,
                                           slot.cooldownRemaining, book.mana, def.manaCost});
        return false;
    }

    book.mana -= def.manaCost;
    slot.cooldownTotal = def.cooldown;
    slot.cooldownRemaining = def.cooldown;

    eventBus.emit(SpellCastStartedEvent{entityId, def.id, slotIndex, 0});
    eventBus.emit(SpellCooldownChangedEvent{entityId, def.id, slotIndex, slot.cooldownRemaining, slot.cooldownTotal});
    eventBus.emit(ManaChangedEvent{entityId, book.mana, book.manaMax, -def.manaCost});
    emitSnapshot(entityId, book);
    return true;
}

void SpellSystem::emitSnapshot(Entity::Id entityId, const SpellbookComponent &book) {
    SpellStateChangedEvent ev{};
    ev.entityId = entityId;
    ev.mana = book.mana;
    ev.manaMax = book.manaMax;
    ev.slots.reserve(book.slots.size());
    for (const auto &slot : book.slots) {
        ev.slots.push_back(SpellSlotSnapshot{slot.spellId, slot.cooldownRemaining, slot.cooldownTotal, slot.charges});
    }
    eventBus.emit(ev);
}

void SpellSystem::update(float dt) {
    Entity *player = findPlayer();
    if (!player)
        return;
    auto *book = player->get<SpellbookComponent>();
    if (!book)
        return;

    tickCooldowns(*book, dt, player->getId());
    regenMana(*book, dt, player->getId());
    handleInput(*book, player->getId());
}

