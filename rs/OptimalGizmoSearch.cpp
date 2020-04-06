//
// Created by Alexander Logan on 05/04/2020.
//

#include "OptimalGizmoSearch.h"
#include <bitset>
#include <iomanip>


std::ostream &operator<<(std::ostream &strm, const GizmoTargetProbability &result) {
    return strm << *result.gizmo << std::endl << "Target Probability: "
                << std::scientific << std::setprecision(6) << 100 * result.target_probability << "%";
}

OptimalGizmoSearch::OptimalGizmoSearch(EquipmentType equipment, GizmoType gizmo_type, GizmoResult target) :
        equipment_type_(equipment),
        gizmo_type_(gizmo_type),
        target_(target) {

}

size_t OptimalGizmoSearch::build_candidate_list() {
    candidate_gizmos_ = candidateGizmos();
    total_candidates = candidate_gizmos_.size();
    return total_candidates;
}

std::vector<GizmoTargetProbability> OptimalGizmoSearch::results(level_t invention_level) {
    return targetSearchResults(invention_level);
}

std::vector<Component> OptimalGizmoSearch::targetPossibleComponents() const {
    std::vector<Component> possible_components;
    std::copy_if(Component::all().begin(), Component::all().end(), std::back_inserter(possible_components),
                 [&](Component c) {
                     if (gizmo_type_ != ANCIENT && c.ancient()) {
                         return false;
                     }
                     auto component_perks = c.perkContributions(equipment_type_);
                     return std::any_of(component_perks.begin(), component_perks.end(),
                                        [&](const PerkContribution &contrib) {
                                            return contrib.perk == target_.first.perk ||
                                                   contrib.perk == target_.second.perk;
                                        });
                 });
    return possible_components;
}

