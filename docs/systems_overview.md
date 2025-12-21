# Обзор систем, зависимостей и рисков

## Новый курс (важно)
Ветка `diablo`: делаем:
- **Изометрию** (2D рендер с depth sorting)
- **Топ-даун движение/коллизии**
- **PvP арены** (до 8 игроков, deathmatch)
- **Сетевую модель** (дизайн → прототип → реализация)

Часть описанных ниже систем остаётся актуальной как фундамент (окно/ресурсы/input/debug), но platformer-ориентированные механики будут постепенно заменяться на top-down.

## Цикл игры (высокоуровнево)
- `GameApp::run()` — главный цикл: обрабатывает события окна, обновляет системы каждый кадр (рендер/инпут/UI) и физику фиксированным шагом (`PHYSICS_TIMESTEP`), затем `eventBus.pump()`.
- Системы не знают друг о друге напрямую; общение через компоненты и события (`EventBus`), менеджеры дают доступ к окну/ресурсам/физике/времени/камере.

## Планируемые системы (Diablo-like)
- `IsometricRenderSystem`:
  - изометрическая проекция/камера, сортировка по depth (y/z), отрисовка окружения/персонажей.
- `TopDownMovementSystem`:
  - движение/ускорение, коллизии (box/circle), скольжение по стенам, “feel”.
- `ArenaRulesSystem`:
  - правила deathmatch, спавны, очки/таймер, зоны PvP.
- `NetServerSystem` / `NetClientSystem` (после дизайн-спайка):
  - authoritative server + репликация состояния + input команд.

## Системы (назначение и входы)
- `InputSystem`  
  - Ответственность: парсинг событий SFML, биндинги из `config/input.json` (движение, прыжок, break/place, хотбар/инвентарь: колесо/цифры/алиасы). Обновляет `InputComponent`, рассылает инпут-события (для механик/камеры и т.д.).  
  - Зависимости: `WindowManager` (события окна), `CameraManager` (координаты мыши в мир), `EntityManager` (компоненты ввода), `config/input.json`.

- `PhysicsSystem`  
  - Ответственность: интеграция физики через Box2D, создание/удаление тел для тайлов (Place/Break), поддержка статичных тел карты, обработка `Grounded`/контактов, дропы из сломанных блоков.  
  - Зависимости: `PhysicsManager` (мир Box2D), `EntityManager` (компоненты `PhysicsBody`, `Transform`), `EventBus` (события PlaceBlock/BreakBlock/Grounded и др.), данные `TilemapComponent` (`tile_size`, `solid_ids`, `tile_id_to_region`).  
  - Особенности: тела на тайл (без чанков), origin карты — верхний левый, ось Y вниз в данных.

- `RenderSystem`  
  - Ответственность: рендер тайлмапов и спрайтов по Z с сортировкой; конвертация world→render с `RENDER_SCALE`.  
  - Зависимости: `WindowManager` (рендер-окно/view), `CameraManager` (центр/zoom/view size), `ResourceManager` (текстуры/атлас регионы), `EntityManager` (`Transform`, `Tilemap`, `Sprite`).  
  - Использует `tile_id_to_region` из тайлмапа и зарегистрированные регионы/текстуры. Пропускает отсутствующие регионы/текстуры без краша.

- `UIRenderSystem`  
  - Ответственность: UI-оверлей (FPS/debug строка через `DebugManager`), хотбар/инвентарь (по `InventoryStateChanged` снапшоту: слоты, qty, активный, иконка).  
  - Зависимости: `WindowManager` (default view), `ResourceManager` (шрифты/иконки), `DebugManager` (строка, видимость), события инвентаря (itemMeta: texture/region). Debug шрифт ищется по алиасу `debug` → `fonts/ArialRegular.ttf` (fallback RobotoMono).

## Менеджеры (ключевые сервисы)
- `WindowManager` — окно SFML, текущий `sf::View`.
- `CameraManager` — центр/zoom/размер в world units; влияет на рендер.
- `ResourceManager` — загрузка текстур/шрифтов/регистрация атласных регионов; базовые пути из `config/game.json` (`resources/`, `textures/`, `fonts/`).
- `PhysicsManager` — мир Box2D, создание тел/фикстур.
- `TimeManager` — dt между кадрами.
- `DebugManager` — хранит debug-строку и флаг видимости.

