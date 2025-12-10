#include "systems/RenderSystem.h"

#include "utils/Constants.h"
#include "utils/CoordinateUtils.h"
#include <algorithm>

RenderSystem::RenderSystem(WindowManager &windowMgr, CameraManager &cameraMgr, ResourceManager &resourceMgr, EntityManager &entityMgr)
    : windowManager(windowMgr), cameraManager(cameraMgr), resourceManager(resourceMgr), entityManager(entityMgr) {}

void RenderSystem::update(float dt) {
    (void)dt;

    sf::RenderWindow &window = windowManager.getWindow();
    updateView();
    window.setView(windowManager.getView());
    window.clear(clearColor);

    std::vector<TilemapDraw> tileDraws;
    std::vector<SpriteDraw> spriteDraws;
    tileDraws.reserve(entityManager.all().size());
    spriteDraws.reserve(entityManager.all().size());

    for (auto &entPtr : entityManager.all()) {
        const TransformComponent *transform = entPtr->get<TransformComponent>();

        if (const auto *tilemap = entPtr->get<TilemapComponent>()) {
            if (tilemap->visible) {
                tileDraws.push_back(TilemapDraw{tilemap->z, entPtr->getId(), tilemap, transform});
            }
        }

        if (const auto *sprite = entPtr->get<SpriteComponent>()) {
            if (sprite->visible) {
                spriteDraws.push_back(SpriteDraw{sprite->z, entPtr->getId(), sprite, transform});
            }
        }
    }

    const auto byZ = [](const auto &a, const auto &b) {
        if (a.z == b.z)
            return a.id < b.id;
        return a.z < b.z;
    };
    std::sort(tileDraws.begin(), tileDraws.end(), byZ);
    std::sort(spriteDraws.begin(), spriteDraws.end(), byZ);

    for (const auto &cmd : tileDraws) {
        drawTilemap(*cmd.tilemap, cmd.transform, window);
    }

    for (const auto &cmd : spriteDraws) {
        drawSprite(*cmd.sprite, cmd.transform, window);
    }
}

void RenderSystem::updateView() {
    sf::View view = windowManager.getView();
    const Vec2 renderCenter = worldToRender(cameraManager.getCenter());
    const Vec2 viewportSize = cameraManager.getViewportSize();
    const float zoom = cameraManager.getZoom();

    view.setCenter(renderCenter.x, renderCenter.y);
    view.setSize(viewportSize.x * zoom, viewportSize.y * zoom);
    windowManager.setView(view);
}

void RenderSystem::drawTilemap(const TilemapComponent &tilemap, const TransformComponent *transform, sf::RenderWindow &window) {
    if (tilemap.width <= 0 || tilemap.height <= 0 || tilemap.tiles.empty())
        return;

    const Vec2 transformPos = transform ? transform->position : Vec2{};
    const Vec2 base = tilemap.origin + transformPos;
    const Vec2 transformScale = transform ? transform->scale : Vec2{1.0f, 1.0f};

    for (int y = 0; y < tilemap.height; ++y) {
        for (int x = 0; x < tilemap.width; ++x) {
            const int tileId = tilemap.get(x, y);
            if (tileId < 0)
                continue;

            const auto regionIt = tilemap.tileIdToRegion.find(tileId);
            if (regionIt == tilemap.tileIdToRegion.end())
                continue;
            const std::string &regionName = regionIt->second;
            if (!resourceManager.hasAtlasRegion(regionName))
                continue;

            const ResourceManager::AtlasRegion &region = resourceManager.getAtlasRegion(regionName);
            if (!resourceManager.hasTexture(region.textureName))
                continue;

            sf::Sprite sprite;
            sprite.setTexture(resourceManager.getTexture(region.textureName));
            sprite.setTextureRect(region.rect);

            const Vec2 worldPos{base.x + static_cast<float>(x) * tilemap.tileSize,
                                base.y - static_cast<float>(y) * tilemap.tileSize};
            const Vec2 renderPos = worldToRender(worldPos);
            sprite.setPosition(renderPos.x, renderPos.y);

            const float scaleX = (tilemap.tileSize * RENDER_SCALE) / static_cast<float>(region.rect.width);
            const float scaleY = (tilemap.tileSize * RENDER_SCALE) / static_cast<float>(region.rect.height);
            sprite.setScale(scaleX * transformScale.x, scaleY * transformScale.y);

            window.draw(sprite);
        }
    }
}

void RenderSystem::drawSprite(const SpriteComponent &spriteComp, const TransformComponent *transform, sf::RenderWindow &window) {
    const bool hasAtlas = !spriteComp.atlasRegion.empty() && resourceManager.hasAtlasRegion(spriteComp.atlasRegion);
    const bool hasTexture = !spriteComp.textureName.empty() && resourceManager.hasTexture(spriteComp.textureName);
    if (!hasAtlas && !hasTexture)
        return;

    sf::Sprite sprite;
    if (hasAtlas) {
        const auto &region = resourceManager.getAtlasRegion(spriteComp.atlasRegion);
        if (!resourceManager.hasTexture(region.textureName))
            return;
        sprite.setTexture(resourceManager.getTexture(region.textureName));
        sprite.setTextureRect(region.rect);
    } else {
        sprite.setTexture(resourceManager.getTexture(spriteComp.textureName));
        if (spriteComp.useTextureRect)
            sprite.setTextureRect(spriteComp.textureRect);
    }

    if (transform) {
        const Vec2 renderPos = worldToRender(transform->position);
        sprite.setPosition(renderPos.x, renderPos.y);
        sprite.setRotation(worldAngleToRender(transform->rotationDeg));
        sprite.setScale(transform->scale.x * spriteComp.scale.x, transform->scale.y * spriteComp.scale.y);
    } else {
        sprite.setScale(spriteComp.scale.x, spriteComp.scale.y);
    }

    sprite.setOrigin(spriteComp.origin.x, spriteComp.origin.y);
    window.draw(sprite);
}