std::vector<Gizmo> OptimalGizmoSearch::candidateGizmos() const {
    std::vector<Component> possible_components = targetPossibleComponents();
    if (possible_components.size() == 0) {
        return {};
    }

    std::vector<Gizmo> candidates;
    candidates.reserve(32000);

    // Thresholds for target perks.
    rank_threshold_t target_1_threshold = target_.first.perk.rank(target_.first.rank).threshold;
    rank_threshold_t target_2_threshold = target_.second.perk.rank(target_.second.rank).threshold;

    // Maximum possible gizmo contribution from a component.
    size_t max_target_1_contrib = std::max_element(possible_components.begin(),
                                                   possible_components.end(),
                                                   [&](const Component &c1, const Component &c2) {
                                                       return c1.totalPotentialContribution(equipment_type_,
                                                                                            target_.first.perk.id) <
                                                              c2.totalPotentialContribution(equipment_type_,
                                                                                            target_.first.perk.id);
                                                   })->totalPotentialContribution(equipment_type_,
                                                                                  target_.first.perk.id);
    size_t max_target_2_contrib = std::max_element(possible_components.begin(),
                                                   possible_components.end(),
                                                   [&](const Component &c1, const Component &c2) {
                                                       return c1.totalPotentialContribution(equipment_type_,
                                                                                            target_.second.perk.id) <
                                                              c2.totalPotentialContribution(equipment_type_,
                                                                                            target_.second.perk.id);
                                                   })->totalPotentialContribution(equipment_type_,
                                                                                  target_.second.perk.id);

    // Vector to hold current configurations.
    std::vector<Component> current_configuration(slotsForType(gizmo_type_), Component::empty);
    // Loop through, adding candidate gizmos.
    std::vector<size_t> indices(slotsForType(gizmo_type_), 0);
    while (indices[0] < possible_components.size()) {
        size_t t1_contrib_remaining = max_target_1_contrib * slotsForType(gizmo_type_);
        size_t t2_contrib_remaining = max_target_2_contrib * slotsForType(gizmo_type_);

        // Ensure only normal form gizmos are generated.
        std::bitset<std::numeric_limits<perk_id_t>::max()>
                possible_perks = possible_components[indices[0]].possiblePerkBitset(equipment_type_);
        size_t current_t1_contrib = possible_components[indices[0]].totalPotentialContribution(equipment_type_,
                                                                                               target_.first.perk.id);
        size_t current_t2_contrib = possible_components[indices[0]].totalPotentialContribution(equipment_type_,
                                                                                               target_.second.perk.id);

        bool indifferent = false;
        for (size_t i = 1; i < indices.size(); ++i) {
            size_t idx = indices[i];
            t1_contrib_remaining -= max_target_1_contrib;
            t2_contrib_remaining -= max_target_2_contrib;

            // Check if the current component adds new possible perks.
            auto current_comp_possible_perks = possible_components[idx].possiblePerkBitset(equipment_type_);
            bool contributes_new = (possible_perks & current_comp_possible_perks) != current_comp_possible_perks;

            if (indifferent && contributes_new) {
                // If this current component does not generate new perks, advance until we find one that does.
                for (size_t reset_idx = i + 1; reset_idx < indices.size(); ++reset_idx) {
                    indices[reset_idx] = possible_components.size() - 1;
                }
                goto skip;
            }

            // If currently indifferent, check the ID's are in order (if not move on).
            if (indifferent && possible_components[indices[i - 1]].id > possible_components[idx].id) {
                for (size_t reset_idx = i + 1; reset_idx < indices.size(); ++reset_idx) {
                    indices[reset_idx] = possible_components.size() - 1;
                }
                goto skip;
            }

            current_t1_contrib += possible_components[idx].totalPotentialContribution(equipment_type_,
                                                                                      target_.first.perk.id);
            current_t2_contrib += possible_components[idx].totalPotentialContribution(equipment_type_,
                                                                                      target_.second.perk.id);

            // Check it'll be possible to generate the targets.
            if (current_t1_contrib + t1_contrib_remaining < target_1_threshold ||
                current_t2_contrib + t2_contrib_remaining < target_2_threshold) {
                // Cannot possibly reach one of the targets from this point forwards.
                for (size_t reset_idx = i + 1; reset_idx < indices.size(); ++reset_idx) {
                    indices[reset_idx] = possible_components.size() - 1;
                }
                goto skip;
            }

            indifferent = !contributes_new;
            possible_perks = possible_perks | current_comp_possible_perks;
        }

        // Add the current configuration to our candidate gizmo list.
        std::transform(indices.begin(), indices.end(), current_configuration.begin(), [&](size_t idx) {
            return possible_components[idx];
        });
        candidates.emplace_back(equipment_type_, gizmo_type_, current_configuration);

        // Increment indices.
        skip:
        for (size_t i = indices.size() - 1; i < indices.size(); --i) {
            indices[i]++;
            if (indices[i] >= possible_components.size()) {
                if (i == 0) {
                    break;
                }

                indices[i] = 0;
                continue;
            }

            break;
        }
    }

    return candidates;
}

std::vector<GizmoTargetProbability> OptimalGizmoSearch::targetSearchResults(level_t invention_level) {
    results_searched = 0;
    const GizmoResult &target__ = target_;
    std::vector<GizmoTargetProbability> results;
    results.reserve(candidate_gizmos_.size());

    for (const Gizmo &candidate : candidate_gizmos_) {
        auto possible_results = candidate.targetPerkProbabilities(invention_level, target__);
        probability_t total_gizmo_probability = std::accumulate(possible_results.begin(),
                                                                possible_results.end(),
                                                                0.0,
                                                                [](probability_t x,
                                                                   const GizmoResultProbability &result) {
                                                                    return x + result.probability;
                                                                });
        if (total_gizmo_probability > 0) {
            results.emplace_back(&candidate, total_gizmo_probability);
        }
        results_searched++;
    }

    // Sort results by inverse probability.
    std::sort(results.begin(), results.end(), [](const auto &a, const auto &b) {
        return a.target_probability > b.target_probability;
    });

    return results;
}
