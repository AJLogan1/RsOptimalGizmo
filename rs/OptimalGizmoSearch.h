//
// Created by Alexander Logan on 05/04/2020.
//

#ifndef RSPERKS_OPTIMALGIZMOSEARCH_H
#define RSPERKS_OPTIMALGIZMOSEARCH_H


#include "InventionTypes.h"
#include "Gizmo.h"
#include "Component.h"


struct GizmoTargetProbability {
    GizmoTargetProbability(const Gizmo *g, probability_t p) : gizmo(g), target_probability(p) {}

    const Gizmo *gizmo;
    probability_t target_probability;
};

std::ostream &operator<<(std::ostream &strm, const GizmoTargetProbability &result);


class OptimalGizmoSearch {
public:
    OptimalGizmoSearch(EquipmentType equipment,
                       GizmoType gizmo_type,
                       GizmoResult target);

    size_t build_candidate_list();

    std::vector<GizmoTargetProbability> results(level_t invention_level);

    size_t total_candidates;
    size_t results_searched;

private:
    EquipmentType equipment_type_;
    GizmoType gizmo_type_;
    GizmoResult target_;

    std::vector<Gizmo> candidate_gizmos_;

    std::vector<Component> targetPossibleComponents() const;

    std::vector<Gizmo> candidateGizmos() const;

    std::vector<GizmoTargetProbability> targetSearchResults(level_t invention_level);
};


#endif //RSPERKS_OPTIMALGIZMOSEARCH_H
