#ifndef DDD_EVENTS_TILE_EVENTS_H
#define DDD_EVENTS_TILE_EVENTS_H

#include "core/Entity.h"

struct PlaceBlockEvent {
    int x{0};
    int y{0};
    int tileId{0};
};

struct BreakBlockEvent {
    int x{0};
    int y{0};
    int previousTileId{0};
};

#endif // DDD_EVENTS_TILE_EVENTS_H

