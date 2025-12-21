#include "systems/RenderSystem.h"

#include "components/DropComponent.h"
#include "components/PhysicsBodyComponent.h"
#include "components/Tags.h"
#include "utils/Constants.h"
#include "utils/CoordinateUtils.h"
#include "utils/IsoCoordinate.h"
#include <algorithm>
#include <cmath>

RenderSystem::RenderSystem(WindowManager &windowMgr, CameraManager &cameraMgr, ResourceManager &resourceMgr,
                           EntityManager &entityMgr, DebugManager *debugMgr, IsoConfig isoCfg)
    : windowManager(windowMgr), cameraManager(cameraMgr), resourceManager(resourceMgr), entityManager(entityMgr),
      debugManager(debugMgr), isoConfig(isoCfg) {
    isoConfig.computeDirections();
}

void RenderSystem::update(float dt) {
    sf::RenderWindow &window = windowManager.getWindow();
    updateView();
    window.setView(windowManager.getView());
    window.clear(clearColor);

    std::vector<DrawCmd> draws;
    draws.reserve(entityManager.all().size() * 4);

    for (auto &entPtr : entityManager.all()) {
        auto *transform = entPtr->get<TransformComponent>();

        // Update minimal animation (idle/run) before enqueue.
        if (auto *anim = entPtr->get<SpriteAnimationComponent>()) {
            if (auto *sprite = entPtr->get<SpriteComponent>()) {
                anim->time += dt;
                float speed = 0.0f;
                if (auto *body = entPtr->get<PhysicsBodyComponent>(); body && body->body) {
                    const b2Vec2 v = body->body->GetLinearVelocity();
                    speed = std::sqrt(v.x * v.x + v.y * v.y);
                }
                anim->running = speed > anim->runSpeedThreshold;
                const auto &frames = anim->running ? anim->runFrames : anim->idleFrames;
                if (!frames.empty()) {
                    const int frameIdx =
                        static_cast<int>(std::floor(anim->time * anim->fps)) % static_cast<int>(frames.size());
                    sprite->atlasRegion = frames[frameIdx];
                    sprite->useTextureRect = false;
                }
            }
        }

        if (const auto *tilemap = entPtr->get<TilemapComponent>()) {
            if (tilemap->visible) {
                enqueueTilemap(*tilemap, transform, draws);
            }
        }

        if (const auto *sprite = entPtr->get<SpriteComponent>()) {
            if (sprite->visible) {
                const auto *shadow = entPtr->get<ShadowComponent>();
                enqueueSprite(*sprite, shadow, transform, entPtr->getId(), draws);
            }
        }
    }

    std::sort(draws.begin(), draws.end(), [](const DrawCmd &a, const DrawCmd &b) {
        if (std::abs(a.sortKey - b.sortKey) > 0.0001f)
            return a.sortKey < b.sortKey;
        if (std::abs(a.sortKeyX - b.sortKeyX) > 0.0001f)
            return a.sortKeyX < b.sortKeyX;
        if (a.z != b.z)
            return a.z < b.z;
        return a.id < b.id;
    });

    for (const auto &cmd : draws) {
        drawCmd(cmd, window);
    }

    updateMinimapControls();
    drawMinimap(window);
}

void RenderSystem::updateView() {
    sf::View view = windowManager.getView();
    const Vec2 renderCenter{0.0f, 0.0f}; // world center maps to origin in view
    const Vec2 viewportSize = cameraManager.getViewportSize();
    const float zoom = cameraManager.getZoom();

    view.setCenter(renderCenter.x, renderCenter.y);
    view.setSize(viewportSize.x * zoom, viewportSize.y * zoom);
    windowManager.setView(view);
}

