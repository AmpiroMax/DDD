#include "game/ConfigLoader.h"

#include "utils/Constants.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

GameConfig ConfigLoader::loadGameConfig(const std::filesystem::path &path) {
    GameConfig cfg{};

    std::ifstream file(path);
    if (!file.is_open())
        return cfg;

    try {
        nlohmann::json j;
        file >> j;

        if (j.contains("mode"))
            cfg.mode = j["mode"].get<std::string>();

        if (j.contains("window")) {
            const auto &w = j["window"];
            if (w.contains("size") && w["size"].is_array() && w["size"].size() >= 2) {
                cfg.windowWidth = w["size"][0].get<int>();
                cfg.windowHeight = w["size"][1].get<int>();
            }
            if (w.contains("title"))
                cfg.windowTitle = w["title"].get<std::string>();
        }

        if (j.contains("resources_path"))
            cfg.resourcesPath = j["resources_path"].get<std::string>();
        if (j.contains("textures_path"))
            cfg.texturesPath = j["textures_path"].get<std::string>();
        if (j.contains("fonts_path"))
            cfg.fontsPath = j["fonts_path"].get<std::string>();

        if (j.contains("world")) {
            const auto &w = j["world"];
            if (w.contains("tile_size"))
                cfg.tileSize = w["tile_size"].get<float>() / RENDER_SCALE;
            if (w.contains("map_file"))
                cfg.mapFile = w["map_file"].get<std::string>();
        }

        if (j.contains("inventory_file"))
            cfg.inventoryFile = j["inventory_file"].get<std::string>();
        if (j.contains("spells_file"))
            cfg.spellsFile = j["spells_file"].get<std::string>();
        if (j.contains("spawns_file"))
            cfg.spawnsFile = j["spawns_file"].get<std::string>();

        if (j.contains("match") && j["match"].contains("duration_sec"))
            cfg.matchDurationSec = j["match"]["duration_sec"].get<float>();

        if (j.contains("arena") && j["arena"].contains("bounds") && j["arena"]["bounds"].is_array() &&
            j["arena"]["bounds"].size() >= 4) {
            cfg.arena.has = true;
            cfg.arena.minX = j["arena"]["bounds"][0].get<float>();
            cfg.arena.maxX = j["arena"]["bounds"][1].get<float>();
            cfg.arena.minY = j["arena"]["bounds"][2].get<float>();
            cfg.arena.maxY = j["arena"]["bounds"][3].get<float>();
        }

        if (j.contains("player")) {
            const auto &p = j["player"];
            if (p.contains("speed"))
                cfg.playerSpeed = p["speed"].get<float>();
            if (p.contains("jump_impulse"))
                cfg.playerJump = p["jump_impulse"].get<float>();
            if (p.contains("radius"))
                cfg.playerRadius = p["radius"].get<float>();
            if (p.contains("accel"))
                cfg.playerAccel = p["accel"].get<float>();
            if (p.contains("decel"))
                cfg.playerDecel = p["decel"].get<float>();
            if (p.contains("linear_damping"))
                cfg.playerLinearDamping = p["linear_damping"].get<float>();
            if (p.contains("pickup_radius"))
                cfg.pickupRadius = p["pickup_radius"].get<float>() / RENDER_SCALE;
        }

        if (j.contains("max_players"))
            cfg.maxPlayers = j["max_players"].get<int>();
        if (j.contains("player_player_collision"))
            cfg.playerPlayerCollision = j["player_player_collision"].get<bool>();

        if (j.contains("iso")) {
            const auto &iso = j["iso"];
            if (iso.contains("axis_x_deg"))
                cfg.iso.axisXDeg = iso["axis_x_deg"].get<float>();
            if (iso.contains("axis_y_deg"))
                cfg.iso.axisYDeg = iso["axis_y_deg"].get<float>();
            if (iso.contains("pitch"))
                cfg.iso.pitch = iso["pitch"].get<float>();
            if (iso.contains("screen_scale"))
                cfg.iso.screenScale = iso["screen_scale"].get<float>();
        }
        cfg.iso.computeDirections();
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse " << path.string() << ": " << e.what() << "\n";
    }

    return cfg;
}


