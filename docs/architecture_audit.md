## Architecture audit (ECS): компоненты, системы, менеджеры, связи и риски

Документ: **аудит текущей архитектуры** в `src/` и влияния `config/`.
Цель: убрать "две правды" (как было с Transform vs PhysicsBody), зафиксировать ownership/инварианты, подсветить риски до релиза.

### 1) Координаты и соглашения (важно)
- **World units**: 1.0 = 1 метр = 1 тайл (по замыслу), рендер: `RENDER_SCALE=32 px`.
- **Оси**:
  - мир: Y вверх;
  - SFML render: Y вниз;
  - тайловые данные: `TilemapComponent.tiles` — row-major, **y растёт вниз**.
- Конвертеры: `src/utils/CoordinateUtils.h`.

### 2) Runtime-loop и жизненный цикл (GameApp)
Файл: `src/game/GameApp.cpp`.

Основной цикл (упрощённо):

```mermaid
flowchart TD
  Poll[Poll SFML events] --> InputEv[InputSystem.handleEvent]
  InputEv --> Tick[TimeManager.tick dt]
  Tick --> InputUpd[InputSystem.update]
  InputUpd --> Menu[GameApp.updateMenu]
  Menu --> Logic[updateSystems (кроме Input) if Playing]
  Logic --> PhysAcc[accumulate physicsAccumulator]
  PhysAcc --> PhysStep[PhysicsSystem.update fixed-step]
  PhysStep --> Pump[EventBus.pump]
  Pump --> Render[RenderSystem + UIRenderSystem]
  Render --> Poll

  note1["Physics fixed-step: while accumulator>=1/60"]
  PhysAcc -.-> note1
```

Ключевые точки жизненного цикла:
- **Init**: `loadConfig()` → `WindowManager.create()` → `ResourceManager.setBasePaths()` → `loadResources()` → `initSystems()` → menu.
- **startGame(mapPath)**:
  - `physicsSystem->reset()` (очистка внутренних кэшей/коллайдеров),
  - `resetWorld()` (пересоздаёт b2World + `EntityManager.clear()`),
  - `inputSystem->resetInputEntity()` (создаёт отдельную сущность с `InputComponent`),
  - `loadMapAndEntities(mapPath)` (Tilemap, Player, компоненты),
  - `screen=Playing`.
- **save/load**:
  - `collectSaveData()` берёт diff `TilemapComponent.tiles` vs `originalTiles` + состояние player + drops.
  - `loadSave()` → `startGame(base_map)` → `applySaveData()` (накатывает tiles/drops/player/inventory).

### 3) Инвентаризация сущностей

#### 3.1 Components (`src/components/*.h`)
- **`TransformComponent`**: позиция/угол/скейл в world. Источник правды для рендера и gameplay, если нет физики.
- **`PhysicsBodyComponent`**: описание тела Box2D + runtime pointer `b2Body*`. Поля `position/angleDeg` — world-кэш.
- **`SpriteComponent`**: визуал (textureName/atlasRegion/rect/origin/scale/z).
- **`TilemapComponent`**: тайловая карта (width/height/tileSize/origin/tiles/solidIds/emptyId) + `originalTiles` для сейва.
- **`InputComponent`**: raw keys/mouse + actions + mouse world coords.
- **`GroundedComponent`**: grounded flag.
- **`InventoryComponent`**: slots + activeSlot + hotbarSize.
- **`DropComponent`**: itemId/count.
- **`Tags`**: `PlayerTag`, `CameraTargetTag`.

#### 3.2 Systems (`src/systems/*`)
- **`InputSystem`**: создаёт Input-entity, собирает SFML input → `InputComponent.actions`, mouseWorld. Читает `config/input.json`.
- **`PhysicsSystem`** (fixed-step):
  - создаёт Box2D тела из `PhysicsBodyComponent`,
  - поддерживает per-tile static colliders из `TilemapComponent`,
  - слушает `PlaceBlockEvent/BreakBlockEvent`,
  - эмитит `ContactEvent/GroundedEvent`,
  - синхронизирует позицию/угол в `PhysicsBodyComponent` и (после фикса) в `TransformComponent`.