## Данные/компоненты (основные)
- `TransformComponent` — позиция/масштаб/поворот в world units.
- `TilemapComponent` — тайлы, размеры, `tile_size`, `origin`, `tile_id_to_region`, `solid_ids`, `visible`.
- `SpriteComponent` — atlasRegion или textureName + rect, scale, origin, visible, z.
- `PhysicsBodyComponent` — ссылка на физ. тело, флаги для синхронизации.
- `InputComponent` — состояние действий (pressed/held/released).
- `GroundedComponent` — флаг касания земли.
- `InventoryComponent` (через Mechanics2) — слоты, активный индекс, данные предметов.
- `DropComponent` — данные дропа (itemId/count).
- `Tags` — метки сущностей.

## Конфиги и ресурсы (что потребляют системы)
- `config/game.json`: окно, `resources_path/textures_path/fonts_path`, `world.map_file` (сейчас `maps/level_house.json`), `world.tile_size=32`, `inventory_file`.
- `config/input.json`: биндинги (ходьба/прыжок/break/place, slot_prev/next, slot_1..10, wheel aliases inventory_prev/next).
- `config/inventory.json`: слоты/хотбар, предметы (itemId → icon_region/icon_texture, max_stack, place_tile_id), стартовые предметы.
- Карты `config/maps/*.json`: размеры, `tile_size` (world units), `origin` top-left, `tiles`, `solid_ids`, `tile_id_to_region`, `player_spawn`. Текущая демо: `level_house.json`.
- Атлас: `resources/textures/tileMap.png` (fallback `tiles.png`), регионы 32x32: ground(0,0), path(1,0), grass_alt(6,0), leaves(6,1), water(10,0), stone_brick(11,0), dirt(12,0), roof(13,0), trunk(1,1); tileId 1..9 → регионы, пусто=-1.
- Шрифты: `resources/fonts/ArialRegular.ttf` (alias debug, fallback RobotoMono).

## Связность и зависимости
- Системы связаны через компоненты и события, но есть жёсткие точки:
  - `RenderSystem` зависит от корректного `tile_id_to_region` и регистрации регионов в `ResourceManager`.
  - `PhysicsSystem` требует согласованных `solid_ids`, `tile_size`, origin (top-left, Y вниз) — иначе коллизии/Place/Break будут смещены.
  - `UIRenderSystem` зависит от `InventoryStateChanged` (снапшот + itemMeta) и наличия шрифта по алиасу `debug`.
  - `InputSystem` опирается на валидный `config/input.json`; отсутствующие бинды приводят к пропуску действий.
- Ресурсы/пути: исполняемый ожидает запуск из `build` с относительными путями `../resources`; в `scripts/run.sh` это соблюдено.
- Координатные соглашения: world units = метры/тайлы, render scale 32 px; ось Y вниз в данных карты и рендере, в физике вверх — нужна `CoordinateUtils` при добавлении нового кода.

## Уязвимые места и риски
- Атлас/ресурсы: отсутствие текстур/регионов → тихое отсутствие отрисовки (RenderSystem пропускает). Нужно следить за актуальностью `tile_id_to_region` и регистрацией регионов.
- Карты: несоответствие `tile_size`/origin/`solid_ids` нарушит физику Place/Break и коллизии; новые карты должны соблюдать top-left origin и Y вниз.
- Физика: per-tile тела без чанков — риск производительности на больших картах; нет агрегирования/LOD.
- Инвентарь/UI: если `InventoryStateChanged` не шлётся при attach или itemMeta пуст, UI покажет заглушки; несогласованные бинды wheel/цифр приведут к пропуску переключений.
- Шрифты: при отсутствии `ArialRegular.ttf` (и fallback) debug/UI текст не появится.
- Относительные пути: запуск не из корня/`build` нарушит загрузку `config/` и `resources/`.

## Как использовать/расширять безопасно
- Добавляя тайлы/регионы: обновляйте `tile_id_to_region` в карте и регистрируйте регионы в `ResourceManager`; держите `solid_ids` в синхроне.
- Новые предметы инвентаря: дополняйте `config/inventory.json` (itemId→icon_region/icon_texture, place_tile_id), передавайте itemMeta в `InventoryStateChanged`.
- Новые карты: сохраняйте origin top-left, ось Y вниз, корректный `tile_size`; проверяйте `solid_ids`.
- Если добавляете UI/иконки: гарантируйте наличие шрифтов/текстур, регистрируйте регионы и фиксируйте в доках.
