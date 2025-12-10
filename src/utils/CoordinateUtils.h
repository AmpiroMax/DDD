#ifndef DDD_UTILS_COORDINATE_UTILS_H
#define DDD_UTILS_COORDINATE_UTILS_H

#include "Constants.h"
#include "Vec2.h"

inline Vec2 worldToRender(const Vec2 &world) { return {world.x * RENDER_SCALE, -world.y * RENDER_SCALE}; }
inline Vec2 renderToWorld(const Vec2 &render) { return {render.x / RENDER_SCALE, -render.y / RENDER_SCALE}; }

inline Vec2 worldToPhysics(const Vec2 &world) { return {world.x * PHYSICS_SCALE, world.y * PHYSICS_SCALE}; }
inline Vec2 physicsToWorld(const Vec2 &phys) { return {phys.x / PHYSICS_SCALE, phys.y / PHYSICS_SCALE}; }

inline float worldAngleToPhysics(float deg) { return deg * DEG2RAD; }
inline float physicsAngleToWorld(float rad) { return rad * RAD2DEG; }
inline float worldAngleToRender(float deg) { return -deg; }
inline float renderAngleToWorld(float deg) { return -deg; }

#endif // DDD_UTILS_COORDINATE_UTILS_H

