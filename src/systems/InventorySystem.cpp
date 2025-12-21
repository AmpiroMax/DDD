#include "systems/InventorySystem.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <unordered_map>

InventorySystem::InventorySystem(InputSystem &inputSys, EntityManager &entityMgr, EventBus &eventBus)
    : inputSystem(inputSys), entityManager(entityMgr), eventBus(eventBus) {
    addItemSub_ = eventBus.subscribeWithToken<InventoryAddItemEvent>(
        [this](const InventoryAddItemEvent &ev) { addItem(ev.entityId, ev.itemId, ev.amount); });
}

void InventorySystem::shutdown() {
    eventBus.unsubscribe(addItemSub_);
    addItemSub_ = {};
}

void InventorySystem::loadConfigFromFile(const std::string &path) {
    // Defaults
    slotCount = 5;
    hotbarSize = 5;
    itemDefs.clear();
    initialSlots.clear();

    // Default ground block entry to keep existing behavior even without config.
    ItemDefinition ground;
    ground.id = 1;
    ground.maxStack = 99;
    ground.placeTileId = 1;
    ground.iconRegion = "ground";
    ground.iconTexture = "tiles";
    itemDefs[ground.id] = ground;

    initialSlots.assign(slotCount, {});
    initialSlots[0] = ItemSlot{ground.id, 20};

    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "Inventory config not found, using defaults: " << path << "\n";
        return;
    }

    try {
        nlohmann::json j;
        in >> j;

        slotCount = j.value("slots", slotCount);
        hotbarSize = j.value("hotbar_size", hotbarSize);
        if (slotCount < 1)
            slotCount = 1;
        if (hotbarSize < 1)
            hotbarSize = 1;
        hotbarSize = std::min(hotbarSize, slotCount);

        if (j.contains("items") && j["items"].is_object()) {
            std::unordered_map<int, ItemDefinition> parsed;
            for (auto &[key, val] : j["items"].items()) {
                ItemDefinition def{};
                def.id = std::stoi(key);
                def.maxStack = val.value("max_stack", def.maxStack);
                def.placeTileId = val.value("place_tile_id", def.placeTileId);
                if (val.contains("icon_region"))
                    def.iconRegion = val["icon_region"].get<std::string>();
                if (val.contains("icon_texture"))
                    def.iconTexture = val["icon_texture"].get<std::string>();
                parsed[def.id] = def;
            }
            if (!parsed.empty())
                itemDefs = std::move(parsed);
        }

        initialSlots.assign(slotCount, {});
        if (j.contains("initial") && j["initial"].is_array()) {
            int fillIdx = 0;
            for (const auto &entry : j["initial"]) {
                if (!entry.is_object())
                    continue;
                const int itemId = entry.value("item_id", -1);
                const int count = entry.value("count", 0);
                if (itemId < 0 || count <= 0)
                    continue;

                const auto *def = findDefinition(itemId);
                if (!def)
                    continue;

                int slot = entry.value("slot", -1);
                if (slot >= 0 && slot < slotCount) {
                    initialSlots[slot] = ItemSlot{itemId, std::min(count, def->maxStack)};
                } else {
                    while (fillIdx < slotCount && !initialSlots[fillIdx].empty())
                        ++fillIdx;
                    if (fillIdx < slotCount) {
                        initialSlots[fillIdx] = ItemSlot{itemId, std::min(count, def->maxStack)};
                    }
                }
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse inventory config: " << e.what() << "\n";
        // Keep defaults on error.
        initialSlots.assign(slotCount, {});
        initialSlots[0] = ItemSlot{1, 20};
    }
}

void InventorySystem::attachToEntity(Entity &entity) {
    auto *inv = entity.get<InventoryComponent>();
    if (!inv)
        inv = entity.addComponent<InventoryComponent>();

    inv->slots.assign(slotCount, {});
    inv->hotbarSize = hotbarSize;
    inv->activeSlot = 0;

    const int limit = std::min(static_cast<int>(inv->slots.size()), static_cast<int>(initialSlots.size()));
    for (int i = 0; i < limit; ++i) {
        const ItemSlot &src = initialSlots[i];
        if (src.empty())
            continue;
        const auto *def = findDefinition(src.itemId);
        if (!def)
            continue;
        inv->slots[i].itemId = src.itemId;
        inv->slots[i].count = std::min(src.count, def->maxStack);
    }

    clampActive(*inv);
    emitActiveChanged(entity.getId(), *inv, -1);
    emitStateChanged(entity.getId(), *inv);
}

void InventorySystem::update(float dt) {
    (void)dt;
    handleInput();
}

int InventorySystem::addItem(Entity::Id entityId, int itemId, int amount) {
    if (amount <= 0)
        return amount;
    auto *inv = findInventory(entityId);
    if (!inv)
        return amount;

    const ItemDefinition &def = ensureDefinition(itemId);

    const ItemSlot beforeActive = inv->isValidSlot(inv->activeSlot) ? inv->slots[inv->activeSlot] : ItemSlot{};

    int remaining = amount;
    // Fill existing stacks.
    for (auto &slot : inv->slots) {
        if (slot.itemId != itemId || slot.count >= def.maxStack)
            continue;
        const int canAdd = std::min(remaining, def.maxStack - slot.count);
        slot.count += canAdd;
        remaining -= canAdd;
        if (remaining <= 0)
            break;
    }
    // Use empty slots.
    for (auto &slot : inv->slots) {
        if (!slot.empty() || remaining <= 0)
            continue;
        const int toAdd = std::min(remaining, def.maxStack);
        slot.itemId = itemId;
        slot.count = toAdd;
        remaining -= toAdd;
    }

    const int added = amount - remaining;
    if (added > 0) {
        eventBus.emit(InventoryDropAddedEvent{entityId, itemId, added, remaining});
        if (inv->isValidSlot(inv->activeSlot)) {
            const ItemSlot &after = inv->slots[inv->activeSlot];
            if (after.itemId != beforeActive.itemId || after.count != beforeActive.count) {
                emitActiveChanged(entityId, *inv, inv->activeSlot);
            }
        }
        emitStateChanged(entityId, *inv);
    }

    return remaining;
}

bool InventorySystem::consumeFromSlot(Entity::Id entityId, int slotIndex, int amount) {
    if (amount <= 0)
        return false;
    auto *inv = findInventory(entityId);
    if (!inv || !inv->isValidSlot(slotIndex))
        return false;

    ItemSlot before = inv->slots[slotIndex];
    if (before.empty())
        return false;

    const int toConsume = std::min(amount, before.count);
    ItemSlot &slot = inv->slots[slotIndex];
    slot.count -= toConsume;
    if (slot.count <= 0)
        slot.clear();

    if (slotIndex == inv->activeSlot) {
        if (slot.itemId != before.itemId || slot.count != before.count) {
            emitActiveChanged(entityId, *inv, slotIndex);
        }
    }

    emitStateChanged(entityId, *inv);

    return true;
}

bool InventorySystem::consumeActive(Entity::Id entityId, int amount) {
    auto *inv = findInventory(entityId);
    if (!inv)
        return false;
    return consumeFromSlot(entityId, inv->activeSlot, amount);
}

bool InventorySystem::setActiveSlot(Entity::Id entityId, int slotIndex) {
    auto *inv = findInventory(entityId);
    if (!inv || inv->slots.empty())
        return false;
    if (!inv->isValidSlot(slotIndex))
        return false;

    if (inv->activeSlot == slotIndex)
        return false;

    const int previous = inv->activeSlot;
    inv->activeSlot = slotIndex;
    emitActiveChanged(entityId, *inv, previous);
    emitStateChanged(entityId, *inv);
    return true;
}

bool InventorySystem::cycleActiveSlot(Entity::Id entityId, int delta) {
    auto *inv = findInventory(entityId);
    if (!inv || inv->slots.empty() || delta == 0)
        return false;

    const int count = inv->hotbarSize > 0 ? std::min(inv->hotbarSize, static_cast<int>(inv->slots.size()))
                                          : static_cast<int>(inv->slots.size());
    if (count <= 0)
        return false;

    int next = inv->activeSlot + delta;
    while (next < 0)
        next += count;
    next %= count;

    return setActiveSlot(entityId, next);
}

std::optional<InventorySystem::ActiveItemInfo> InventorySystem::getActiveItem(Entity::Id entityId) const {
    const auto *inv = findInventory(entityId);
    if (!inv || !inv->isValidSlot(inv->activeSlot))
        return std::nullopt;
    const ItemSlot &slot = inv->slots[inv->activeSlot];
    if (slot.empty())
        return std::nullopt;

    ActiveItemInfo info{};
    info.slotIndex = inv->activeSlot;
    info.itemId = slot.itemId;
    info.count = slot.count;

    if (const auto *def = findDefinition(slot.itemId)) {
        info.placeTileId = def->placeTileId;
    }

    return info;
}

InventoryComponent *InventorySystem::findInventory(Entity::Id entityId) const {
    Entity *ent = entityManager.find(entityId);
    if (!ent)
        return nullptr;
    return ent->get<InventoryComponent>();
}

Entity *InventorySystem::findOwnerEntity() const {
    for (auto &entPtr : entityManager.all()) {
        if (entPtr->get<InventoryComponent>())
            return entPtr.get();
    }
    return nullptr;
}

const InventorySystem::ItemDefinition *InventorySystem::findDefinition(int itemId) const {
    auto it = itemDefs.find(itemId);
    if (it == itemDefs.end())
        return nullptr;
    return &it->second;
}

InventorySystem::ItemDefinition &InventorySystem::ensureDefinition(int itemId) {
    auto it = itemDefs.find(itemId);
    if (it != itemDefs.end())
        return it->second;

    ItemDefinition def{};
    def.id = itemId;
    def.maxStack = 99;
    def.placeTileId = itemId; // default: place same tile id
    def.iconRegion = "";
    def.iconTexture = "";
    auto [insertedIt, _] = itemDefs.emplace(itemId, def);
    return insertedIt->second;
}

void InventorySystem::handleInput() {
    InputComponent *input = inputSystem.getInput();
    if (!input)
        return;

    Entity *owner = findOwnerEntity();
    if (!owner)
        return;

    auto *inv = owner->get<InventoryComponent>();
    if (!inv || inv->slots.empty())
        return;

    int requestedSlot = -1;
    for (int i = 0; i < inv->hotbarSize && i < static_cast<int>(inv->slots.size()); ++i) {
        const std::string actionPrimary = "slot_" + std::to_string(i + 1);
        const std::string actionAlias = "inventory_slot_" + std::to_string(i + 1);
        const auto actPrimary = input->actions.find(actionPrimary);
        const auto actAlias = input->actions.find(actionAlias);
        const bool hitPrimary = actPrimary != input->actions.end() && actPrimary->second.pressed;
        const bool hitAlias = actAlias != input->actions.end() && actAlias->second.pressed;
        if (hitPrimary || hitAlias) {
            requestedSlot = i;
        }
    }

    int cycleDelta = 0;
    const auto nextIt = input->actions.find("slot_next");
    const auto prevIt = input->actions.find("slot_prev");
    const auto nextAlias = input->actions.find("inventory_next");
    const auto prevAlias = input->actions.find("inventory_prev");
    if (nextIt != input->actions.end() && nextIt->second.pressed)
        cycleDelta = 1;
    if (nextAlias != input->actions.end() && nextAlias->second.pressed)
        cycleDelta = 1;
    if (prevIt != input->actions.end() && prevIt->second.pressed)
        cycleDelta = -1;
    if (prevAlias != input->actions.end() && prevAlias->second.pressed)
        cycleDelta = -1;

    if (requestedSlot >= 0)
        setActiveSlot(owner->getId(), requestedSlot);
    else if (cycleDelta != 0)
        cycleActiveSlot(owner->getId(), cycleDelta);
}

void InventorySystem::clampActive(InventoryComponent &inv) {
    if (inv.slots.empty()) {
        inv.activeSlot = 0;
        return;
    }
    if (!inv.isValidSlot(inv.activeSlot))
        inv.activeSlot = 0;
    inv.hotbarSize = std::min(inv.hotbarSize, static_cast<int>(inv.slots.size()));
    if (inv.hotbarSize < 1)
        inv.hotbarSize = static_cast<int>(inv.slots.size());
}

void InventorySystem::emitActiveChanged(Entity::Id entityId, const InventoryComponent &inv, int previousSlot) {
    InventoryActiveSlotChangedEvent ev{};
    ev.entityId = entityId;
    ev.previous = previousSlot;
    ev.current = inv.activeSlot;

    if (inv.isValidSlot(inv.activeSlot)) {
        const ItemSlot &slot = inv.slots[inv.activeSlot];
        ev.itemId = slot.itemId;
        ev.count = slot.count;
    }

    eventBus.emit(ev);
}

void InventorySystem::emitStateChanged(Entity::Id entityId, const InventoryComponent &inv) {
    InventoryStateChangedEvent ev{};
    ev.entityId = entityId;
    ev.activeIndex = inv.activeSlot;
    ev.slots.reserve(inv.slots.size());
    for (const auto &slot : inv.slots) {
        ev.slots.push_back(InventorySlotState{slot.itemId, slot.count});
    }
    for (const auto &[id, def] : itemDefs) {
        InventoryIconRef ref{};
        ref.texture = def.iconTexture;
        ref.region = def.iconRegion;
        ev.itemMeta[id] = ref;
    }
    eventBus.emit(ev);
}

