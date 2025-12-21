#ifndef DDD_SYSTEMS_RENDER_SYSTEM_H
#define DDD_SYSTEMS_RENDER_SYSTEM_H

#include "core/System.h"
#include "managers/CameraManager.h"
#include "managers/DebugManager.h"
#include "managers/ResourceManager.h"
#include "managers/WindowManager.h"
#include "core/EntityManager.h"
#include "components/SpriteComponent.h"
#include "components/SpriteAnimationComponent.h"
#include "components/ShadowComponent.h"
#include "components/TilemapComponent.h"
#include "components/TransformComponent.h"
#include "utils/IsoConfig.h"
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>

class RenderSystem : public System {
  public:
    RenderSystem(WindowManager &windowMgr, CameraManager &cameraMgr, ResourceManager &resourceMgr, EntityManager &entityMgr,
                 DebugManager *debugMgr = nullptr, IsoConfig isoCfg = {});
    void update(float dt) override;
    void setIsoConfig(const IsoConfig &cfg) { isoConfig = cfg; isoConfig.computeDirections(); }

  private:
    struct DrawCmd {
        float sortKey{0.0f};
        float sortKeyX{0.0f};
        int z{0};
        Entity::Id id{0};
        const SpriteComponent *sprite{nullptr};
        const TilemapComponent *tilemap{nullptr};
        const TransformComponent *transform{nullptr};
        Vec2 worldPos{}; // foot/anchor in world units
        int tileId{-1};
        const ShadowComponent *shadow{nullptr};
        bool isShadow{false};
    };

    void updateView();
    void enqueueTilemap(const TilemapComponent &tilemap, const TransformComponent *transform, std::vector<DrawCmd> &out);
    void enqueueSprite(const SpriteComponent &spriteComp, const ShadowComponent *shadow, const TransformComponent *transform,
                       Entity::Id id, std::vector<DrawCmd> &out);
    void enqueueShadow(const ShadowComponent &shadow, const TransformComponent *transform, Entity::Id id, float sortKey,
                       float sortKeyX, std::vector<DrawCmd> &out);
    void drawCmd(const DrawCmd &cmd, sf::RenderWindow &window);
    void drawAnchorDebug(const Vec2 &isoPos, bool isTile, sf::RenderWindow &window) const;
    void updateMinimapControls();
    void drawMinimap(sf::RenderWindow &window) const;

    WindowManager &windowManager;
    CameraManager &cameraManager;
    ResourceManager &resourceManager;
    EntityManager &entityManager;
    DebugManager *debugManager{nullptr};
    IsoConfig isoConfig{};

    sf::Color clearColor{sf::Color::Black};

    // Minimap / top-down debug view
    float minimapZoom{1.0f}; // larger = zoomed out
    bool prevZoomIn{false};
    bool prevZoomOut{false};
};

#endif // DDD_SYSTEMS_RENDER_SYSTEM_H

