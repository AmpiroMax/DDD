#ifndef DDD_MANAGERS_CAMERA_MANAGER_H
#define DDD_MANAGERS_CAMERA_MANAGER_H

#include "utils/Vec2.h"

class CameraManager {
  public:
    void setCenter(const Vec2 &c) { center = c; }
    Vec2 getCenter() const { return center; }

    void setViewportSize(const Vec2 &s) { viewportSize = s; }
    Vec2 getViewportSize() const { return viewportSize; }

    void setZoom(float z) { zoom = z; }
    float getZoom() const { return zoom; }

  private:
    Vec2 center{0.0f, 0.0f};
    Vec2 viewportSize{800.0f, 600.0f};
    float zoom{1.0f};
};

#endif // DDD_MANAGERS_CAMERA_MANAGER_H

