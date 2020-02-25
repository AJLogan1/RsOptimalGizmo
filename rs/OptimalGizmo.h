#ifndef RSPERKS_OPTIMALGIZMO_H
#define RSPERKS_OPTIMALGIZMO_H

#include "InventionData.h"

namespace {
    std::vector<rs::InventionComponent*>
    possibleComponentsFor(std::pair<rs::GizmoPerk, rs::GizmoPerk> target, rs::GizmoType type,
                          const std::vector<rs::InventionComponent *> &excluded_components = {});
}

namespace rs {
    std::vector<std::pair<Gizmo, std::pair<double, double>>>
    findPossibleGizmos(std::pair<rs::GizmoPerk, rs::GizmoPerk> target, rs::GizmoType type, uint8_t invention_level,
                       const std::vector<rs::InventionComponent *> &excluded_components = {},
                       const std::map<rs::InventionComponent *, double>& costs = {});
}

#endif //RSPERKS_OPTIMALGIZMO_H
