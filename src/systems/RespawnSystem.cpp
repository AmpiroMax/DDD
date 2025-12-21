#include "systems/RespawnSystem.h"

RespawnSystem::RespawnSystem(EntityManager &entityMgr, EventBus &evtBus, GameplayCommandBuffer &cmdBuffer)
    : entityManager(entityMgr), eventBus(evtBus), commandBuffer(cmdBuffer) {
    diedSub_ = eventBus.subscribeWithToken<PlayerDiedEvent>([this](const PlayerDiedEvent &ev) { onPlayerDied(ev); });
    respawnReqSub_ = eventBus.subscribeWithToken<RespawnRequestEvent>(
        [this](const RespawnRequestEvent &ev) { commandBuffer.push(RespawnCommand{ev.entityId}); });
}

void RespawnSystem::shutdown() {
    eventBus.unsubscribe(diedSub_);
    eventBus.unsubscribe(respawnReqSub_);
    diedSub_ = {};
    respawnReqSub_ = {};
}

void RespawnSystem::update(float dt) {
    (void)dt;
    // No-op: commands are deferred to GameApp phase.
}

void RespawnSystem::onPlayerDied(const PlayerDiedEvent &ev) {
    commandBuffer.push(RespawnCommand{ev.victim});
}

