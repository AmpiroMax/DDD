#ifndef DDD_MANAGERS_DEBUG_MANAGER_H
#define DDD_MANAGERS_DEBUG_MANAGER_H

#include <string>

class DebugManager {
  public:
    void setString(const std::string &s) { text = s; }
    const std::string &getString() const { return text; }

    void setVisible(bool v) { visible = v; }
    bool isVisible() const { return visible; }

  private:
    std::string text;
    bool visible{true};
};

#endif // DDD_MANAGERS_DEBUG_MANAGER_H