void RenderSystem::enqueueTilemap(const TilemapComponent &tilemap, const TransformComponent *transform,
                                  std::vector<DrawCmd> &out) {
    if (tilemap.width <= 0 || tilemap.height <= 0 || tilemap.tiles.empty())
        return;

    const Vec2 transformPos = transform ? transform->position : Vec2{};
    const Vec2 base = tilemap.origin + transformPos;
    const Vec2 transformScale = transform ? transform->scale : Vec2{1.0f, 1.0f};

    const float isoW = ISO_TILE_WIDTH;
    const float isoH = ISO_TILE_HEIGHT;

    for (int y = 0; y < tilemap.height; ++y) {
        for (int x = 0; x < tilemap.width; ++x) {
            const int tileId = tilemap.get(x, y);
            if (tileId < 0)
                continue;

            const Vec2 worldPos{base.x + static_cast<float>(x) * tilemap.tileSize,
                                base.y - static_cast<float>(y) * tilemap.tileSize};

            DrawCmd cmd{};
            cmd.tilemap = &tilemap;
            cmd.transform = transform;
            cmd.worldPos = worldPos;
            cmd.id = static_cast<Entity::Id>((y * tilemap.width) + x);
            cmd.z = tilemap.z;
            cmd.tileId = tileId;
            // Sort by iso screen y to get proper overlap.
            const Vec2 isoPos = IsoCoordinate::worldToScreen(worldPos, cameraManager, isoConfig);
            cmd.sortKey = isoPos.y;
            cmd.sortKeyX = isoPos.x;
            // Precompute sprite scale factors via z field? Will compute at draw.
            out.push_back(std::move(cmd));
        }
    }
}

void RenderSystem::enqueueSprite(const SpriteComponent &spriteComp, const ShadowComponent *shadow,
                                 const TransformComponent *transform, Entity::Id id, std::vector<DrawCmd> &out) {
    DrawCmd cmd{};
    cmd.sprite = &spriteComp;
    cmd.shadow = shadow;
    cmd.transform = transform;
    cmd.worldPos = transform ? transform->position : Vec2{};
    cmd.id = id;
    cmd.z = spriteComp.z;
    const Vec2 isoPos = IsoCoordinate::worldToScreen(cmd.worldPos, cameraManager, isoConfig);
    cmd.sortKey = isoPos.y + static_cast<float>(spriteComp.z) * 0.001f;
    cmd.sortKeyX = isoPos.x;

    if (shadow && shadow->visible) {
        enqueueShadow(*shadow, transform, id, cmd.sortKey - 0.0005f, cmd.sortKeyX, out);
    }
    out.push_back(std::move(cmd));
}

void RenderSystem::enqueueShadow(const ShadowComponent &shadow, const TransformComponent *transform, Entity::Id id, float sortKey,
                                 float sortKeyX, std::vector<DrawCmd> &out) {
    DrawCmd cmd{};
    cmd.isShadow = true;
    cmd.shadow = &shadow;
    cmd.transform = transform;
    cmd.worldPos = transform ? transform->position : Vec2{};
    cmd.id = id;
    cmd.z = -10; // always under regular sprites/tiles
    cmd.sortKey = sortKey;
    cmd.sortKeyX = sortKeyX;
    out.push_back(std::move(cmd));
}

