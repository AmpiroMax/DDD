#ifndef DDD_GAME_CONFIG_LOADER_H
#define DDD_GAME_CONFIG_LOADER_H

#include "game/GameConfig.h"
#include <filesystem>

class ConfigLoader {
  public:
    static GameConfig loadGameConfig(const std::filesystem::path &path);
};

#endif // DDD_GAME_CONFIG_LOADER_H


