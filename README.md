# DDD — Diablo-like Isometric PvP Arena (C++20 / SFML / Box2D)

Проект меняет курс (ветка `diablo`): делаем **изометрическую игру** с активными **заклинаниями/призывами** и **PvP аренами** (до 8 игроков, deathmatch).

Порядок работ:
- сначала **графика (изометрический рендер)** и **топ-даун физика/коллизии**,
- затем **сетевой дизайн** и прототип,
- после этого — **заклинания, призывы, баланс**.

## Быстрый старт
- Сборка/запуск и ожидания: `docs/build_run.md`
- Быстрый запуск скриптом (соберёт при необходимости): `scripts/run.sh`

## Важные доки (start here)
- **Build & Run**: `docs/build_run.md`
- **Обзор проекта + кодовая карта**: `docs/overview_and_map.md`
- **Системы, зависимости и риски**: `docs/systems_overview.md`
- **Release notes (placeholder)**: `docs/release-notes.md`
- **Аудит/риск-лист (инженерный взгляд)**: `docs/architecture_audit.md`
- **Top priority tasks (живой список)**: `docs/top_priority_tasks.md`
 - **Isometric renderer (спека)**: `docs/isometric_render.md`
 - **PvP arena (спека)**: `docs/pvp_arena.md`
 - **Networking (спека)**: `docs/networking.md`
 - **Spells & balance (спека)**: `docs/spells_balance.md`

## Коммуникации (в репозитории)
- Общий чат (только итоги/эскалации): `slack/general.md`
- Личные/парные чаты по доменам:
  - Physics: `slack/physics.md`
  - Graphics: `slack/graphics.md`
  - Mechanics: `slack/mechanics.md`
  - TechWriter: `slack/techwriter.md`
  - UI/Inventory: `slack/ui_inventory.md`
  - Sync Graphics×TechWriter (ресурсы/доки): `slack/graphics_techwriter_resources.md`
  - Sync Mechanics×Physics (Place/Break): `slack/mechanics_physics_place_break.md`
  - Sync Mechanics×Graphics (атлас/тайлы): `slack/mechanics_graphics_atlas.md`
  - Inventory planning: `slack/mechanics_inventory.md`

## Правила/борда
- Глобальные соглашения и масштаб/координаты: `rules/GLOBAL.md`
- Канбан: `rules/BOARD.md`
- Доменные правила: `rules/PHYSICS.md`, `rules/GRAPHICS.md`, `rules/MECHANICS.md`
- Роль техписа: `rules/TECHWRITER.md`

## Где что лежит в коде (карта)
- **Entry point**: `src/main.cpp`
- **Game loop / init / config**: `src/game/GameApp.{h,cpp}`
- **ECS core**: `src/core/` (`Entity`, `EntityManager`, `Component`, `System`, `EventBus`)
- **Components (данные)**: `src/components/`
  - трансформ/спрайт/тайлмап: `TransformComponent.h`, `SpriteComponent.h`, `TilemapComponent.h`
  - физика/земля: `PhysicsBodyComponent.h`, `GroundedComponent.h`
  - ввод/инвентарь/дроп: `InputComponent.h`, `InventoryComponent.h`, `DropComponent.h`
- **Systems (логика)**: `src/systems/`
  - ввод: `InputSystem.*`
  - механики: `PlayerControlSystem.*`, `TileInteractionSystem.*`, `CameraFollowSystem.*`
  - физика: `PhysicsSystem.h`
  - рендер: `RenderSystem.*`, UI: `UIRenderSystem.*`
  - инвентарь/дропы: `InventorySystem.*`, `DropPickupSystem.*`
  - debug: `DebugSystem.*`
- **Managers (сервисы)**: `src/managers/` (`WindowManager`, `CameraManager`, `ResourceManager`, `PhysicsManager`, `TimeManager`, `DebugManager`)
- **Events**: `src/events/` (`TileEvents.h`, `PhysicsEvents.h`, `InventoryEvents.h`)
- **Utils**: `src/utils/` (константы/скейлы, конвертеры координат)

## Конфиги/ресурсы (что влияет на рантайм)
- `config/game.json` — окно, пути, карта, инвентарь, параметры игрока.
- `config/input.json` — биндинги (движение/прыжок/break/place + хотбар/инвентарь: wheel/цифры/алиасы).
- `config/inventory.json` — предметы (stack, place_tile_id, icon_region/icon_texture), стартовые слоты.
- `config/maps/*.json` — карты, `tile_id_to_region`, `solid_ids`, `player_spawn`.
- `resources/` — ассеты (textures/fonts/maps).

## “Прикольные места”, куда смотреть
- Главный цикл и порядок апдейтов: `src/game/GameApp.cpp`
- Рендер тайлмапа по `tile_id_to_region`: `src/systems/RenderSystem.cpp`
- Инвентарь снапшотом (InventoryStateChanged) и хотбар UI: `src/events/InventoryEvents.h`, `src/systems/UIRenderSystem.cpp`
- Place/Break + тела тайлов + дропы: `src/events/TileEvents.h`, `src/systems/PhysicsSystem.h`, `src/components/DropComponent.h`
- Масштаб/координаты (важно для физики/рендера): `src/utils/Constants.h`, `src/utils/CoordinateUtils.h`

## Что считается “рабочим билдом” для релиза
- `main` зелёный: сборка проходит, игра запускается через `scripts/run.sh` или из `build/`.
- Базовый сценарий (обновится по мере миграции): запуск, изометрическая сцена, управление игроком, коллизии, спавн нескольких игроков (пока локально), базовая PvP-арена.

