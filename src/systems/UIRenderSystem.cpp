#include "systems/UIRenderSystem.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

UIRenderSystem::UIRenderSystem(WindowManager &windowMgr, ResourceManager &resourceMgr, DebugManager &debugMgr, EventBus &eventBus)
    : windowManager(windowMgr), resourceManager(resourceMgr), debugManager(debugMgr), eventBus(eventBus) {
    subscriptions_.push_back(eventBus.subscribeWithToken<InventoryStateChangedEvent>(
        [this](const InventoryStateChangedEvent &ev) { handleInventoryStateChanged(ev); }));
    subscriptions_.push_back(eventBus.subscribeWithToken<MatchStateEvent>(
        [this](const MatchStateEvent &ev) { handleMatchState(ev); }));
    subscriptions_.push_back(
        eventBus.subscribeWithToken<PlayerStatsEvent>([this](const PlayerStatsEvent &ev) { handlePlayerStats(ev); }));
    subscriptions_.push_back(eventBus.subscribeWithToken<KillEvent>([this](const KillEvent &ev) { handleKillEvent(ev); }));
    subscriptions_.push_back(
        eventBus.subscribeWithToken<RespawnTimerEvent>([this](const RespawnTimerEvent &ev) { handleRespawnTimer(ev); }));
}

void UIRenderSystem::shutdown() {
    for (const auto &tok : subscriptions_)
        eventBus.unsubscribe(tok);
    subscriptions_.clear();
}

void UIRenderSystem::update(float dt) {
    sf::RenderWindow &window = windowManager.getWindow();
    window.setView(window.getDefaultView());

    drawInventoryUI();
    drawHUD(dt);
    drawMenuButton();
    drawMenuUI();
    drawDebugOverlay(dt);
    window.display();
}

void UIRenderSystem::drawDebugOverlay(float dt) {
    if (!debugManager.isVisible())
        return;
    if (!resourceManager.hasFont(debugFontName))
        return;

    sf::RenderWindow &window = windowManager.getWindow();

    std::vector<std::string> lines;
    lines.reserve(16);

    std::ostringstream fpss;
    fpss.setf(std::ios::fixed);
    fpss << std::setprecision(1);
    const float fps = dt > 0.0001f ? 1.0f / dt : 0.0f;
    fpss << "FPS: " << fps;
    lines.push_back(fpss.str());

    const auto &streams = debugManager.getStreams();
    if (!streams.empty()) {
        for (const auto &kv : streams) {
            if (!debugManager.isSourceEnabled(kv.first))
                continue;
            lines.push_back(kv.first + ":");
            for (const auto &ln : kv.second) {
                lines.push_back("  " + ln);
            }
        }
    } else {
        const std::string &debugStr = debugManager.getString();
        if (!debugStr.empty())
            lines.push_back(debugStr);
    }

    sf::Font &font = resourceManager.getFont(debugFontName);
    const unsigned int size = debugCharacterSize;

    float maxWidth = 0.0f;
    sf::Text measure;
    measure.setFont(font);
    measure.setCharacterSize(size);
    for (const auto &ln : lines) {
        measure.setString(ln);
        maxWidth = std::max(maxWidth, measure.getLocalBounds().width);
    }
    const float lineHeight = static_cast<float>(size) + 4.0f;
    const float height = lineHeight * static_cast<float>(lines.size()) + 12.0f;
    const float width = maxWidth + 16.0f;

    sf::RectangleShape bg;
    bg.setPosition(8.0f, 8.0f);
    bg.setSize({width, height});
    bg.setFillColor(sf::Color(0, 0, 0, 140));
    bg.setOutlineThickness(1.0f);
    bg.setOutlineColor(sf::Color(80, 80, 80, 180));
    window.draw(bg);

    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(size);
    text.setFillColor(debugTextColor);

    float y = 12.0f;
    for (const auto &ln : lines) {
        text.setString(ln);
        text.setPosition(14.0f, y);
        window.draw(text);
        y += lineHeight;
    }
}

