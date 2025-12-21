#include "systems/DebugSystem.h"

#include <box2d/box2d.h>
#include <cmath>
#include <fstream>
#include <sstream>

DebugSystem::DebugSystem(EntityManager &entityMgr, DebugManager &debugMgr, InputSystem &inputSys)
    : entityManager(entityMgr), debugManager(debugMgr), inputSystem(inputSys) {}

void DebugSystem::update(float dt) {
    (void)dt;

    if (!debugManager.isEnabled())
        return;

    debugManager.clearSection("mechanics");
    debugManager.clearSection("physics");
    std::vector<std::string> lines;
    std::vector<std::string> physLines;

    for (auto &entPtr : entityManager.all()) {
        if (!entPtr->has<PlayerTag>())
            continue;

        const auto *transform = entPtr->get<TransformComponent>();
        const auto *bodyComp = entPtr->get<PhysicsBodyComponent>();
        const auto *grounded = entPtr->get<GroundedComponent>();

        if (transform)
            lines.push_back("Pos: (" + std::to_string(transform->position.x) + ", " + std::to_string(transform->position.y) + ")");

        if (bodyComp && bodyComp->body) {
            const b2Vec2 vel = bodyComp->body->GetLinearVelocity();
            lines.push_back("Vel: (" + std::to_string(vel.x) + ", " + std::to_string(vel.y) + ")");
        }

        if (grounded)
            lines.push_back(std::string("Grounded: ") + (grounded->grounded ? "yes" : "no"));

        break; // only first player
    }

    if (debugManager.isSourceEnabled("mechanics"))
        debugManager.setSection("mechanics", lines);

    // Tile / physics debug
    TilemapComponent *tilemap = nullptr;
    for (auto &entPtr : entityManager.all()) {
        tilemap = entPtr->get<TilemapComponent>();
        if (tilemap)
            break;
    }

    InputComponent *input = inputSystem.getInput();
    if (tilemap && input) {
        const Vec2 mw = input->mouseWorld;
        const float localX = (mw.x - tilemap->origin.x) / tilemap->tileSize;
        const float localY = (tilemap->origin.y - mw.y) / tilemap->tileSize;
        const int tx = static_cast<int>(std::floor(localX));
        const int ty = static_cast<int>(std::floor(localY));
        std::ostringstream oss;
        oss << "Mouse world: (" << mw.x << ", " << mw.y << ")";
        physLines.push_back(oss.str());
        std::ostringstream oss2;
        oss2 << "Tile: (" << tx << ", " << ty << ")";
        if (tilemap->inBounds(tx, ty)) {
            const int tid = tilemap->get(tx, ty);
            oss2 << " id=" << tid << (tilemap->isSolid(tid) ? " solid" : " air");
        } else {
            oss2 << " out_of_bounds";
        }
        physLines.push_back(oss2.str());
    }

    int dropCount = 0;
    int dumpLimit = 5;
    for (auto &entPtr : entityManager.all()) {
        auto *drop = entPtr->get<DropComponent>();
        if (!drop)
            continue;
        ++dropCount;
        if (dumpLimit > 0) {
            const auto *t = entPtr->get<TransformComponent>();
            const auto *b = entPtr->get<PhysicsBodyComponent>();
            std::ostringstream d;
            d << "drop#" << dropCount << " id=" << drop->itemId;
            if (t)
                d << " pos=(" << t->position.x << "," << t->position.y << ")";
            if (b && b->body) {
                const b2Vec2 v = b->body->GetLinearVelocity();
                d << " vel=(" << v.x << "," << v.y << ")" << (b->body->IsAwake() ? " awake" : " sleep");
                // Tile under drop
                if (tilemap && t) {
                    const float lx = (t->position.x - tilemap->origin.x) / tilemap->tileSize;
                    const float ly = (tilemap->origin.y - t->position.y) / tilemap->tileSize;
                    const int tx = static_cast<int>(std::floor(lx));
                    const int ty = static_cast<int>(std::floor(ly));
                    d << " tile=(" << tx << "," << ty << ")";
                    if (tilemap->inBounds(tx, ty)) {
                        const int tid = tilemap->get(tx, ty);
                        d << " tid=" << tid << (tilemap->isSolid(tid) ? " solid" : " air");
                    } else {
                        d << " oob";
                    }
                }
            }
            physLines.push_back(d.str());
            --dumpLimit;
        }
    }
    physLines.push_back("Drops total: " + std::to_string(dropCount));

    if (debugManager.isSourceEnabled("physics"))
        debugManager.setSection("physics", physLines);

    maybeLogToFile(lines, physLines);
}

void DebugSystem::maybeLogToFile(const std::vector<std::string> &mechanicsLines,
                                 const std::vector<std::string> &physicsLines) {
    ++frameCounter;
    if (frameCounter % 1000 != 0)
        return;

    std::ofstream out(logPath, std::ios::app);
    if (!out.is_open())
        return;

    out << "==== Frame " << frameCounter << " ====\n";
    out << "[mechanics]\n";
    for (const auto &l : mechanicsLines)
        out << l << "\n";
    out << "[physics]\n";
    for (const auto &l : physicsLines)
        out << l << "\n";
    out << "\n";
}

