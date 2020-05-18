//
// Created by Alexander Logan on 05/04/2020.
//

#ifndef RSPERKS_OPTIMALGIZMOSEARCH_H
#define RSPERKS_OPTIMALGIZMOSEARCH_H

#include <atomic>

#include "InventionTypes.h"
#include "Gizmo.h"
#include "Component.h"


struct GizmoTargetProbability {
    GizmoTargetProbability(const Gizmo *g, probability_t p) : gizmo(g), target_probability(p) {}

    const Gizmo *gizmo;
    probability_t target_probability;
};

std::ostream &operator<<(std::ostream &strm, const GizmoTargetProbability &result);


// Struct to store search progress information.
// Deliberately increased size to 64-bytes to ensure instances reside in different cache lines.
struct SubsearchProgress {
    // 8 bytes
    int64_t results_searched;

    // 7 * 8 bytes of padding
    int64_t padding__[7] = {0};
};


class OptimalGizmoSearch {
public:
    OptimalGizmoSearch(EquipmentType equipment,
                       GizmoType gizmo_type,
                       GizmoResult target);

    size_t build_candidate_list(const std::vector<Component> &excluded);

    std::vector<GizmoTargetProbability> results(level_t invention_level, int thread_count = 1);

    size_t resultsSearched();

    size_t total_candidates;

private:
    EquipmentType equipment_type_;
    GizmoType gizmo_type_;
    GizmoResult target_;

    std::vector<Gizmo> candidate_gizmos_;

    std::vector<SubsearchProgress> thread_progress_;

    std::vector<Component> targetPossibleComponents(const std::vector<Component> &excluded) const;

    std::vector<Gizmo> candidateGizmos(const std::vector<Component> &excluded) const;

    std::vector<GizmoTargetProbability> targetSearchResults(level_t invention_level, size_t thread_count = 1);
};


#endif //RSPERKS_OPTIMALGIZMOSEARCH_H