void UIRenderSystem::drawMenuButton() {
    if (!menuState || !menuState->showMenuButton)
        return;
    sf::RenderWindow &window = windowManager.getWindow();
    sf::RectangleShape rect;
    rect.setPosition(menuState->menuButtonRect.left, menuState->menuButtonRect.top);
    rect.setSize({menuState->menuButtonRect.width, menuState->menuButtonRect.height});
    rect.setFillColor(sf::Color(60, 72, 85, 230));
    rect.setOutlineThickness(2.5f);
    rect.setOutlineColor(sf::Color(120, 150, 190));
    window.draw(rect);

    sf::Font *font = resourceManager.hasFont(debugFontName) ? &resourceManager.getFont(debugFontName) : nullptr;
    if (font) {
        sf::Text text;
        text.setFont(*font);
        text.setString("Menu");
        text.setCharacterSize(18);
        text.setFillColor(sf::Color::White);
        text.setOutlineThickness(1.0f);
        text.setOutlineColor(sf::Color(0, 0, 0, 140));
        text.setPosition(menuState->menuButtonRect.left + 10.0f, menuState->menuButtonRect.top + 6.0f);
        window.draw(text);
    }
}

void UIRenderSystem::drawDebugButton() {
    // Debug button disabled per latest requirements (use key toggle instead).
}

void UIRenderSystem::drawMenuUI() {
    if (!menuState || !menuState->visible)
        return;
    sf::RenderWindow &window = windowManager.getWindow();

    const sf::Color overlay(20, 25, 32, 180);
    sf::RectangleShape bg;
    bg.setSize(sf::Vector2f(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y)));
    bg.setFillColor(overlay);
    window.draw(bg);

    sf::Font *font = resourceManager.hasFont(debugFontName) ? &resourceManager.getFont(debugFontName) : nullptr;

    for (const auto &btn : menuState->buttons) {
        sf::RectangleShape rect;
        rect.setPosition(btn.rect.left, btn.rect.top);
        rect.setSize(sf::Vector2f(btn.rect.width, btn.rect.height));
        rect.setFillColor(btn.focused ? sf::Color(70, 90, 110, 220) : sf::Color(44, 52, 60, 220));
        rect.setOutlineThickness(2.0f);
        rect.setOutlineColor(btn.focused ? sf::Color(111, 163, 216) : sf::Color(74, 85, 96));
        window.draw(rect);

        if (font) {
            sf::Text text;
            text.setFont(*font);
            text.setString(btn.label);
            text.setCharacterSize(18);
            text.setFillColor(sf::Color::White);
            text.setOutlineThickness(1.0f);
            text.setOutlineColor(sf::Color(0, 0, 0, 140));
            const float tx = btn.rect.left + 12.0f;
            const float ty = btn.rect.top + (btn.rect.height - text.getLocalBounds().height) * 0.5f - 6.0f;
            text.setPosition(tx, ty);
            window.draw(text);
        }
    }

    if (font && menuState->screen == MenuScreen::Settings) {
        float y = menuState->buttons.empty() ? 120.0f : menuState->buttons.back().rect.top + 60.0f;
        for (const auto &line : menuState->settingsLines) {
            sf::Text t;
            t.setFont(*font);
            t.setCharacterSize(16);
            t.setFillColor(sf::Color::White);
            t.setString(line);
            t.setPosition(80.0f, y);
            t.setOutlineThickness(1.0f);
            t.setOutlineColor(sf::Color(0, 0, 0, 140));
            window.draw(t);
            y += 22.0f;
        }
    }
}

