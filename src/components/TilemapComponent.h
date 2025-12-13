#ifndef DDD_COMPONENTS_TILEMAP_COMPONENT_H
#define DDD_COMPONENTS_TILEMAP_COMPONENT_H

#include "core/Component.h"
#include "utils/Vec2.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

struct TilemapComponent : Component {
    int width{0};
    int height{0};
    float tileSize{1.0f}; // world units
    Vec2 origin{0.0f, 0.0f}; // world position of the top-left tile
    int z{0};
    bool visible{true};

    int emptyId{-1};
    std::vector<int> solidIds;

    std::vector<int> tiles; // row-major, y grows downward
    std::vector<int> originalTiles; // snapshot of initial tiles for diff/saving
    std::unordered_map<int, std::string> tileIdToRegion; // tileId -> atlas region name
    std::string textureName; // optional direct texture if atlas region not used

    int index(int x, int y) const { return y * width + x; }
    bool inBounds(int x, int y) const { return x >= 0 && x < width && y >= 0 && y < height; }
    int get(int x, int y) const { return inBounds(x, y) ? tiles[index(x, y)] : -1; }
    bool isSolid(int id) const {
        return std::find(solidIds.begin(), solidIds.end(), id) != solidIds.end();
    }
};

#endif // DDD_COMPONENTS_TILEMAP_COMPONENT_H

