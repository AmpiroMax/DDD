#ifndef DDD_MANAGERS_TIME_MANAGER_H
#define DDD_MANAGERS_TIME_MANAGER_H

#include <SFML/System/Clock.hpp>

class TimeManager {
  public:
    float tick() { return clock.restart().asSeconds(); }

  private:
    sf::Clock clock;
};

#endif // DDD_MANAGERS_TIME_MANAGER_H

