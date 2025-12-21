#ifndef DDD_UTILS_ISO_COORDINATE_H
#define DDD_UTILS_ISO_COORDINATE_H

#include "IsoConfig.h"
#include "managers/CameraManager.h"
#include "managers/WindowManager.h"
#include "utils/CoordinateUtils.h"
#include "utils/Vec2.h"
#include <SFML/Graphics.hpp>

namespace IsoCoordinate {

// Build config from camera/scale (angles/pitch come from IsoConfig).
inline IsoConfig makeConfig(const IsoConfig &base, float screenScaleOverride = -1.0f) {
    IsoConfig cfg = base;
    if (screenScaleOverride > 0.0f)
        cfg.screenScale = screenScaleOverride;
    cfg.computeDirections();
    return cfg;
}

// World (game units) -> Screen (pixels) using iso axes and camera center/zoom.
inline Vec2 worldToScreen(const Vec2 &world, const CameraManager &cam, const IsoConfig &cfg) {
    const Vec2 center = cam.getCenter();
    const float zoom = cam.getZoom();
    const float s = cfg.screenScale / zoom;
    const float dx = world.x - center.x;
    const float dy = world.y - center.y;
    const float sx = (dx * cfg.dirXx + dy * cfg.dirYx) * s;
    const float sy = (dx * cfg.dirXy + dy * cfg.dirYy) * s;
    // Screen origin at window center in SFML view space; caller sets view.
    return Vec2{sx, sy};
}

// Screen (pixels, view-space) -> World (game units).
inline Vec2 screenToWorld(const Vec2 &screen, const CameraManager &cam, const IsoConfig &cfg) {
    const float zoom = cam.getZoom();
    const float s = cfg.screenScale / zoom;
    const float sx = screen.x / s;
    const float sy = screen.y / s;
    const float detInv = cfg.invDet;
    const float wx = (sx * cfg.dirYy - sy * cfg.dirYx) * detInv;
    const float wy = (-sx * cfg.dirXy + sy * cfg.dirXx) * detInv;
    const Vec2 center = cam.getCenter();
    return Vec2{center.x + wx, center.y + wy};
}

inline Vec2 mousePixelToWorld(const sf::Vector2i &pixel, const WindowManager &win, const CameraManager &cam,
                              const IsoConfig &cfg) {
    const sf::RenderWindow &w = win.getWindow();
    const sf::Vector2f viewPos = w.mapPixelToCoords(pixel, win.getView());
    return screenToWorld(Vec2{viewPos.x, viewPos.y}, cam, cfg);
}

// Physics helpers (using existing CoordinateUtils).
inline Vec2 worldToPhys(const Vec2 &world) { return worldToPhysics(world); }
inline Vec2 physToWorld(const Vec2 &phys) { return physicsToWorld(phys); }

} // namespace IsoCoordinate

#endif // DDD_UTILS_ISO_COORDINATE_H

