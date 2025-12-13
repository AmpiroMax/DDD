#ifndef DDD_MANAGERS_DEBUG_MANAGER_H
#define DDD_MANAGERS_DEBUG_MANAGER_H

#include <string>
#include <unordered_map>
#include <vector>

class DebugManager {
  public:
    void setString(const std::string &s) { text = s; }
    const std::string &getString() const { return text; }

    void setVisible(bool v) { visible = v; }
    bool isVisible() const { return visible; }

    // Structured per-source debug streams (for UI/overlay consumption).
    // Usage:
    //   debugManager.appendLine("mechanics", "speed=3.2");
    //   debugManager.setSection("physics", {"bodies=12", "contacts=3"});
    // UI can call getStreams() to render grouped lines per source.
    void appendLine(const std::string &source, const std::string &line) { streams[source].push_back(line); }
    void setSection(const std::string &source, const std::vector<std::string> &lines) { streams[source] = lines; }
    void clearSection(const std::string &source) { streams.erase(source); }
    const std::unordered_map<std::string, std::vector<std::string>> &getStreams() const { return streams; }
    void clearAllSections() { streams.clear(); }

  private:
    std::string text;
    bool visible{true};
    std::unordered_map<std::string, std::vector<std::string>> streams;
};

#endif // DDD_MANAGERS_DEBUG_MANAGER_H

