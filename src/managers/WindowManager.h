#ifndef DDD_MANAGERS_WINDOW_MANAGER_H
#define DDD_MANAGERS_WINDOW_MANAGER_H

#include "utils/Vec2.h"
#include <SFML/Graphics.hpp>
#include <string>

class WindowManager {
  public:
    WindowManager() = default;

    void create(int width, int height, const std::string &title) {
        window.create(sf::VideoMode(width, height), title);
        view = window.getDefaultView();
    }

    sf::RenderWindow &getWindow() { return window; }
    const sf::RenderWindow &getWindow() const { return window; }

    void setView(const sf::View &v) {
        view = v;
        window.setView(view);
    }

    sf::View &getView() { return view; }
    const sf::View &getView() const { return view; }

    Vec2 size() const {
        auto s = window.getSize();
        return {static_cast<float>(s.x), static_cast<float>(s.y)};
    }

  private:
    sf::RenderWindow window;
    sf::View view;
};

#endif // DDD_MANAGERS_WINDOW_MANAGER_H

