#ifndef DDD_COMPONENTS_INVENTORY_COMPONENT_H
#define DDD_COMPONENTS_INVENTORY_COMPONENT_H

#include "core/Component.h"
#include <vector>

struct ItemSlot {
    int itemId{-1};
    int count{0};

    bool empty() const { return itemId < 0 || count <= 0; }
    void clear() {
        itemId = -1;
        count = 0;
    }
};

struct InventoryComponent : Component {
    std::vector<ItemSlot> slots;
    int activeSlot{0};
    int hotbarSize{0}; // number of slots considered "quick access"

    bool isValidSlot(int idx) const { return idx >= 0 && idx < static_cast<int>(slots.size()); }
    ItemSlot *getSlot(int idx) { return isValidSlot(idx) ? &slots[idx] : nullptr; }
    const ItemSlot *getSlot(int idx) const { return isValidSlot(idx) ? &slots[idx] : nullptr; }
};

#endif // DDD_COMPONENTS_INVENTORY_COMPONENT_H

