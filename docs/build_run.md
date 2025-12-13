# Build & Run Guide

## Зачем
Краткие шаги, чтобы собрать и запустить MVP (Terraria-like) локально.

## Требования
- CMake ≥ 3.20
- Компилятор с C++20 (Clang 14+/GCC 11+)
- SFML 2.5, Box2D, nlohmann_json (должны быть установлены в систему)
- macOS/Linux: наличие `pkg-config` упростит поиск библиотек

### Пример установки (macOS, Homebrew)
```sh
brew install cmake sfml box2d nlohmann-json
```

## Конфиги и данные
- `config/game.json` — окно, пути ресурсов, параметры игрока/мира; `world.map_file` сейчас `maps/level_house.json`, `world.tile_size = 32` (1 world unit). `inventory_file = inventory.json`.
- `config/input.json` — биндинги клавиш/мыши, хотбар/инвентарь: `slot_prev/next` (Q/E + wheel Up/Down), `slot_1..10` (цифры 1–0), алиасы `inventory_prev/next`, `inventory_slot_1..10`, бинды break/place/jump/движение как раньше.
- `config/inventory.json` — размер слотов/хотбара, определения предметов (`icon_region/icon_texture`, `place_tile_id`), стартовые предметы (по умолчанию 20 блоков ground в слоте 0).
- Карты `config/maps/*.json`: `width/height`, `tile_size` (world units), `origin` (0,0 вверху слева, ось Y вниз в данных), `tiles` (строки), `solid_ids`, `player_spawn`, `tile_id_to_region`. Текущая демо: `level_house.json`.
- Ресурсы читаются из `resources/`, `textures/`, `fonts/` (относительно корня проекта); при запуске из `build` пути остаются относительными `../resources`, copy-step не требуется.
- Атлас: `resources/textures/tileMap.png` (fallback `textures/tiles.png`). Регионы (32x32): `ground(0,0)`, `path(1,0)`, `grass_alt(6,0)`, `leaves(6,1)`, `water(10,0)`, `stone_brick(11,0)`, `dirt(12,0)`, `roof(13,0)`, `trunk(1,1)`. Маппинг tileId: 1→ground, 2→path, 3→grass_alt, 4→leaves, 5→water, 6→stone_brick, 7→dirt, 8→roof, 9→trunk; пусто = -1.
- Шрифты: alias `debug` указывает на `resources/fonts/ArialRegular.ttf` (fallback RobotoMono).

## Сборка
```sh
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```
- Итоговый бинарь: `build/DDDGame`.
- Для релиза переключите `-DCMAKE_BUILD_TYPE=Release`.
- Альтернатива: `scripts/run.sh` — соберёт при отсутствии бинаря и запустит из корня (путь к ресурсам/конфига корректный).

## Запуск
Из каталога `build` (чтобы относительные пути к `config/` и `resources/` резолвились корректно):
```sh
./DDDGame
```
- По умолчанию окно 1280x720 с заголовком `DDD Terraria-like MVP`.
- Если шрифт `resources/fonts/debug.ttf` отсутствует, debug-оверлей будет отключён (UIRenderSystem проверяет наличие шрифта).

## Ожидаемое поведение сейчас
- Игровой цикл: фиксированный шаг физики (`PHYSICS_TIMESTEP`), рендер в основном цикле.
- При отсутствии ресурсов/атласа спрайты/тайлы не рисуются (RenderSystem пропускает неизвестные регионы/текстуры).
- Debug overlay опционален: показывает FPS и debug-строку, если `debugManager.isVisible()` и найден alias `debug` (`resources/fonts/ArialRegular.ttf` или RobotoMono).
- Инвентарь/хотбар: wheel и цифры переключают слоты; UI показывает активный слот, qty и иконку (если есть region). По умолчанию один предмет ground.

## Открытые вопросы
- Если добавятся новые регионы/ассеты, нужно обновить `tile_id_to_region` и список текстур в доке.
