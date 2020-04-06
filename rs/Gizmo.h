//
// Created by Alexander Logan on 03/04/2020.
//

#ifndef RSPERKS_GIZMO_H
#define RSPERKS_GIZMO_H


#include "InventionTypes.h"
#include "Component.h"
#include "Perk.h"
#include "Probability.h"
#include <array>
#include <vector>


// Gizmo's generate pairs of perks.
struct GizmoResult {
    GeneratedPerk first;
    GeneratedPerk second;

    GizmoResult(GeneratedPerk first, GeneratedPerk second);

    bool operator==(const GizmoResult &other) const;
};

struct GizmoResultHash {
    std::size_t operator()(const GizmoResult &result) const;
};

std::ostream &operator<<(std::ostream &strm, const GizmoResult &gizmo_result);

// We generate gizmo result probabilities.
struct GizmoResultProbability {
    GizmoResult result;
    probability_t probability;
};

std::ostream &operator<<(std::ostream &strm, const GizmoResultProbability &gizmo_result_probability);

// Gizmos can produce a series of possible results with probabilities.
typedef std::vector<GizmoResultProbability> GizmoResultProbabilityList;

class Gizmo {
public:
    Gizmo() = delete;

    Gizmo(EquipmentType equipment_type, GizmoType gizmo_type, std::vector<Component> components);

    EquipmentType equipmentType() const;

    GizmoType type() const;

    const std::array<Component, 9> &components() const;

    std::array<Component, 9>::const_iterator begin() const;

    std::array<Component, 9>::const_iterator end() const;

    GizmoResult rollForPerks() const;

    GizmoResultProbabilityList perkProbabilities(level_t invention_level) const;

    GizmoResultProbabilityList targetPerkProbabilities(level_t invention_level,
                                                       const GizmoResult &target,
                                                       bool exact_target = true) const;

private:
    EquipmentType equipment_type_;
    GizmoType gizmo_type_;
    std::array<Component, 9> components_;
    std::vector<Perk> insertion_order_;

    std::vector<Perk> perkInsertionOrder() const;

    std::vector<CDF> perkRollCdf() const;

    std::vector<std::vector<std::pair<rank_t, probability_t>>> perkRankProbabilities() const;

    std::vector<std::pair<std::vector<GeneratedPerk>, probability_t>> perkCombinationProbabilities() const;

    [[maybe_unused]] std::vector<std::pair<std::vector<GeneratedPerk>, probability_t>>
    perkCombinationProbabilities(const GizmoResult &target, bool exact = true) const;

    GizmoResultProbabilityList gizmoResultProbabilities(level_t invention_level,
                                                        bool include_no_effect = false,
                                                        GizmoResult target = {{Perk::no_effect, 0},
                                                                              {Perk::no_effect, 0}},
                                                        bool exact_target = true) const;
};

std::ostream &operator<<(std::ostream &strm, const Gizmo &gizmo);


#endif //RSPERKS_GIZMO_H
