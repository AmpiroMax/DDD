#ifndef DDD_EVENTS_GAME_EVENTS_H
#define DDD_EVENTS_GAME_EVENTS_H

#include <string>
#include <vector>

struct MatchStateEvent {
    enum class Phase { PreMatch, InMatch, PostMatch };
    Phase phase{Phase::PreMatch};
    float matchTime{0.0f};       // seconds since match start (or countdown negative for pre)
    int scoreA{0};
    int scoreB{0};
};

struct PlayerStatsEvent {
    struct PlayerRow {
        int id{0};
        std::string name;
        int kills{0};
        int deaths{0};
    };
    std::vector<PlayerRow> players;
};

struct KillEvent {
    std::string killer;
    std::string victim;
    std::string weapon;
};

struct RespawnTimerEvent {
    float secondsLeft{0.0f};
    bool canRespawn{false};
};

#endif // DDD_EVENTS_GAME_EVENTS_H
