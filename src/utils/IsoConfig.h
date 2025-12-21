#ifndef DDD_UTILS_ISO_CONFIG_H
#define DDD_UTILS_ISO_CONFIG_H

#include <cmath>

struct IsoConfig {
    // Angles (degrees) for axis directions relative to screen X (to the right).
    // Default: 120° apart, X axis points left-down (150°), Y axis points right-down (30°).
    float axisXDeg{150.0f};
    float axisYDeg{30.0f};

    // Pitch factor: 1.0 = classic isometric squash, 0.0 = top-down (no vertical squash).
    float pitch{1.0f};

    // Screen scale (pixels per world unit) before pitch/angle projection.
    float screenScale{32.0f};

    // Derived directions in screen space (unit length).
    void computeDirections() {
        const float radX = axisXDeg * static_cast<float>(M_PI) / 180.0f;
        const float radY = axisYDeg * static_cast<float>(M_PI) / 180.0f;
        dirXx = std::cos(radX);
        dirXy = std::sin(radX) * pitch;
        dirYx = std::cos(radY);
        dirYy = std::sin(radY) * pitch;
        // Precompute matrix determinant for inverse
        const float det = dirXx * dirYy - dirYx * dirXy;
        invDet = (std::abs(det) > 1e-6f) ? 1.0f / det : 0.0f;
    }

    float dirXx{0.0f};
    float dirXy{0.0f};
    float dirYx{0.0f};
    float dirYy{0.0f};
    float invDet{0.0f};
};

#endif // DDD_UTILS_ISO_CONFIG_H