void RenderSystem::drawCmd(const DrawCmd &cmd, sf::RenderWindow &window) {
    const auto fallbackColor = [](int id) {
        const uint32_t h = static_cast<uint32_t>(id * 2654435761u);
        return sf::Color(static_cast<uint8_t>(0x40 | (h & 0x3F)), static_cast<uint8_t>(0x40 | ((h >> 6) & 0x3F)),
                         static_cast<uint8_t>(0x40 | ((h >> 12) & 0x3F)));
    };

    if (cmd.isShadow && cmd.shadow && cmd.transform) {
        const Vec2 isoPos = IsoCoordinate::worldToScreen(cmd.worldPos, cameraManager, isoConfig);
        sf::CircleShape c;
        c.setRadius(cmd.shadow->radiusX);
        c.setOrigin(cmd.shadow->radiusX, cmd.shadow->radiusX);
        c.setPosition(isoPos.x, isoPos.y + cmd.shadow->offsetY);
        c.setScale(1.0f, cmd.shadow->radiusY / cmd.shadow->radiusX);
        c.setFillColor(cmd.shadow->color);
        window.draw(c);
        return;
    }

    const bool isTile = cmd.tilemap != nullptr;
    if (isTile) {
        const TilemapComponent &tilemap = *cmd.tilemap;
        const Vec2 transformScale = cmd.transform ? cmd.transform->scale : Vec2{1.0f, 1.0f};
        int tileId = cmd.tileId;
        if (tileId < 0) {
            const int idx = static_cast<int>(cmd.id);
            if (idx >= 0 && idx < static_cast<int>(tilemap.tiles.size()))
                tileId = tilemap.tiles[idx];
        }
        if (tileId < 0)
            return;
        const Vec2 isoPos = IsoCoordinate::worldToScreen(cmd.worldPos, cameraManager, isoConfig);
        const float desiredW = isoConfig.screenScale * tilemap.tileSize;
        const float desiredH = isoConfig.screenScale * tilemap.tileSize * 0.5f; // classic iso ratio (2:1)

        bool drawn = false;
        auto regionIt = tilemap.tileIdToRegion.find(tileId);
        if (regionIt != tilemap.tileIdToRegion.end()) {
            const std::string &regionName = regionIt->second;
            if (resourceManager.hasAtlasRegion(regionName)) {
                const ResourceManager::AtlasRegion &region = resourceManager.getAtlasRegion(regionName);
                if (resourceManager.hasTexture(region.textureName)) {
                    sf::Sprite sprite;
                    sprite.setTexture(resourceManager.getTexture(region.textureName));
                    sprite.setTextureRect(region.rect);
                    sprite.setPosition(isoPos.x, isoPos.y);
                    const float sx = desiredW / static_cast<float>(region.rect.width);
                    const float sy = desiredH / static_cast<float>(region.rect.height);
                    sprite.setScale(sx * transformScale.x, sy * transformScale.y);
                    sprite.setOrigin(static_cast<float>(region.rect.width) * 0.5f,
                                     static_cast<float>(region.rect.height) * 0.5f);
                    window.draw(sprite);
                    drawn = true;
                }
            }
        }

        if (!drawn) {
            // Fallback: simple diamond-ish rect.
            sf::RectangleShape rect;
            rect.setSize(sf::Vector2f(desiredW * transformScale.x, desiredH * transformScale.y));
            rect.setOrigin(rect.getSize().x * 0.5f, rect.getSize().y * 0.5f);
            rect.setPosition(isoPos.x, isoPos.y);
            rect.setFillColor(fallbackColor(tileId));
            window.draw(rect);
        }
        drawAnchorDebug(isoPos, true, window);
        return;
    }

    // Sprite path
    const SpriteComponent &spriteComp = *cmd.sprite;
    const bool hasAtlas = !spriteComp.atlasRegion.empty() && resourceManager.hasAtlasRegion(spriteComp.atlasRegion);
    const bool hasTexture = !spriteComp.textureName.empty() && resourceManager.hasTexture(spriteComp.textureName);
    const Vec2 isoPos = IsoCoordinate::worldToScreen(cmd.worldPos, cameraManager, isoConfig);
    const Vec2 scale = cmd.transform ? cmd.transform->scale : Vec2{1.0f, 1.0f};

    if (hasAtlas || hasTexture) {
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
        sprite.setPosition(isoPos.x, isoPos.y);
        sprite.setScale(scale.x * spriteComp.scale.x, scale.y * spriteComp.scale.y);
        sprite.setRotation(worldAngleToRender(cmd.transform ? cmd.transform->rotationDeg : 0.0f));
        sprite.setOrigin(spriteComp.origin.x, spriteComp.origin.y);
        window.draw(sprite);
    } else {
        // Fallback: simple square.
        sf::RectangleShape rect;
        rect.setSize(sf::Vector2f(24.0f, 24.0f));
        rect.setOrigin(12.0f, 12.0f);
        rect.setPosition(isoPos.x, isoPos.y);
        rect.setFillColor(fallbackColor(static_cast<int>(cmd.id)));
        rect.setScale(scale.x * spriteComp.scale.x, scale.y * spriteComp.scale.y);
        window.draw(rect);
    }

    drawAnchorDebug(isoPos, false, window);
}

void RenderSystem::drawAnchorDebug(const Vec2 &isoPos, bool isTile, sf::RenderWindow &window) const {
    if (!debugManager || !debugManager->isVisible())
        return;

    sf::CircleShape c;
    c.setRadius(3.0f);
    c.setOrigin(3.0f, 3.0f);
    c.setPosition(isoPos.x, isoPos.y);
    c.setFillColor(isTile ? sf::Color(80, 170, 255, 180) : sf::Color(255, 120, 200, 200));
    window.draw(c);
}