- **`PlayerControlSystem`**: читает input, пишет скорость b2Body, прыжок по grounded.
- **`CameraFollowSystem`**: выставляет `CameraManager.center` по `TransformComponent` игрока.
- **`TileInteractionSystem`**: pick тайла по `mouseWorld`, меняет `TilemapComponent.tiles`, эмитит `PlaceBlockEvent/BreakBlockEvent`, при place — списывает item из `InventorySystem`.
- **`InventorySystem`**: хотбар/слоты; читает input (slot_prev/next и slot_1..10), эмитит `InventoryStateChangedEvent`.
- **`DropPickupSystem`**: подбирает `DropComponent` по расстоянию к игроку и складывает в инвентарь; удаляет физтело через `PhysicsManager.destroyBody`.
- **`RenderSystem`**: рисует `TilemapComponent` и `SpriteComponent` по `TransformComponent`.
- **`UIRenderSystem`**: меню+инвентарь+debug overlay; подписка на `InventoryStateChangedEvent`; читает `DebugManager.getStreams()`.
- **`DebugSystem`**: пишет строки в `DebugManager` (streams mechanics/physics) + периодически логирует в файл.

#### 3.3 Managers (`src/managers/*.h`)
- **`WindowManager`**: SFML window + active view.
- **`CameraManager`**: camera center/viewport/zoom.
- **`ResourceManager`**: textures/fonts + atlas regions (name→texture+rect) и резолв путей.
- **`PhysicsManager`**: владеет `b2World`, фабрики body/fixture, destroyBody, resetWorld.
- **`TimeManager`**: dt.
- **`DebugManager`**: debug visibility + `streams` (source→lines).

#### 3.4 Events (`src/events/*.h`)
- Physics: `ContactEvent`, `GroundedEvent`.
- Tile: `PlaceBlockEvent`, `BreakBlockEvent`.
- Inventory: `InventoryStateChangedEvent`, `InventoryAddItemEvent`, `InventoryDropAddedEvent`, `InventoryUseItemEvent`, ...

### 4) Таблицы связей (визуально)

#### 4.1 Systems summary (входы/выходы)

| System | Cadence | Reads | Writes | Emits | Subscribes | Managers |
|---|---|---|---|---|---|---|
| InputSystem | variable | SFML events, Window view/camera | InputComponent | - | - | WindowManager, CameraManager |
| PhysicsSystem | fixed-step | PhysicsBodyComponent, TilemapComponent, TileEvents | PhysicsBodyComponent.body/pos/angle, TransformComponent | ContactEvent, GroundedEvent | PlaceBlockEvent, BreakBlockEvent | PhysicsManager |
| PlayerControlSystem | variable (Playing) | InputComponent.actions, GroundedComponent | b2Body velocity, (Transform sync optional) | - | GroundedEvent | - |
| CameraFollowSystem | variable (Playing) | TransformComponent (player/target) | CameraManager.center | - | - | CameraManager |
| TileInteractionSystem | variable (Playing) | InputComponent.mouseWorld, TilemapComponent, InventorySystem active item | TilemapComponent.tiles | PlaceBlockEvent, BreakBlockEvent, InventoryUseItemEvent | - | - |
| InventorySystem | variable (Playing) | InputComponent.actions, config/inventory.json | InventoryComponent (slots/active) | InventoryStateChangedEvent, InventoryDropAddedEvent | InventoryAddItemEvent | - |
| DropPickupSystem | variable (Playing) | TransformComponent (player+drop), DropComponent | DropComponent.count, EntityManager.remove | - | - | PhysicsManager |
| RenderSystem | variable | TilemapComponent, SpriteComponent, TransformComponent | - | - | - | WindowManager, CameraManager, ResourceManager |
| UIRenderSystem | variable | DebugManager, InventoryStateChanged snapshot, menu state ptr | - | - | InventoryStateChangedEvent | WindowManager, ResourceManager, DebugManager |
| DebugSystem | variable | Player Transform/PhysicsBody/Grounded, TilemapComponent, InputComponent, DropComponent | DebugManager.streams, debug_log.txt | - | - | DebugManager |

#### 4.2 Components × Systems (R/W)
Легенда: **R**=read, **W**=write, **RW**=both, **-**=нет.
Системы: `Inp` InputSystem, `Phys` PhysicsSystem, `Ply` PlayerControl, `Cam` CameraFollow, `Tile` TileInteraction, `Inv` InventorySystem, `Pick` DropPickup, `Rend` RenderSystem, `UI` UIRenderSystem, `Dbg` DebugSystem.