void UIRenderSystem::drawInventoryUI() {
    if (!hasInventory)
        return;
    if (menuState && menuState->visible)
        return;

    sf::RenderWindow &window = windowManager.getWindow();
    const int slotCount = static_cast<int>(slots.size());
    if (slotCount <= 0)
        return;

    const float slotSize = 48.0f;
    const float slotPad = 6.0f;
    const float barWidth = slotCount * slotSize + (slotCount - 1) * slotPad;
    const float xStart = 0.5f * (window.getSize().x - barWidth);
    const float y = static_cast<float>(window.getSize().y) - slotSize - 20.0f;

    sf::RectangleShape rect;
    rect.setSize({slotSize, slotSize});

    const sf::Color slotFill(0x20, 0x25, 0x2b, 215);   // calm dark background
    const sf::Color slotBorder(0x4a, 0x55, 0x60);      // neutral gray border
    const sf::Color activeBorder(0x6f, 0xa3, 0xd8);    // soft blue highlight

    sf::Font *font = nullptr;
    if (resourceManager.hasFont(debugFontName)) {
        font = &resourceManager.getFont(debugFontName);
    }

    for (int i = 0; i < slotCount; ++i) {
        const float x = xStart + i * (slotSize + slotPad);
        const UISlot &slot = slots[i];

        rect.setPosition(x, y);
        rect.setFillColor(slotFill);
        rect.setOutlineThickness(2.0f);
        rect.setOutlineColor(i == activeIndex ? activeBorder : slotBorder);
        window.draw(rect);

        // Icon rendering
        bool iconDrawn = false;
        if (!slot.iconRegion.empty() && resourceManager.hasAtlasRegion(slot.iconRegion)) {
            const auto &region = resourceManager.getAtlasRegion(slot.iconRegion);
            if (resourceManager.hasTexture(region.textureName)) {
                sf::Sprite sprite;
                sprite.setTexture(resourceManager.getTexture(region.textureName));
                sprite.setTextureRect(region.rect);
                const float scale = slotSize / static_cast<float>(std::max(region.rect.width, region.rect.height));
                sprite.setScale(scale, scale);
                sprite.setPosition(x + 4.0f, y + 4.0f);
                window.draw(sprite);
                iconDrawn = true;
            }
        }

        if (!iconDrawn && slot.itemId >= 0) {
            sf::RectangleShape icon;
            icon.setSize({slotSize - 8.0f, slotSize - 8.0f});
            icon.setPosition(x + 4.0f, y + 4.0f);
            icon.setFillColor(sf::Color(70, 90, 110, 210));
            window.draw(icon);
        }

        if (font) {
            // Item label
            sf::Text label;
            label.setFont(*font);
            label.setCharacterSize(12);
            label.setFillColor(sf::Color::White);
            label.setOutlineThickness(1.0f);
            label.setOutlineColor(sf::Color(0, 0, 0, 160));
            if (slot.itemId >= 0) {
                label.setString("ID " + std::to_string(slot.itemId));
                label.setPosition(x + 6.0f, y + 6.0f);
                window.draw(label);
            }

            // Quantity
            if (slot.count > 0) {
                sf::Text qty;
                qty.setFont(*font);
                qty.setCharacterSize(14);
                qty.setFillColor(sf::Color::White);
                qty.setOutlineThickness(1.0f);
                qty.setOutlineColor(sf::Color(0, 0, 0, 160));
                qty.setString(std::to_string(slot.count));
                qty.setPosition(x + slotSize - 6.0f - qty.getLocalBounds().width, y + slotSize - 22.0f);
                window.draw(qty);
            }
        }
    }

    // Active item info
    if (font && activeIndex >= 0 && activeIndex < slotCount) {
        const UISlot &slot = slots[activeIndex];
        if (slot.itemId >= 0 && slot.count > 0) {
            sf::Text info;
            info.setFont(*font);
            info.setCharacterSize(16);
            info.setFillColor(sf::Color::White);
            info.setString("Active: " + std::to_string(slot.itemId) + " x" + std::to_string(slot.count));
            const float infoX = xStart;
            const float infoY = y - 22.0f;
            info.setPosition(infoX, infoY);
            window.draw(info);
        }
    }
}

static std::string formatTime(float seconds) {
    if (seconds < 0.0f)
        seconds = 0.0f;
    int total = static_cast<int>(seconds + 0.5f);
    int m = total / 60;
    int s = total % 60;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
    return std::string(buf);
}

