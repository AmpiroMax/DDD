#include "systems/CameraFollowSystem.h"

CameraFollowSystem::CameraFollowSystem(CameraManager &cameraMgr, EntityManager &entityMgr)
    : cameraManager(cameraMgr), entityManager(entityMgr) {}

void CameraFollowSystem::update(float dt) {
    (void)dt;

    const TransformComponent *targetTransform = nullptr;
    for (auto &entPtr : entityManager.all()) {
        if (entPtr->has<CameraTargetTag>() || entPtr->has<PlayerTag>()) {
            targetTransform = entPtr->get<TransformComponent>();
            if (targetTransform)
                break;
        }
    }
    if (!targetTransform)
        return;

    // Always lock camera center to the target (player). No clamping to map bounds for now.
    cameraManager.setCenter(targetTransform->position);
}

