#ifndef DDD_UTILS_VEC2_H
#define DDD_UTILS_VEC2_H

#include <SFML/System/Vector2.hpp>
#include <box2d/box2d.h>
#include <cmath>
#include <ostream>

struct Vec2 {
    float x{0.0f};
    float y{0.0f};

    Vec2() = default;
    Vec2(float xx, float yy) : x(xx), y(yy) {}

    Vec2(const sf::Vector2f &v) : x(v.x), y(v.y) {}
    Vec2(const sf::Vector2i &v) : x(static_cast<float>(v.x)), y(static_cast<float>(v.y)) {}
    Vec2(const b2Vec2 &v) : x(v.x), y(v.y) {}

    operator sf::Vector2f() const { return {x, y}; }
    operator b2Vec2() const { return {x, y}; }

    Vec2 operator+(const Vec2 &o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2 &o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2 &operator+=(const Vec2 &o) {
        x += o.x;
        y += o.y;
        return *this;
    }
    Vec2 &operator-=(const Vec2 &o) {
        x -= o.x;
        y -= o.y;
        return *this;
    }

    float length() const { return std::sqrt(x * x + y * y); }
    Vec2 normalized() const {
        float l = length();
        return l > 0.0f ? Vec2{x / l, y / l} : Vec2{};
    }
};

inline std::ostream &operator<<(std::ostream &os, const Vec2 &v) {
    os << "(" << v.x << ", " << v.y << ")";
    return os;
}

#endif // DDD_UTILS_VEC2_H

