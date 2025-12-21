#ifndef DDD_UTILS_COORDINATE_UTILS_H
#define DDD_UTILS_COORDINATE_UTILS_H

#include "Constants.h"
#include "Vec2.h"

inline Vec2 worldToRender(const Vec2 &world) { return {world.x * RENDER_SCALE, -world.y * RENDER_SCALE}; }
inline Vec2 renderToWorld(const Vec2 &render) { return {render.x / RENDER_SCALE, -render.y / RENDER_SCALE}; }

// Classic 2:1 isometric projection helpers (world Y up, screen Y down)
inline Vec2 worldToIso(const Vec2 &world) {
    const float w = ISO_TILE_WIDTH * 0.5f;
    const float h = ISO_TILE_HEIGHT * 0.5f;
    return {(world.x - world.y) * w, (world.x + world.y) * h};
}
inline Vec2 isoToWorld(const Vec2 &iso) {
    const float w = ISO_TILE_WIDTH * 0.5f;
    const float h = ISO_TILE_HEIGHT * 0.5f;
    const float x = (iso.x / w + iso.y / h) * 0.5f;
    const float y = (iso.y / h - iso.x / w) * 0.5f;
    return {x, y};
}

inline Vec2 worldToPhysics(const Vec2 &world) { return {world.x * PHYSICS_SCALE, world.y * PHYSICS_SCALE}; }
inline Vec2 physicsToWorld(const Vec2 &phys) { return {phys.x / PHYSICS_SCALE, phys.y / PHYSICS_SCALE}; }

inline float worldAngleToPhysics(float deg) { return deg * DEG2RAD; }
inline float physicsAngleToWorld(float rad) { return rad * RAD2DEG; }
inline float worldAngleToRender(float deg) { return -deg; }
inline float renderAngleToWorld(float deg) { return -deg; }

#endif // DDD_UTILS_COORDINATE_UTILS_H

