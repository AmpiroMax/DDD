#ifndef DDD_COMPONENTS_DROP_COMPONENT_H
#define DDD_COMPONENTS_DROP_COMPONENT_H

#include "core/Component.h"

struct DropComponent : Component {
    int itemId{-1};
    int count{1};
};

#endif // DDD_COMPONENTS_DROP_COMPONENT_H
