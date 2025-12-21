#ifndef DDD_GAME_SAVE_SYSTEM_H
#define DDD_GAME_SAVE_SYSTEM_H

#include "game/SaveData.h"
#include <filesystem>
#include <optional>

class EntityManager;

class SaveSystem {
  public:
    static std::optional<SaveData> collect(const EntityManager &entityManager, const std::filesystem::path &currentMapPath);
    static bool writeToFile(const SaveData &data, const std::filesystem::path &savePath);
    static std::optional<SaveData> readFromFile(const std::filesystem::path &savePath,
                                                const std::filesystem::path &fallbackBaseMap);
    static bool apply(EntityManager &entityManager, const SaveData &data);
};

#endif // DDD_GAME_SAVE_SYSTEM_H


