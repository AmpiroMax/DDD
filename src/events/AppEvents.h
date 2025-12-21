#ifndef DDD_EVENTS_APP_EVENTS_H
#define DDD_EVENTS_APP_EVENTS_H

#include <filesystem>

// High-level app commands (UI / menu -> GameApp).
struct ExitAppRequestedEvent {};
struct PauseRequestedEvent {};
struct ResumeRequestedEvent {};
struct ExitToMainRequestedEvent {};

struct StartGameRequestedEvent {
    std::filesystem::path mapPath;
};

// Continue == try to load latest save, fallback to starting current map.
struct ContinueRequestedEvent {};

#endif // DDD_EVENTS_APP_EVENTS_H


