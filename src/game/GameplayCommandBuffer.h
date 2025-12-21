#ifndef DDD_GAME_GAMEPLAY_COMMAND_BUFFER_H
#define DDD_GAME_GAMEPLAY_COMMAND_BUFFER_H

#include "events/GameplayCommands.h"
#include <variant>
#include <vector>

using GameplayCommand = std::variant<RespawnCommand, PickupCommand, ConsumeItemCommand>;

class GameplayCommandBuffer {
  public:
    template <typename T> void push(T cmd) { commands_.emplace_back(std::move(cmd)); }

    const std::vector<GameplayCommand> &all() const { return commands_; }

    template <typename Fn> void consumeAll(Fn &&fn) {
        for (const auto &cmd : commands_) {
            fn(cmd);
        }
        commands_.clear();
    }

    void clear() { commands_.clear(); }
    bool empty() const { return commands_.empty(); }

  private:
    std::vector<GameplayCommand> commands_;
};

#endif // DDD_GAME_GAMEPLAY_COMMAND_BUFFER_H


