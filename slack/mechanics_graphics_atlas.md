[TL]: Парный чат Mechanics × Graphics (атлас/тайлы). Формат: `[Name | i]: текст`, сообщения разделяйте пустой строкой.

- Цель: утвердить mapping tileId → atlasRegion → {texture, rect}, пути к файлам, ориентацию и tile_size.
- База: tile_id_to_region приходит из карты (`config/maps/level01.json`), origin верхний левый, данные tiles вниз, tile_size в world units (1 = 1 тайл). Пример сейчас: {1:"ground"}.
- Предложение: атлас `resources/textures/tiles.png`, region `ground` {x:0,y:0,w:32,h:32}; уточните финальные прямоугольники и имена файлов/регионов.
- Итоговый список нужен графике (для ResourceManager) и техпису (docs). Закройте договорённость здесь.

[Mechanics | 1]: Предлагаю зафиксировать минимально: texture `textures/tiles.png`, region `ground` = {x:0,y:0,w:32,h:32}, tileId 1 → ground, empty_id -1, tile_size 1.0 world unit (32 px). Если хотите больше регионов (stone/dirt/background и т.п.) — дайте финальный список {name, rect} и соответствующие tileId, я обновлю `config/maps/level01.json` и отправлю техпису.
[Mechanics | 2]: Итоговая маппилка (tileMap.png 32px grid, origin top-left):
- 1 `ground` -> rect (0,0,32,32) — трава.
- 2 `path` -> (32,0,32,32) — тёплый/жёлтый (дорожка/доска).
- 3 `grass_alt` -> (192,0,32,32) — зелёный вариант.
- 4 `leaves` -> (192,32,32,32) — зелёные листья.
- 5 `water` -> (320,0,32,32) — тёмно-синий (вода/окно-заглушка).
- 6 `stone_brick` -> (352,0,32,32) — серо-каменная текстура (стены/камень).
- 7 `dirt` -> (384,0,32,32) — земля/грунт.
- 8 `roof` -> (416,0,32,32) — светлый/оранжевый (крыша/дерево).
- 9 `trunk` -> (32,32,32,32) — тёмный столб (ствол/колонна).
Пустой = -1. Можно добавить остальные серые тайлы (row1 cols2..6) позже под stone variants.
[Graphics | 1]: Принял базу. Предлагаю зафиксировать для MVP: atlas `textures/tiles.png`, region `ground` rect {0,0,32,32}, mapping tileId 1 -> ground, emptyId = -1. Если у вас появятся новые tileId/regions, пришлите полный список `tileId -> {region, texture, rect}` — добавлю в ResourceManager/доки. tile_size читаем из карты (world units), origin top-left, y вниз, render scale 32. Ок?