| Component | Inp | Phys | Ply | Cam | Tile | Inv | Pick | Rend | UI | Dbg |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| TransformComponent | - | **W** | **W** | **R** | - | - | **R** | **R** | - | **R** |
| PhysicsBodyComponent | - | **RW** | **R** | - | - | - | **R** | - | - | **R** |
| SpriteComponent | - | (создаёт для drop) | - | - | - | - | - | **R** | - | - |
| TilemapComponent | - | **R** (build colliders) | - | - | **W** | - | - | **R** | - | **R** |
| InputComponent | **W** | - | **R** | - | **R** | **R** | - | - | - | **R** |
| GroundedComponent | - | - | **RW** | - | - | - | - | - | - | **R** |
| InventoryComponent | - | - | - | - | - | **RW** | - | - | - | - |
| DropComponent | - | (создаёт) | - | - | - | - | **RW** | - | - | **R** |
| Tags (Player/CameraTarget) | - | - | **R** | **R** | **R** | - | **R** | **R** | - | - |

#### 4.3 Systems × Managers
| System | WindowManager | CameraManager | ResourceManager | PhysicsManager | TimeManager | DebugManager |
|---|---:|---:|---:|---:|---:|---:|
| InputSystem | U | U | - | - | - | - |
| PhysicsSystem | - | - | - | **U/C** | - | - |
| PlayerControlSystem | - | - | - | - | - | - |
| CameraFollowSystem | - | **U** | - | - | - | - |
| TileInteractionSystem | - | - | - | - | - | - |
| InventorySystem | - | - | - | - | - | - |
| DropPickupSystem | - | - | - | **U** | - | - |
| RenderSystem | **U** | **U** | **U** | - | - | - |
| UIRenderSystem | **U** | - | **U** | - | - | **U** |
| DebugSystem | - | - | - | - | - | **U** |

#### 4.4 Events × Systems
| Event | Emit | Subscribe |
|---|---|---|
| PlaceBlockEvent | TileInteractionSystem | PhysicsSystem |
| BreakBlockEvent | TileInteractionSystem | PhysicsSystem |
| ContactEvent | PhysicsSystem | (нет сейчас) |
| GroundedEvent | PhysicsSystem | PlayerControlSystem |
| InventoryStateChangedEvent | InventorySystem | UIRenderSystem |
| InventoryAddItemEvent | (внешний/опционально) | InventorySystem |
| InventoryUseItemEvent | TileInteractionSystem | (нет сейчас) |

### 5) Влияние config/* (источник правды)

| Config key | Читатель | Эффект |
|---|---|---|
| `config/game.json.window.size/title` | GameApp | размер окна, заголовок |
| `resources_path/textures_path/fonts_path` | GameApp → ResourceManager | базовые пути ресурсов |
| `world.tile_size` | GameApp | `config.tileSize = tile_px / RENDER_SCALE` |
| `world.map_file` | GameApp | стартовая карта |
| `inventory_file` | GameApp → InventorySystem | конфиг инвентаря |
| `player.speed/jump_impulse` | GameApp → PlayerControlSystem | скорость/прыжок |
| `player.pickup_radius` | GameApp → DropPickupSystem | радиус подбора (px→world) |
| `config/input.json.actions.*` | InputSystem | бинды, алиасы inventory_* |
| `config/maps/*.json` | GameApp | tilemap (tiles/solid_ids/origin/player_spawn/regions) |
| `config/inventory.json` | InventorySystem | defs/стартовые слоты/иконки |

### 6) Связи «каждый с каждым» (кратко)

#### Компоненты
- `TransformComponent` ↔ `PhysicsBodyComponent`: **связаны через sync** в `PhysicsSystem::syncTransforms()` (физика → трансформ). Если у сущности есть `PhysicsBodyComponent.body`, physics — authoritative.
- `TilemapComponent` ↔ `PhysicsSystem`: tilemap → набор статических colliders (per-tile) + updates через Place/Break.
- `TilemapComponent` ↔ `RenderSystem`: tile_id_to_region → atlas region name.
- `DropComponent` ↔ `DropPickupSystem` ↔ `InventorySystem`: drop → addItem → инвентарь.
- `InventorySystem` ↔ `UIRenderSystem`: snapshot через `InventoryStateChangedEvent`.
- `DebugManager` ↔ `DebugSystem` ↔ `UIRenderSystem`: streams → overlay.