void UIRenderSystem::drawHUD(float dt) {
    sf::RenderWindow &window = windowManager.getWindow();
    sf::Font *font = resourceManager.hasFont(debugFontName) ? &resourceManager.getFont(debugFontName) : nullptr;
    if (!font)
        return;

    // Timer and score
    sf::Text txt;
    txt.setFont(*font);
    txt.setCharacterSize(20);
    txt.setFillColor(sf::Color::White);
    txt.setOutlineThickness(1.0f);
    txt.setOutlineColor(sf::Color(0, 0, 0, 140));

    std::string phaseLabel = "Pre";
    if (matchPhase == MatchStateEvent::Phase::InMatch)
        phaseLabel = "Match";
    else if (matchPhase == MatchStateEvent::Phase::PostMatch)
        phaseLabel = "Post";

    txt.setString(phaseLabel + " | " + formatTime(matchTime) + " | " + std::to_string(scoreA) + " - " +
                  std::to_string(scoreB));
    const float centerX = window.getSize().x * 0.5f;
    txt.setPosition(centerX - txt.getLocalBounds().width * 0.5f, 8.0f);
    window.draw(txt);

    // Hotkey hints
    sf::Text hint;
    hint.setFont(*font);
    hint.setCharacterSize(14);
    hint.setFillColor(sf::Color(220, 220, 220));
    hint.setOutlineThickness(1.0f);
    hint.setOutlineColor(sf::Color(0, 0, 0, 120));
    hint.setString("Tab: Scoreboard   ~: Debug   Esc: Pause");
    hint.setPosition(12.0f, 36.0f);
    window.draw(hint);

    drawKillfeed(dt);
    if (showScoreboard)
        drawScoreboard();
    if (showRespawn)
        drawRespawn();
}

void UIRenderSystem::drawKillfeed(float dt) {
    if (killfeed.empty())
        return;
    sf::RenderWindow &window = windowManager.getWindow();
    sf::Font *font = resourceManager.hasFont(debugFontName) ? &resourceManager.getFont(debugFontName) : nullptr;
    if (!font)
        return;

    for (auto &k : killfeed) {
        k.ttl -= dt;
    }
    killfeed.erase(std::remove_if(killfeed.begin(), killfeed.end(), [](const KillFeedEntry &e) { return e.ttl <= 0.0f; }),
                   killfeed.end());
    if (killfeed.empty())
        return;

    const float margin = 12.0f;
    const float slotH = 20.0f;
    const float startX = static_cast<float>(window.getSize().x) - 260.0f;
    const float startY = 60.0f;

    sf::Text t;
    t.setFont(*font);
    t.setCharacterSize(16);
    t.setFillColor(sf::Color::White);
    t.setOutlineThickness(1.0f);
    t.setOutlineColor(sf::Color(0, 0, 0, 140));

    int idx = 0;
    for (const auto &e : killfeed) {
        const float y = startY + idx * (slotH + 4.0f);
        std::string line = e.killer + " -> " + e.victim;
        if (!e.weapon.empty())
            line += " [" + e.weapon + "]";
        t.setString(line);
        t.setPosition(startX, y);
        window.draw(t);
        ++idx;
    }
}

void UIRenderSystem::drawScoreboard() {
    sf::RenderWindow &window = windowManager.getWindow();
    sf::Font *font = resourceManager.hasFont(debugFontName) ? &resourceManager.getFont(debugFontName) : nullptr;
    if (!font)
        return;

    const float width = 420.0f;
    const float rowH = 26.0f;
    const float x = (window.getSize().x - width) * 0.5f;
    const float y = 120.0f;
    const float height = rowH * static_cast<float>(std::max<std::size_t>(players.size(), 1) + 1) + 16.0f;

    sf::RectangleShape bg;
    bg.setPosition(x, y);
    bg.setSize({width, height});
    bg.setFillColor(sf::Color(20, 24, 30, 200));
    bg.setOutlineThickness(2.0f);
    bg.setOutlineColor(sf::Color(90, 110, 140));
    window.draw(bg);

    sf::Text t;
    t.setFont(*font);
    t.setCharacterSize(16);
    t.setFillColor(sf::Color::White);
    t.setOutlineThickness(1.0f);
    t.setOutlineColor(sf::Color(0, 0, 0, 120));

    t.setString("Player            K   D");
    t.setPosition(x + 12.0f, y + 8.0f);
    window.draw(t);

    float rowY = y + 8.0f + rowH;
    for (const auto &p : players) {
        t.setString(p.name);
        t.setPosition(x + 12.0f, rowY);
        window.draw(t);

        t.setString(std::to_string(p.kills));
        t.setPosition(x + width - 80.0f, rowY);
        window.draw(t);

        t.setString(std::to_string(p.deaths));
        t.setPosition(x + width - 40.0f, rowY);
        window.draw(t);

        rowY += rowH;
    }
}

