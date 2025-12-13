#ifndef DDD_SYSTEMS_CAMERA_FOLLOW_SYSTEM_H
#define DDD_SYSTEMS_CAMERA_FOLLOW_SYSTEM_H

#include "components/Tags.h"
#include "components/TilemapComponent.h"
#include "components/TransformComponent.h"
#include "core/EntityManager.h"
#include "core/System.h"
#include "managers/CameraManager.h"
#include "utils/Constants.h"
#include <algorithm>

class CameraFollowSystem : public System {
  public:
    CameraFollowSystem(CameraManager &cameraMgr, EntityManager &entityMgr);
    void update(float dt) override;

  private:
    CameraManager &cameraManager;
    EntityManager &entityManager;
};

#endif // DDD_SYSTEMS_CAMERA_FOLLOW_SYSTEM_H

