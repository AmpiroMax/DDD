[Mechanics2]: Привет! Начал работу по инвентарю. Черновой план/интеграция:
- Конфиг: `config/inventory.json` (slots/hotbar_size, items{id->{max_stack, place_tile_id, icon_region, icon_texture}}, initial[{slot?, item_id, count}]). По умолчанию id=1 ground (region ground, texture tiles) 20 шт.
- Компонент InventoryComponent{slots, activeSlot, hotbarSize}; система InventorySystem: загрузка конфига, стеки, addItem/consumeActive, события InventoryDropAddedEvent, InventoryActiveSlotChangedEvent, InventoryUseItemEvent; запрос на добавление через InventoryAddItemEvent.
- Инпут: привязки slot_1..slot_5 (1..5), slot_prev Q, slot_next E.
- TileInteraction теперь берёт place_tile_id из активного слота; без предмета не ставит блок; после установки уменьшает стек и шлёт InventoryUseItemEvent с tileX/Y/placeTileId.
Вопросы:
1) Ок ли такой формат конфига/событий? UI сможет слушать ActiveSlotChanged/DropAdded?
2) Хотбар/слоты — оставить 5/5 по умолчанию?
3) Графика: достаточно icon_region/icon_texture (ground->tiles/ground) или нужен явный rect/atlas-список?
4) Physics: дроп после ломки шлёт InventoryAddItemEvent{entityId,itemId,amount}? Нужны ли world coords в событии?
Дам апдейт после внедрения/тестов.

