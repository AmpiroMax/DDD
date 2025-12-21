# Обзор проекта и кодовая карта

## Что делаем
Ветка `diablo`: делаем **изометрическую экшен-игру** с:
- активными **заклинаниями** и **призывами**,
- **PvP аренами** (до 8 игроков, deathmatch),
- продуманной **сетевой моделью** (дизайн → прототип → реализация).

Ближайший фокус: **графика (изометрия)** и **топ-даун физика/коллизии**. Заклинания/баланс — после того, как сцена/движение/пвп-арена стабильно играются.

## Технологии и стек
- C++20
- SFML 2.5 (window/render/input)
- Box2D (physics)
- nlohmann/json (конфиги)

## Архитектурные принципы
- ECS: `EntityManager` хранит сущности, компоненты — plain-данные (`Transform`, `Sprite`, `Tilemap`, `PhysicsBody`, `Input`, `Grounded` и т.п.).
- Системы: `InputSystem`, `PhysicsSystem`, `RenderSystem`, `UIRenderSystem`. Системы обновляются в игровом цикле `GameApp` (рендер каждый кадр, физика — fixed step).
- Менеджеры: обёртки над подсистемами/ресурсами (`WindowManager`, `CameraManager`, `PhysicsManager`, `ResourceManager`, `TimeManager`, `DebugManager`).
- События: `EventBus` (каркас для ассинхронной доставки), за физику — события в `events/PhysicsEvents.h`.
- Соглашения по масштабам: 1 world unit = 1 тайл = 1 метр физики; `RENDER_SCALE = 32 px`, `PHYSICS_SCALE = 1.0`; ось Y вниз в рендере (SFML), вверх в мире — используем конвертеры `utils/CoordinateUtils.h`.

### Новые опорные понятия (для Editor + 2.5D)
### Новые опорные понятия (Diablo-like)
- **Isometric camera**: ортографическая “изометрия” (2D), с z-order сортировкой по Y/слоям.
- **Top-down physics**: коллизии в плоскости (X,Y), без platformer-гравитации.
- **Arena**: отдельная карта “храм с колоннами”, рассчитанная на 2–8 игроков.
- **Networking**: server-authoritative симуляция (минимум для PvP) + репликация состояний.

## Кодовая карта
- `src/main.cpp` — вход, создаёт `GameApp`.
- `src/game/` — жизненный цикл, конфиг, инициализация систем (`GameApp`).
- `src/core/` — каркас ECS: сущности, менеджер, базовый класс системы, шина событий.
- `src/components/` — данные компонентов: `Transform`, `Sprite`, `Tilemap`, `PhysicsBody`, `Input`, `Grounded`, `Inventory`, `Drop`, `Tags`, др.
- `src/systems/` — логика систем: `InputSystem`, `PhysicsSystem`, `RenderSystem`, `UIRenderSystem`.
  - (план) добавятся:
    - `IsometricRenderSystem` (изометрический рендер и depth sorting)
    - `TopDownMovementSystem` (движение/коллизии top-down)
    - `ArenaRulesSystem` (deathmatch правила, спавны, очки)
    - `NetClientSystem` / `NetServerSystem` (после дизайн-спайка)
- `src/managers/` — окна, камера, ресурсы (текстуры/шрифты/атлас), время, физика, дебаг.
- `src/events/` — события физики и инвентаря (Place/BreakBlock, Grounded, InventoryStateChanged/ActiveSlotChanged/DropAdded/UseItem).
- `src/utils/` — константы, утилиты координат, векторы.
- `config/` — `game.json` (окно/ресурсы/мир, `world.map_file`, `world.tile_size=32`, `inventory_file`), `input.json` (биндинги движения/действий и хотбара до 10 слотов, wheel prev/next), `inventory.json` (определения предметов, stack, icon_region/icon_texture, стартовые предметы), `maps/*.json` (ширина/высота, `tile_size` в wu, origin top-left, ось Y вниз, `tiles`, `solid_ids`, `tile_id_to_region`, `player_spawn`).
- `resources/`, `textures/`, `fonts/` — ассеты; атлас `textures/tileMap.png` (fallback `textures/tiles.png`), регионы 32x32: ground(0,0), path(1,0), grass_alt(6,0), leaves(6,1), water(10,0), stone_brick(11,0), dirt(12,0), roof(13,0), trunk(1,1); маппинг tileId 1..9 → регионы, пусто=-1; debug alias `debug` → `fonts/ArialRegular.ttf` (fallback RobotoMono).
- `rules/` — договорённости по доменам (physics/graphics/mechanics) и борда.
- `slack/` — рабочие чаты.
- `docs/` — текущая документация (этот файл, Build & Run, release notes).

## Ожидания по веткам и билду
- `main` всегда green; коммиты не ломают сборку.
- Новые задачи — в feature-ветках (говорящее имя), мержим через pull/merge request после ревью.
- Перед мержем: собрать, проверить базовый сценарий запуска, согласовать кросс-области (physics/graphics/mechanics); ресурсы читаются из корня при запуске из `build` (отн. `../resources`), copy-step не нужен.

## Открытые вопросы (нужен ответ от команды)
- Сетка/мир: карта для арены будет тайловой (изометрический tilemap) или “меш” из префабов-колонн?
- Сетевая модель PvP: фиксируем **server-authoritative** (рекомендуется), и какие данные реплицируем (позиция, скорость, касты, hp/mana).

## Смежные материалы
- Build & Run: `docs/build_run.md`.
- Плейсхолдер под релизы: `docs/release-notes.md`.
 - Isometric renderer: `docs/isometric_render.md`
 - PvP arena: `docs/pvp_arena.md`
 - Networking: `docs/networking.md`
 - Spells & balance: `docs/spells_balance.md`
