#ifndef DDD_SYSTEMS_RESPAWN_SYSTEM_H
#define DDD_SYSTEMS_RESPAWN_SYSTEM_H

#include "components/HealthComponent.h"
#include "components/Tags.h"
#include "core/EntityManager.h"
#include "core/EventBus.h"
#include "core/System.h"
#include "events/MatchEvents.h"
#include "game/GameplayCommandBuffer.h"

class RespawnSystem : public System {
  public:
    RespawnSystem(EntityManager &entityMgr, EventBus &eventBus, GameplayCommandBuffer &cmdBuffer);
    ~RespawnSystem() override { shutdown(); }
    void update(float dt) override;
    void shutdown() override;

  private:
    void onPlayerDied(const PlayerDiedEvent &ev);

    EntityManager &entityManager;
    EventBus &eventBus;
    GameplayCommandBuffer &commandBuffer;
    EventBus::SubscriptionToken diedSub_{};
    EventBus::SubscriptionToken respawnReqSub_{};
};

#endif // DDD_SYSTEMS_RESPAWN_SYSTEM_H

