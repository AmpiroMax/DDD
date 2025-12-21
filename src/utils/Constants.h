#ifndef DDD_UTILS_CONSTANTS_H
#define DDD_UTILS_CONSTANTS_H

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG2RAD = PI / 180.0f;
constexpr float RAD2DEG = 180.0f / PI;

// Scales
constexpr float PHYSICS_SCALE = 1.0f;   // 1 world unit == 1 meter in physics
constexpr float RENDER_SCALE = 32.0f;   // 1 world unit == 32 pixels
constexpr float ISO_TILE_WIDTH = RENDER_SCALE * 2.0f;  // isometric diamond width in pixels for 1 world unit
constexpr float ISO_TILE_HEIGHT = RENDER_SCALE * 1.0f; // isometric diamond height in pixels for 1 world unit

// Physics step
constexpr float PHYSICS_TIMESTEP = 1.0f / 60.0f;
constexpr int PHYSICS_VELOCITY_ITER = 8;
constexpr int PHYSICS_POSITION_ITER = 3;

#endif // DDD_UTILS_CONSTANTS_H

