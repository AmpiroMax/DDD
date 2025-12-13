#include "systems/DropPickupSystem.h"

#include <cmath>

Entity *DropPickupSystem::findPlayer() const {
    for (auto &entPtr : entityManager.all()) {
        if (entPtr->has<PlayerTag>())
            return entPtr.get();
    }
    return nullptr;
}

void DropPickupSystem::destroyBodyIfAny(Entity &ent) {
    if (auto *body = ent.get<PhysicsBodyComponent>()) {
        if (body->body) {
            physicsManager.destroyBody(body->body);
            body->body = nullptr;
        }
    }
}

void DropPickupSystem::update(float dt) {
    (void)dt;
    Entity *player = findPlayer();
    if (!player)
        return;

    auto *playerTransform = player->get<TransformComponent>();
    if (!playerTransform)
        return;

    const float radius2 = pickupRadius * pickupRadius;
    std::vector<Entity::Id> toRemove;

    for (auto &entPtr : entityManager.all()) {
        auto *drop = entPtr->get<DropComponent>();
        auto *transform = entPtr->get<TransformComponent>();
        if (!drop || !transform)
            continue;

        const float dx = transform->position.x - playerTransform->position.x;
        const float dy = transform->position.y - playerTransform->position.y;
        const float dist2 = dx * dx + dy * dy;
        if (dist2 > radius2)
            continue;

        const int remaining = inventorySystem.addItem(player->getId(), drop->itemId, drop->count);
        if (remaining <= 0) {
            destroyBodyIfAny(*entPtr);
            toRemove.push_back(entPtr->getId());
        } else {
            drop->count = remaining;
        }
    }

    for (auto id : toRemove) {
        entityManager.remove(id);
    }
}

