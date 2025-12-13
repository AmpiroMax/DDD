[TL]: Парный чат Graphics × TechWriter (ресурсы/доки). Формат: `[Name | i]: текст`, сообщения разделяйте пустой строкой.

- Цель: зафиксировать финальные пути ресурсов, список атласных регионов и что нужно документировать.
- База: base `resources/`, поддиры `textures/`, `fonts/`; overlay ищет `fonts/debug.ttf`.
- Нужен список регионов (tileId -> region -> {texture, rect}) и имена файлов (атлас/спрайты/шрифты) — отдаём техпису для обновления `docs/build_run.md` и `docs/overview_and_map.md`.
- Обсудите, нужен ли JSON-файл атласа или регистрируем регионы в коде. После решения — коротко зафиксируйте итог здесь.

[Mechanics | 1]: Минимальный набор для MVP: texture `textures/tiles.png`, region `ground` = {x:0,y:0,w:32,h:32}, tileId 1 → ground, empty_id -1, tile_size 1.0 world unit (32 px). ResourceManager при старте грузит tiles.png и регистрирует ground, а при загрузке карты регистрирует недостающие регионы по умолчанию (square tile_size*RENDER_SCALE). Если добавляем новые регионы — нужен список {tileId, region, texture, rect}. Шрифт debug остаётся `fonts/debug.ttf`.

[TL | 2]: Финал для доков: ресурсы лежат в `resources/` (`textures/tiles.png` с регионом ground 0,0,32,32; `fonts/debug.ttf` — опционально). TechWriter — внеси это в build_run/overview. Graphics — будь готов расширять атлас при появлении новых tileId; держите список {tileId, region, texture, rect} для доков.
[Graphics | 3]: Апдейт финала: фактический тайлсет сейчас `resources/textures/tileMap.png` (fallback `tiles.png`), регион ground {0,0,32,32}, tileId 1 → ground, empty_id -1, tile_size 1.0 wu. Debug-шрифт alias `debug` указывает на `resources/fonts/ArialRegular.ttf` (fallback RobotoMono), отдельный debug.ttf не обязателен. Техпис, пожалуйста, обнови build_run/overview этим списком. Если появятся новые tileId/регионы (например, иконки инвентаря) — пришлю список {tileId, region, texture, rect}.
