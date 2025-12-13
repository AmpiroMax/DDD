#ifndef DDD_MANAGERS_PHYSICS_MANAGER_H
#define DDD_MANAGERS_PHYSICS_MANAGER_H

#include "physics/PhysicsDefs.h"
#include "utils/CoordinateUtils.h"
#include <algorithm>
#include <cstdint>
#include <box2d/box2d.h>
#include <new>

class PhysicsManager {
  public:
    PhysicsManager() : world(b2Vec2(0.0f, -9.8f)) {}

    b2World &getWorld() { return world; }
    void resetWorld() {
        world.~b2World();
        new (&world) b2World(b2Vec2(0.0f, -9.8f));
    }

    b2Body *createBody(b2BodyType type, const Vec2 &worldPos, float angleDeg, bool canRotate, float linearDamping,
                       float angularDamping) {
        b2BodyDef def;
        def.type = type;
        def.position = worldToPhysics(worldPos);
        def.angle = worldAngleToPhysics(angleDeg);
        def.fixedRotation = !canRotate;
        def.linearDamping = linearDamping;
        def.angularDamping = angularDamping;
        return world.CreateBody(&def);
    }

    b2Fixture *createFixture(b2Body &body, const PhysicsFixtureConfig &cfg, FixtureTag *tag) {
        b2FixtureDef fdef;
        fdef.density = cfg.density;
        fdef.friction = cfg.friction;
        fdef.restitution = cfg.restitution;
        fdef.isSensor = cfg.isSensor;
        fdef.userData.pointer = reinterpret_cast<uintptr_t>(tag);

        b2PolygonShape boxShape;
        b2CircleShape circleShape;
        b2PolygonShape polygonShape;

        switch (cfg.shape) {
        case PhysicsShapeType::Box: {
            const Vec2 half = worldToPhysics(cfg.size) * 0.5f;
            boxShape.SetAsBox(half.x, half.y);
            fdef.shape = &boxShape;
            break;
        }
        case PhysicsShapeType::Circle: {
            circleShape.m_radius = worldToPhysics(Vec2{cfg.radius, cfg.radius}).x;
            fdef.shape = &circleShape;
            break;
        }
        case PhysicsShapeType::Polygon: {
            const std::size_t count =
                std::min(cfg.vertices.size(), static_cast<std::size_t>(b2_maxPolygonVertices));
            if (count >= 3) {
                b2Vec2 verts[b2_maxPolygonVertices];
                for (std::size_t i = 0; i < count; ++i) {
                    Vec2 p = worldToPhysics(cfg.vertices[i]);
                    verts[i] = {p.x, p.y};
                }
                polygonShape.Set(verts, static_cast<int32>(count));
                fdef.shape = &polygonShape;
                break;
            }
            // Fallback to box if polygon data is invalid
            const Vec2 half = worldToPhysics(cfg.size) * 0.5f;
            boxShape.SetAsBox(half.x, half.y);
            fdef.shape = &boxShape;
            break;
        }
        }

        return body.CreateFixture(&fdef);
    }

    void destroyBody(b2Body *body) {
        if (body)
            world.DestroyBody(body);
    }

  private:
    b2World world;
};

#endif // DDD_MANAGERS_PHYSICS_MANAGER_H