#### Системы
- `TileInteractionSystem` → (events) → `PhysicsSystem` (коллайдеры/дропы).
- `PhysicsSystem` → (events) → `PlayerControlSystem` (Grounded).
- `InventorySystem` → (event) → `UIRenderSystem`.
- `RenderSystem` зависит от `CameraManager` (view), который зависит от `CameraFollowSystem`.

### 7) Проблемы и риски

#### ✅ Confirmed (уже проявлялось)
1) **Дублирование источника правды позиции**: `TransformComponent.position` и `PhysicsBodyComponent.body` жили независимо.
   - Симптом: "дропы не падают" (скорость меняется в Box2D, но визуал/пикап стоят на месте).
   - Фикс-инвариант: если есть `PhysicsBodyComponent.body`, то каждый physics tick должен синкать `TransformComponent`.
   - Реализация: `PhysicsSystem::syncTransforms()` пишет в Transform (позиция/угол).

#### ⚠️ High-risk (может стрельнуть до релиза)
1) **EventBus без unsubscribe** (`src/core/EventBus.h`).
   - Все `subscribe` захватывают `this` (PhysicsSystem/InventorySystem/UIRenderSystem/PlayerControlSystem).
   - Пока системы живут весь runtime — ок. Если начнёте пересоздавать системы (например, при reset) → UAF.
   - Рекомендация: либо добавить unsubscribe/token, либо закрепить инвариант: системы живут до конца приложения.

2) **Reset b2World через placement-new** (`PhysicsManager.resetWorld`).
   - Это корректно для Box2D, но требует жёсткого жизненного цикла: после reset **нельзя** трогать старые `b2Body*`.
   - Сейчас спасает `EntityManager.clear()` сразу после reset. Если порядок поменяется — риск.

3) **Per-tile bodies для коллизий** (`PhysicsSystem.tileBodies`).
   - Работает для MVP, но рост карты → нагрузка на Box2D.
   - План: перейти на chunk bodies / edge shapes.

4) **Tilemap tiles vs physics colliders**.
   - Инвариант: изменения `TilemapComponent.tiles` должны сопровождаться событием Place/Break.
   - Сейчас TileInteractionSystem делает и tiles update, и emit — ок.
   - Риск: другой код может менять `tiles` напрямую (например, applySaveData) — тогда нужен rebuild коллайдеров.

5) **Save/Load неполнота**.
   - `applySaveData()` создаёт drops, но **не применяет сохранённые velocity** на b2Body.
   - `removed_tiles` в save не хранит tile_id (по дизайну не нужен), но структура SaveData содержит tileId — неоднозначность.

#### ℹ️ Code smells (не критично, но стоит подчистить)
- В `PhysicsSystem::ensureBodies()` есть дублирующие вызовы `SetSleepingAllowed(false)` в ветке drop (можно подчистить).
- В `GameApp::run()` логика `if (!menuActive || sys.get()==inputSystem)` сейчас частично мёртвая, т.к. inputSystem уже пропускается.

### 8) Рекомендованные инварианты (фиксируем как правила)
1) **Physics-authoritative**: если сущность имеет `PhysicsBodyComponent.body != nullptr`, то:
   - Transform синкается из физики каждый fixed-step,
   - gameplay/рендер/пикап/дебаг читают Transform.
2) **Tilemap-authoritative**: `TilemapComponent.tiles` — источник правды для мира.
   - Коллайдеры — производные; изменяются только через Place/Break events.
3) **System lifetime**: либо добавляем unsubscribe, либо фиксируем, что системы живут весь runtime.
4) **Reset order**: reset physics world → clear entities → recreate entities → rebuild colliders.

### 9) До релиза: чеклист (минимум)
- [ ] Зафиксировать инварианты выше (как короткие правила в коде/доках).
- [ ] Решить политику EventBus (unsubscribe или "systems live forever").
- [ ] Привести save/load drops: применять velocity (опционально).
- [ ] (опционально) Чанкование физики тайлов, если начнутся тормоза.
