#ifndef DDD_SYSTEMS_RENDER_SYSTEM_H
#define DDD_SYSTEMS_RENDER_SYSTEM_H

#include "core/System.h"
#include "managers/CameraManager.h"
#include "managers/ResourceManager.h"
#include "managers/WindowManager.h"
#include "core/EntityManager.h"
#include "components/SpriteComponent.h"
#include "components/TilemapComponent.h"
#include "components/TransformComponent.h"
#include <SFML/Graphics.hpp>
#include <vector>

class RenderSystem : public System {
  public:
    RenderSystem(WindowManager &windowMgr, CameraManager &cameraMgr, ResourceManager &resourceMgr, EntityManager &entityMgr);
    void update(float dt) override;

  private:
    struct SpriteDraw {
        int z{0};
        Entity::Id id{0};
        const SpriteComponent *sprite{nullptr};
        const TransformComponent *transform{nullptr};
    };

    struct TilemapDraw {
        int z{0};
        Entity::Id id{0};
        const TilemapComponent *tilemap{nullptr};
        const TransformComponent *transform{nullptr};
    };

    void updateView();
    void drawTilemap(const TilemapComponent &tilemap, const TransformComponent *transform, sf::RenderWindow &window);
    void drawSprite(const SpriteComponent &spriteComp, const TransformComponent *transform, sf::RenderWindow &window);

    WindowManager &windowManager;
    CameraManager &cameraManager;
    ResourceManager &resourceManager;
    EntityManager &entityManager;

    sf::Color clearColor{sf::Color::Black};
};

#endif // DDD_SYSTEMS_RENDER_SYSTEM_H

