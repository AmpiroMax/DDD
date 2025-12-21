#ifndef DDD_SYSTEMS_MATCH_SYSTEM_H
#define DDD_SYSTEMS_MATCH_SYSTEM_H

#include "core/EntityManager.h"
#include "core/EventBus.h"
#include "core/System.h"
#include "events/MatchEvents.h"
#include "managers/DebugManager.h"
#include <unordered_map>

class MatchSystem : public System {
  public:
    enum class State { Idle, Lobby, Playing, Ended };

    MatchSystem(EventBus &eventBus, DebugManager &debugManager, float matchDurationSec = 180.0f);
    ~MatchSystem() override { shutdown(); }

    void update(float dt) override;
    void shutdown() override;

    void startMatch();
    void endMatch(bool timeUp);

    State getState() const { return state; }
    float getTimeLeft() const { return timeLeft; }
    const std::unordered_map<Entity::Id, int> &getScores() const { return scores; }

  private:
    void onPlayerScored(const PlayerScoredEvent &ev);
    void onPlayerDied(const PlayerDiedEvent &ev);
    void pushDebug();

    EventBus &eventBus;
    DebugManager &debugManager;
    EventBus::SubscriptionToken scoredSub_{};
    EventBus::SubscriptionToken diedSub_{};
    State state{State::Idle};
    float matchDuration{180.0f};
    float timeLeft{0.0f};
    std::unordered_map<Entity::Id, int> scores;
  };

#endif // DDD_SYSTEMS_MATCH_SYSTEM_H

