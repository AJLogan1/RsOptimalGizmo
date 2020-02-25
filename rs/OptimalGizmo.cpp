#include <set>

#include "OptimalGizmo.h"
#include "InventionCalc.h"

namespace {
    /**
     * Gets a vector of components which may possibly generate at least one of the target perks.
     */
    std::vector<rs::InventionComponent *>
    possibleComponentsFor(std::pair<rs::GizmoPerk, rs::GizmoPerk> target, rs::GizmoType type,
                          const std::vector<rs::InventionComponent *> &excluded_components) {
        // Get set of possible components for generating this combination.
        std::set<rs::InventionComponent *> possible_components;
        auto first_target = rs::components::componentsWhichCanGenerate(*target.first.perk, type);
        auto second_target = rs::components::componentsWhichCanGenerate(*target.second.perk, type);
        possible_components.insert(&rs::components::empty);
        possible_components.insert(first_target.begin(), first_target.end());
        possible_components.insert(second_target.begin(), second_target.end());

        // Remove excluded components.
        std::for_each(excluded_components.begin(), excluded_components.end(),
                      [&possible_components](const auto &c) { possible_components.erase(c); });

        // Get components as vector.
        std::vector<rs::InventionComponent *> possible_components_vector(possible_components.begin(),
                                                                         possible_components.end());

        return possible_components_vector;
    }
}

