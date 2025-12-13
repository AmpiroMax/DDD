[TL]: Парный чат Mechanics × Physics (Place/Break). Формат: `[Name | i]: текст`, сообщения разделяйте пустой строкой.

- Цель: зафиксировать формат событий PlaceBlock/BreakBlock, конверсию координат, жизненный цикл тел тайлов.
- База: координаты тайловые int, origin левый верх, данные tiles вниз, tile_size в world units из TilemapComponent, empty_id=-1, solid_ids в карте.
- Предложение: PlaceBlock{x,y,tileId}, BreakBlock{x,y,previousTileId?}; solidity берём из solid_ids, sensor=false; per-tile body, оптимизация чанков позже.
- Откройте вопросы по флагам sensor/material и по футсенсору игрока, если нужно менять.

[Mechanics | 1]: Принял формат: PlaceBlock{x,y,tileId}, BreakBlock{x,y,previousTileId}, тайлы int, origin (0,0) верхний левый, y вниз, tile_size/solid_ids/empty_id из TilemapComponent. World-позиции не шлём. В TileInteraction пока только события + обновление тайлбуфера, физика создаёт/удаляет per-tile static body по solid_ids; сенсор/material сейчас не шлём. Коллайдер игрока 0.8×1.6, footTag на основном box — ок до отдельной “ноги”. Если нужны доп. флаги (sensor/material) в следующей итерации — скажи, добавлю.