void UIRenderSystem::drawRespawn() {
    if (!showRespawn)
        return;
    sf::RenderWindow &window = windowManager.getWindow();
    sf::Font *font = resourceManager.hasFont(debugFontName) ? &resourceManager.getFont(debugFontName) : nullptr;
    if (!font)
        return;

    sf::RectangleShape overlay;
    overlay.setSize(sf::Vector2f(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y)));
    overlay.setFillColor(sf::Color(0, 0, 0, 160));
    window.draw(overlay);

    sf::Text t;
    t.setFont(*font);
    t.setCharacterSize(22);
    t.setFillColor(sf::Color::White);
    t.setOutlineThickness(1.5f);
    t.setOutlineColor(sf::Color(0, 0, 0, 180));

    t.setString("Respawn in: " + formatTime(respawnTime));
    t.setPosition(40.0f, 120.0f);
    window.draw(t);

    if (canRespawn) {
        sf::RectangleShape btn;
        btn.setPosition(40.0f, 170.0f);
        btn.setSize({200.0f, 40.0f});
        btn.setFillColor(sf::Color(60, 72, 85, 230));
        btn.setOutlineThickness(2.0f);
        btn.setOutlineColor(sf::Color(120, 150, 190));
        window.draw(btn);

        sf::Text b;
        b.setFont(*font);
        b.setCharacterSize(18);
        b.setFillColor(sf::Color::White);
        b.setString("Respawn");
        b.setPosition(60.0f, 178.0f);
        window.draw(b);
    }
}

void UIRenderSystem::handleInventoryStateChanged(const InventoryStateChangedEvent &ev) {
    hasInventory = true;
    activeIndex = ev.activeIndex;
    slots.clear();
    slots.reserve(ev.slots.size());
    for (size_t i = 0; i < ev.slots.size(); ++i) {
        const auto &src = ev.slots[i];
        UISlot dst{};
        dst.itemId = src.itemId;
        dst.count = src.count;
        if (dst.itemId >= 0) {
            const auto metaIt = ev.itemMeta.find(dst.itemId);
            if (metaIt != ev.itemMeta.end()) {
                dst.iconTexture = metaIt->second.texture;
                dst.iconRegion = metaIt->second.region;
            }
        }
        slots.push_back(std::move(dst));
    }
    if (activeIndex < 0 || activeIndex >= static_cast<int>(slots.size()))
        activeIndex = 0;
}

void UIRenderSystem::handleMatchState(const MatchStateEvent &ev) {
    matchPhase = ev.phase;
    matchTime = ev.matchTime;
    scoreA = ev.scoreA;
    scoreB = ev.scoreB;
}

void UIRenderSystem::handlePlayerStats(const PlayerStatsEvent &ev) {
    players.clear();
    players.reserve(ev.players.size());
    for (const auto &p : ev.players) {
        players.push_back(PlayerRow{p.name, p.kills, p.deaths});
    }
}

void UIRenderSystem::handleKillEvent(const KillEvent &ev) {
    KillFeedEntry k{};
    k.killer = ev.killer;
    k.victim = ev.victim;
    k.weapon = ev.weapon;
    k.ttl = 6.0f;
    killfeed.push_back(k);
    if (killfeed.size() > 7)
        killfeed.erase(killfeed.begin());
}

void UIRenderSystem::handleRespawnTimer(const RespawnTimerEvent &ev) {
    respawnTime = ev.secondsLeft;
    canRespawn = ev.canRespawn;
    showRespawn = ev.secondsLeft > 0.0f || ev.canRespawn;
}

