#ifndef DDD_SYSTEMS_UI_RENDER_SYSTEM_H
#define DDD_SYSTEMS_UI_RENDER_SYSTEM_H

#include "core/System.h"
#include "core/EventBus.h"
#include "managers/DebugManager.h"
#include "managers/ResourceManager.h"
#include "managers/WindowManager.h"
#include "events/InventoryEvents.h"
#include "events/GameEvents.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class UIRenderSystem : public System {
  public:
    struct MenuRenderButton {
        std::string id;
        std::string label;
        sf::FloatRect rect;
        bool focused{false};
    };

    enum class MenuScreen { Main, MapSelect, Pause, Settings };

    struct MenuRenderState {
        bool visible{false};
        MenuScreen screen{MenuScreen::Main};
        std::vector<MenuRenderButton> buttons;
        std::vector<std::string> maps;
        int selectedMap{-1};
        bool hasSave{false};
        bool canResume{false};
        bool showMenuButton{false};
        sf::FloatRect menuButtonRect;
        bool showDebugButton{false};
        sf::FloatRect debugButtonRect;
        std::vector<std::string> settingsLines;
    };

    struct PlayerRow {
        std::string name;
        int kills{0};
        int deaths{0};
    };

    struct KillFeedEntry {
        std::string killer;
        std::string victim;
        std::string weapon;
        float ttl{5.0f};
    };

    UIRenderSystem(WindowManager &windowMgr, ResourceManager &resourceMgr, DebugManager &debugMgr, EventBus &eventBus);
    ~UIRenderSystem() override { shutdown(); }
    void update(float dt) override;
    void shutdown() override;
    void setMenuState(const MenuRenderState *state) { menuState = state; }
    void setShowScoreboard(bool v) { showScoreboard = v; }
    bool isScoreboardVisible() const { return showScoreboard; }

  private:
    struct UISlot {
        int itemId{-1};
        int count{0};
        std::string iconTexture;
        std::string iconRegion;
    };

    void drawDebugOverlay(float dt);
    void drawInventoryUI();
    void drawMenuUI();
    void drawMenuButton();
    void drawDebugButton();
    void handleInventoryStateChanged(const InventoryStateChangedEvent &ev);
    void handleMatchState(const MatchStateEvent &ev);
    void handlePlayerStats(const PlayerStatsEvent &ev);
    void handleKillEvent(const KillEvent &ev);
    void handleRespawnTimer(const RespawnTimerEvent &ev);
    void drawHUD(float dt);
    void drawScoreboard();
    void drawRespawn();
    void drawKillfeed(float dt);

    WindowManager &windowManager;
    ResourceManager &resourceManager;
    DebugManager &debugManager;
    EventBus &eventBus;
    std::vector<EventBus::SubscriptionToken> subscriptions_;

    std::string debugFontName{"debug"};
    sf::Color debugTextColor{sf::Color::White};
    unsigned int debugCharacterSize{14};

    std::vector<UISlot> slots;
    int activeIndex{0};
    bool hasInventory{false};

    const MenuRenderState *menuState{nullptr};

    // Match UI state
    MatchStateEvent::Phase matchPhase{MatchStateEvent::Phase::PreMatch};
    float matchTime{0.0f};
    int scoreA{0};
    int scoreB{0};
    std::vector<PlayerRow> players;
    std::vector<KillFeedEntry> killfeed;
    float respawnTime{0.0f};
    bool canRespawn{false};
    bool showRespawn{false};
    bool showScoreboard{false};
};

#endif // DDD_SYSTEMS_UI_RENDER_SYSTEM_H