namespace rs {
    /**
     * Calculates possible gizmos for obtaining some target perks.
     * @param target A pair of GizmoPerks representing the target perks to search for.
     * @param type The gizmo type (as a GizmoType).
     * @param invention_level The invention level to search at.
     * @param excluded_components A vector of components to exclude from the search. Can be empty.
     * @param costs A mapping from components to cost to determine estimated price of Gizmo.
     * @return A vector of (Gizmo, (Probability of Obtaining Target, Estimated Cost for Target)).
     * Note the vector is returned unsorted.
     */
    std::vector<std::pair<Gizmo, std::pair<double, double>>>
    findPossibleGizmos(std::pair<rs::GizmoPerk, rs::GizmoPerk> target, rs::GizmoType type, uint8_t invention_level,
                       const std::vector<rs::InventionComponent *> &excluded_components,
                       const std::map<rs::InventionComponent *, double> &costs) {
        // Get vector of possible components.
        auto possible_components_vector = possibleComponentsFor(target, type, excluded_components);

        // Use possible components as the search vector.
        auto &component_search_vector = possible_components_vector;

        // Calculate search space size.
        size_t search_space_size = component_search_vector.size();
        search_space_size *= search_space_size;
        search_space_size *= search_space_size;
        search_space_size *= component_search_vector.size();

        if (search_space_size == 0) {
            // Not possible to generate this combination.
            return {};
        }

        // Thresholds for target perks.
        uint16_t target_1_threshold = target.first.perk->ranks.size() >= target.first.rank ?
                                      target.first.perk->ranks[target.first.rank - 1].threshold : 255;
        uint16_t target_2_threshold = target.second.perk->ranks.size() >= target.second.rank ?
                                      target.second.perk->ranks[target.second.rank - 1].threshold : 255;

        // Maximum possible gizmo contribution from a component.
        uint8_t max_t1_delta =
                (*std::max_element(component_search_vector.begin(),
                                   component_search_vector.end(),
                                   [&type, &target](const auto &a, const auto &b) {
                                       return a->totalPotentialContribution(type, target.first.perk) <
                                              b->totalPotentialContribution(type, target.first.perk);
                                   }))->totalPotentialContribution(type, target.first.perk);
        uint8_t max_t2_delta =
                (*std::max_element(component_search_vector.begin(),
                                   component_search_vector.end(),
                                   [&type, &target](const auto &a, const auto &b) {
                                       return a->totalPotentialContribution(type, target.second.perk) <
                                              b->totalPotentialContribution(type, target.second.perk);
                                   }))->totalPotentialContribution(type, target.second.perk);

        // Main search loop.
        // Vector to store results - reserve a fair bit of space to begin with.
        std::vector<std::pair<Gizmo, std::pair<double, double>>> results;
        results.reserve(search_space_size / 4);
        for (const auto &a : component_search_vector) {
            // Back four component encoding of all normal form gizmos tested with this first component.
            std::set<uint32_t> tested_nf_gizmos;

            uint8_t a_t1 = a->totalPotentialContribution(type, target.first.perk);
            uint8_t a_t2 = a->totalPotentialContribution(type, target.second.perk);

            for (const auto &b : component_search_vector) {
                // Calculate total target contribution from components so far.
                uint8_t b_t1 = b->totalPotentialContribution(type, target.first.perk);
                uint8_t b_t2 = b->totalPotentialContribution(type, target.second.perk);
                uint16_t b_t1_rt = a_t1 + b_t1;
                uint16_t b_t2_rt = b_t1 + b_t2;
                // If it is not possible to reach the target amount, move on to the next component.
                if (b_t1_rt + (max_t1_delta * 3) < target_1_threshold ||
                    b_t2_rt + (max_t2_delta * 3) < target_2_threshold) {
                    continue;
                }

                for (const auto &c : component_search_vector) {
                    // Calculate total target contribution from components so far.
                    uint8_t c_t1 = c->totalPotentialContribution(type, target.first.perk);
                    uint8_t c_t2 = c->totalPotentialContribution(type, target.second.perk);
                    uint16_t c_t1_rt = b_t1_rt + c_t1;
                    uint16_t c_t2_rt = b_t2_rt + c_t2;
                    // If it is not possible to reach the target amount, move on to the next component.
                    if (c_t1_rt + (max_t1_delta * 2) < target_1_threshold ||
                        c_t2_rt + (max_t2_delta * 2) < target_2_threshold) {
                        continue;
                    }

                    for (const auto &d : component_search_vector) {
                        // Calculate total target contribution from components so far.
                        uint8_t d_t1 = c->totalPotentialContribution(type, target.first.perk);
                        uint8_t d_t2 = c->totalPotentialContribution(type, target.second.perk);
                        uint16_t d_t1_rt = c_t1_rt + c_t1;
                        uint16_t d_t2_rt = c_t2_rt + c_t2;
                        // If it is not possible to reach the target amount, move on to the next component.
                        if (d_t1_rt + (max_t1_delta * 2) < target_1_threshold ||
                            d_t2_rt + (max_t2_delta * 2) < target_2_threshold) {
                            continue;
                        }

                        for (const auto &e : component_search_vector) {
                            uint8_t e_t1 = e->totalPotentialContribution(type, target.first.perk);
                            uint8_t e_t2 = e->totalPotentialContribution(type, target.second.perk);
                            uint16_t t1_total = a_t1 + b_t1 + c_t1 + d_t1 + e_t1;
                            uint16_t t2_total = a_t2 + b_t2 + c_t2 + d_t2 + e_t2;
                            if (t1_total < target_1_threshold || t2_total < target_2_threshold) {
                                // Impossible to generate at least one of the two target perks.
                                continue;
                            }

                            // Create test Gizmo (implicitly converted to normal form).
                            rs::Gizmo gizmo = {type, a, b, c, d, e};
                            if (tested_nf_gizmos.count(gizmo.backFourEncoding())) {
                                // Already tested this normal form Gizmo.
                                continue;
                            }
                            tested_nf_gizmos.insert(gizmo.backFourEncoding());

                            // Calculate perk probabilities.
                            auto res = rs::perkProbabilities(gizmo, invention_level);

                            // Find probability for this gizmo.
                            double gizmo_probability = 0;
                            gizmo_probability = std::accumulate(res.possible_perks.begin(),
                                                                res.possible_perks.end(),
                                                                0.0,
                                                                [&target](double x, const auto &pp) {
                                                                    return pp.perk1 == target.first &&
                                                                           pp.perk2 == target.second ?
                                                                           x + pp.probability : x;
                                                                });

                            // Add to possible gizmos.
                            if (gizmo_probability > 0) {
                                results.emplace_back(gizmo,
                                                     std::make_pair(gizmo_probability,
                                                                    gizmo.cost(costs) / gizmo_probability));
                            }
                        }
                    }
                }
            }
        }

        return results;
    }
}
