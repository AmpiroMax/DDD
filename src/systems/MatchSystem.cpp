#include "systems/MatchSystem.h"

#include <sstream>

MatchSystem::MatchSystem(EventBus &bus, DebugManager &dbg, float matchDurationSec)
    : eventBus(bus), debugManager(dbg), matchDuration(matchDurationSec) {
    scoredSub_ = eventBus.subscribeWithToken<PlayerScoredEvent>([this](const PlayerScoredEvent &ev) { onPlayerScored(ev); });
    diedSub_ = eventBus.subscribeWithToken<PlayerDiedEvent>([this](const PlayerDiedEvent &ev) { onPlayerDied(ev); });
    state = State::Lobby;
}

void MatchSystem::shutdown() {
    eventBus.unsubscribe(scoredSub_);
    eventBus.unsubscribe(diedSub_);
    scoredSub_ = {};
    diedSub_ = {};
}

void MatchSystem::startMatch() {
    state = State::Playing;
    timeLeft = matchDuration;
    scores.clear();
    eventBus.emit(MatchStartEvent{matchDuration});
}

void MatchSystem::endMatch(bool timeUp) {
    if (state == State::Ended)
        return;
    state = State::Ended;
    eventBus.emit(MatchEndEvent{timeUp});
}

void MatchSystem::update(float dt) {
    if (state == State::Lobby) {
        // Auto-start if not already running.
        startMatch();
    } else if (state == State::Playing) {
        timeLeft -= dt;
        if (timeLeft <= 0.0f) {
            timeLeft = 0.0f;
            endMatch(true);
        }
    }
    pushDebug();
}

void MatchSystem::onPlayerScored(const PlayerScoredEvent &ev) { scores[ev.scorer] += ev.delta; }

void MatchSystem::onPlayerDied(const PlayerDiedEvent &ev) {
    (void)ev;
    // Could emit respawn requests; for now just track.
}

void MatchSystem::pushDebug() {
    std::vector<std::string> lines;
    switch (state) {
    case State::Idle:
        lines.push_back("State: Idle");
        break;
    case State::Lobby:
        lines.push_back("State: Lobby");
        break;
    case State::Playing: {
        lines.push_back("State: Playing");
        std::ostringstream oss;
        oss << "Time left: " << timeLeft;
        lines.push_back(oss.str());
        break;
    }
    case State::Ended:
        lines.push_back("State: Ended");
        break;
    }
    if (!scores.empty()) {
        lines.push_back("Scores:");
        for (const auto &kv : scores) {
            std::ostringstream s;
            s << "  " << kv.first << " -> " << kv.second;
            lines.push_back(s.str());
        }
    }
    debugManager.setSection("match", lines);
}