void RenderSystem::updateMinimapControls() {
    // Zoom controls: '+' and '-' (also support numpad add/subtract).
    const bool zoomIn = sf::Keyboard::isKeyPressed(sf::Keyboard::Add) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Equal);
    const bool zoomOut = sf::Keyboard::isKeyPressed(sf::Keyboard::Subtract) ||
                         sf::Keyboard::isKeyPressed(sf::Keyboard::Hyphen);

    if (zoomIn && !prevZoomIn) {
        minimapZoom = std::max(0.25f, minimapZoom * 0.9f);
    }
    if (zoomOut && !prevZoomOut) {
        minimapZoom = std::min(10.0f, minimapZoom * 1.1f);
    }

    prevZoomIn = zoomIn;
    prevZoomOut = zoomOut;
}

void RenderSystem::drawMinimap(sf::RenderWindow &window) const {
    // Debug minimap: top-down world view in screen corner.
    if (!debugManager || !debugManager->isVisible())
        return;

    // Normalized viewport in the window: right-top corner.
    const sf::FloatRect vp{0.70f, 0.02f, 0.28f, 0.28f};
    const sf::Vector2u ws = window.getSize();
    const float vpPxW = vp.width * static_cast<float>(ws.x);
    const float vpPxH = vp.height * static_cast<float>(ws.y);
    const float aspect = vpPxW / std::max(1.0f, vpPxH);

    // Center on the camera target (player-follow camera).
    const Vec2 centerRender = worldToRender(cameraManager.getCenter());

    sf::View v;
    v.setViewport(vp);
    v.setCenter(centerRender.x, centerRender.y);

    // Base size in world units, then scaled by zoom.
    const float baseWorldHeight = 24.0f; // world units visible vertically at zoom=1
    const float heightPx = baseWorldHeight * RENDER_SCALE * minimapZoom;
    const float widthPx = heightPx * aspect;
    v.setSize(widthPx, heightPx);

    window.setView(v);

    // Draw physics bodies as simple top-down primitives.
    for (const auto &entPtr : entityManager.all()) {
        const auto *body = entPtr->get<PhysicsBodyComponent>();
        const auto *transform = entPtr->get<TransformComponent>();
        if (!body || !transform)
            continue;

        const Vec2 posPx = worldToRender(transform->position);

        // Color coding by tags/components
        sf::Color col(200, 200, 200, 190);
        if (entPtr->has<PlayerTag>())
            col = sf::Color(90, 220, 120, 220);
        if (entPtr->get<DropComponent>())
            col = sf::Color(240, 220, 120, 220);
        if (body->bodyType == b2_staticBody)
            col = sf::Color(180, 180, 200, 200);

        if (body->fixture.shape == PhysicsShapeType::Circle) {
            const float r = body->fixture.radius * RENDER_SCALE;
            sf::CircleShape c;
            c.setRadius(r);
            c.setOrigin(r, r);
            c.setPosition(posPx.x, posPx.y);
            c.setFillColor(sf::Color(col.r, col.g, col.b, 60));
            c.setOutlineThickness(2.0f);
            c.setOutlineColor(col);
            window.draw(c);
        } else {
            // Box/polygon rendered as bbox using size for now (good enough for debug).
            const Vec2 sizePx = Vec2{body->fixture.size.x * RENDER_SCALE, body->fixture.size.y * RENDER_SCALE};
            sf::RectangleShape rect;
            rect.setSize(sf::Vector2f(sizePx.x, sizePx.y));
            rect.setOrigin(sizePx.x * 0.5f, sizePx.y * 0.5f);
            rect.setPosition(posPx.x, posPx.y);
            rect.setFillColor(sf::Color(col.r, col.g, col.b, 50));
            rect.setOutlineThickness(2.0f);
            rect.setOutlineColor(col);
            window.draw(rect);
        }
    }

    // Restore main view, then draw minimap frame in screen space.
    window.setView(windowManager.getView());
    window.setView(window.getDefaultView());

    const float x = vp.left * ws.x;
    const float y = vp.top * ws.y;
    const float w = vp.width * ws.x;
    const float h = vp.height * ws.y;

    sf::RectangleShape frame;
    frame.setPosition(x, y);
    frame.setSize({w, h});
    frame.setFillColor(sf::Color(0, 0, 0, 70));
    frame.setOutlineThickness(2.0f);
    frame.setOutlineColor(sf::Color(120, 150, 190, 200));
    window.draw(frame);
}
