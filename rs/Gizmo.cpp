//
// Created by Alexander Logan on 03/04/2020.
//

#include "Gizmo.h"
#include "RSSort.h"
#include <bitset>

GizmoResult::GizmoResult(GeneratedPerk first, GeneratedPerk second) {
    if (first.rank == 0) {
        first.perk = Perk::no_effect;
    }
    if (second.rank == 0) {
        second.perk = Perk::no_effect;
    }
    if (first.rank > second.rank || (first.rank == second.rank && first.perk.id > second.perk.id)) {
        this->first = first;
        this->second = second;
    } else {
        this->first = second;
        this->second = first;
    }
}

bool GizmoResult::operator==(const GizmoResult &other) const {
    return this->first.perk.id == other.first.perk.id &&
           this->first.rank == other.first.rank &&
           this->second.perk.id == other.second.perk.id &&
           this->second.rank == other.second.rank;
}

std::size_t GizmoResultHash::operator()(const GizmoResult &result) const {
    return std::hash<int>{}((result.first.perk.id << 24) |
                            (result.first.rank << 16) |
                            (result.second.perk.id << 8) |
                            (result.second.rank));
}

Gizmo::Gizmo(EquipmentType equipment_type, GizmoType gizmo_type, std::vector<Component> components) {
    this->equipment_type_ = equipment_type;
    this->gizmo_type_ = gizmo_type;
    assert(!(gizmo_type == STANDARD) || (components.size() <= slotsForType(STANDARD)));
    assert(!(gizmo_type == ANCIENT) || (components.size() <= slotsForType(ANCIENT)));
    auto last_specified = std::copy(components.begin(), components.end(), components_.begin());
    std::fill(last_specified, components_.end(), Component::empty);

    // Build insertion order.
    insertion_order_ = perkInsertionOrder();
}

std::array<Component, 9>::const_iterator Gizmo::begin() const {
    return components_.begin();
}

std::array<Component, 9>::const_iterator Gizmo::end() const {
    switch (gizmo_type_) {
        case STANDARD:
            return components_.begin() + 5;
        case ANCIENT:
            return components_.end();
    }
    return nullptr;
}

GizmoResultProbabilityList Gizmo::perkProbabilities(level_t invention_level) const {
    return gizmoResultProbabilities(invention_level);
}

GizmoResultProbabilityList Gizmo::targetPerkProbabilities(level_t invention_level,
                                                          GizmoResult target,
                                                          bool exact_target) const {
    return gizmoResultProbabilities(invention_level, true, target, exact_target);
}

std::vector<perk_id_t> Gizmo::perkInsertionOrder() const {
    std::bitset<std::numeric_limits<perk_id_t>::max()> perk_set;
    std::vector<perk_id_t> insertion_order;
    std::for_each(begin(), end(), [&](Component comp) {
        auto component_perks = comp.perkContributions(this->equipment_type_);
        std::for_each(component_perks.begin(), component_perks.end(), [&](PerkContribution contrib) {
            if (!perk_set.test(contrib.perk.id)) {
                insertion_order.emplace_back(contrib.perk.id);
                perk_set.set(contrib.perk.id);
            }
        });
    });
    return insertion_order;
}

std::vector<CDF> Gizmo::perkRollCdf() const {
    std::vector<perk_id_t> insertion_order = this->insertion_order_;
    std::unordered_map<perk_id_t, int> bases;
    std::unordered_map<perk_id_t, std::vector<int>> rolls;
    std::for_each(begin(), end(), [&](Component comp) {
        auto component_perks = comp.perkContributions(this->equipment_type_);
        std::for_each(component_perks.begin(), component_perks.end(), [&](PerkContribution contrib) {
            bases[contrib.perk.id] += (this->gizmo_type_ == ANCIENT && !comp.ancient()) ?
                                      0.8 * contrib.base : contrib.base;
            rolls[contrib.perk.id].emplace_back((this->gizmo_type_ == ANCIENT && !comp.ancient()) ?
                                                0.8 * contrib.roll : contrib.roll);
        });
    });

    std::vector<CDF> cdfs;

    for (perk_id_t perk : insertion_order) {
        CDF base_cdf = CDF(bases[perk], 0);
        CDF perk_cdf = Cdf(rolls[perk]);
        base_cdf.insert(base_cdf.end(), perk_cdf.begin(), perk_cdf.end());
        cdfs.push_back(base_cdf);
    }

    return cdfs;
}

std::vector<std::vector<std::pair<rank_t, probability_t>>> Gizmo::perkRankProbabilities() const {
    std::vector<std::vector<std::pair<rank_t, probability_t>>> rank_probabilities;
    std::vector<perk_id_t> insertion_order = this->insertion_order_;
    std::vector<CDF> perk_contrib_cdf = perkRollCdf();

    for (size_t i = 0; i < insertion_order.size(); ++i) {
        perk_id_t perk_id = insertion_order[i];
        const CDF &perk_cdf = perk_contrib_cdf[i];
        const std::vector<Rank> &ranks = Perk::ranks(perk_id);
        std::vector<std::pair<rank_t, probability_t>> perk_rank_probabilities;

        // Loop backwards through ranks.
        for (size_t rank_i = ranks.size() - 1; rank_i < ranks.size(); --rank_i) {
            rank_threshold_t threshold = ranks[rank_i].threshold;
            // Is threshold greater or equal to the maximum possible generated contribution?
            if (threshold > perk_cdf.size() - 1 ||
                (ranks[rank_i].ancient && this->gizmo_type_ != ANCIENT)) {
                // Cannot generate this perk.
                continue;
            }

            // Calculate the probability the roll will fall between this threshold and the rank above's
            // threshold.
            probability_t rank_prob;
            // Is this the first rank which is possible to generate?
            if (rank_i == ranks.size() - 1 ||
                ranks[rank_i + 1].threshold > perk_cdf.size() - 1 ||
                (this->gizmo_type_ != ANCIENT && ranks[rank_i + 1].ancient)) {
                if (threshold > 0) {
                    rank_prob = 1.0 - perk_cdf[threshold - 1];
                } else {
                    rank_prob = 1.0;
                }
            } else {
                if (threshold > 0) {
                    rank_prob = perk_cdf[ranks[rank_i + 1].threshold - 1] - perk_cdf[threshold - 1];
                } else {
                    rank_prob = perk_cdf[ranks[rank_i + 1].threshold - 1];
                }
            }

            // Add this to vector.
            if (rank_prob > 0) {
                perk_rank_probabilities.emplace_back(ranks[rank_i].rank, rank_prob);
            }

            // If value of CDF at this threshold is zero, we know we can't generate any perks of lower rank.
            if (perk_cdf[threshold] == 0) {
                break;
            }
        }

        // If the CDF at the minimum threshold is greater than zero, there is a chance we generate a zero-rank perk.
        // Add that probability now.
        if (perk_cdf[ranks[0].threshold] > 0) {
            if (ranks[0].threshold >= perk_cdf.size()) {
                perk_rank_probabilities.emplace_back(0, 1.0);
            } else {
                perk_rank_probabilities.emplace_back(0, perk_cdf[ranks[0].threshold - 1]);
            }
        }

        rank_probabilities.emplace_back(perk_rank_probabilities);
    }

    return rank_probabilities;
}

std::vector<std::pair<std::vector<GeneratedPerk>, probability_t>> Gizmo::perkCombinationProbabilities() const {
    std::vector<perk_id_t> insertion_order = this->insertion_order_;
    std::vector<std::vector<std::pair<rank_t, probability_t>>> perk_rank_probabilities = perkRankProbabilities();

    std::vector<std::pair<std::vector<GeneratedPerk>, probability_t>> perk_combinations;
    perk_combinations.reserve(512);

    std::vector<size_t> indices(perk_rank_probabilities.size());
    while (indices[0] < perk_rank_probabilities[0].size()) {
        probability_t combined_prob = 1.0;
        size_t i = 0;
        std::vector<GeneratedPerk> combination(insertion_order.size());
        for (i = 0; i < insertion_order.size(); ++i) {
            rank_t rank = perk_rank_probabilities[i][indices[i]].first;
            probability_t rank_probability = perk_rank_probabilities[i][indices[i]].second;
            combined_prob *= rank_probability;
            combination[i] = {{insertion_order[i]}, rank};
        }
        perk_combinations.emplace_back(combination, combined_prob);

        // Increment indices.
        for (i = indices.size() - 1; i < indices.size(); --i) {
            indices[i]++;
            if (indices[i] == perk_rank_probabilities[i].size()) {
                if (i == 0) {
                    break;
                }

                indices[i] = 0;
                continue;
            }

            break;
        }
    }

    return perk_combinations;
}

std::vector<std::pair<std::vector<GeneratedPerk>, probability_t>>
Gizmo::perkCombinationProbabilities(GizmoResult target, bool exact) const {
    std::vector<perk_id_t> insertion_order = this->insertion_order_;
    std::vector<std::vector<std::pair<rank_t, probability_t>>> perk_rank_probabilities = perkRankProbabilities();

    std::vector<std::pair<std::vector<GeneratedPerk>, probability_t>> perk_combinations;
    perk_combinations.reserve(512);

    std::vector<size_t> indices(perk_rank_probabilities.size());
    while (indices[0] < perk_rank_probabilities[0].size()) {
        probability_t combined_prob = 1.0;
        size_t i = 0;
        std::vector<GeneratedPerk> combination(insertion_order.size());
        for (i = 0; i < insertion_order.size(); ++i) {
            rank_t rank = perk_rank_probabilities[i][indices[i]].first;

            if (insertion_order[i] == target.first.perk.id) {
                if ((exact && rank != target.first.rank) || rank < target.first.rank) {
                    goto skip;
                }
            }
            if (insertion_order[i] == target.second.perk.id) {
                if ((exact && rank != target.second.rank) || rank < target.second.rank) {
                    goto skip;
                }
            }

            probability_t rank_probability = perk_rank_probabilities[i][indices[i]].second;
            combined_prob *= rank_probability;
            combination[i] = {{insertion_order[i]}, rank};
        }
        perk_combinations.emplace_back(combination, combined_prob);

        // Increment indices.
        skip:
        for (i = indices.size() - 1; i < indices.size(); --i) {
            indices[i]++;
            if (indices[i] == perk_rank_probabilities[i].size()) {
                if (i == 0) {
                    break;
                }

                indices[i] = 0;
                continue;
            }

            break;
        }
    }

    return perk_combinations;
}


GizmoResultProbabilityList Gizmo::gizmoResultProbabilities(level_t invention_level,
                                                           bool include_no_effect,
                                                           GizmoResult target,
                                                           bool exact_target) const {
    GizmoResultProbabilityList results;
    std::unordered_map<GizmoResult, probability_t, GizmoResultHash> result_total_probabilities;
    auto perk_combination_probabilities = perkCombinationProbabilities(target, exact_target);
    CDF budget_cdf = inventionBudgetCdf(invention_level, this->gizmo_type_ == ANCIENT);
    GeneratedPerk no_effect_result = {Perk::no_effect, 0};

    bool check_target = target.first.perk.id != no_effect_id;

    probability_t probability_sum = 0.0;

    // For each combination, sort it using modified quicksort and then calculate probabilities of the combination.
    for (const auto &combination : perk_combination_probabilities) {
        std::vector<GeneratedPerk> perks = combination.first;
        probability_t probability = combination.second;

        rs::quicksort(perks.begin(), perks.end(),
                      [](const GeneratedPerk &a) -> int { return static_cast<int>(a.cost()); });
        perks.insert(perks.begin(), no_effect_result);

        size_t prev_cost = budget_cdf.size() - 1;
        // Loop backwards through the sorted combinations to calculate probability of each occurring.
        for (size_t i = perks.size() - 1; i < perks.size(); --i) {
            if (perks[i].rank == 0) {
                continue;
            }

            for (size_t j = i - 1; j < i; --j) {
                GizmoResult perk_pair = {perks[i], perks[j]};

                size_t combo_cost = perk_pair.first.cost() + perk_pair.second.cost();

                // Skip if not our target.
                if (check_target) {
                    if (perk_pair.first.perk.id != target.first.perk.id) {
                        prev_cost = combo_cost;
                        continue;
                    }
                    if (exact_target && perk_pair.second.perk.id != target.second.perk.id) {
                        prev_cost = combo_cost;
                        continue;
                    }
                }

                // Cannot possibly generate a combination which costs more than a previously generated one.
                // (Or is greater than our initial previous cost - the max invention budget.)
                if (combo_cost >= prev_cost) {
                    continue;
                }

                // Finding P(inv_budget <= combo_cost)
                probability_t combo_probability = budget_cdf[prev_cost] - budget_cdf[combo_cost];
                prev_cost = combo_cost;

                if (combo_probability == 0) {
                    goto next_combination;
                }

                // If either perk was a two-slot, set the other to nothing.
                if (perk_pair.first.perk.twoSlot()) {
                    perk_pair.second = no_effect_result;
                }
                if (perk_pair.second.perk.twoSlot()) {
                    perk_pair.first = perk_pair.second;
                    perk_pair.second = no_effect_result;
                }

                probability_t result_probability = probability * combo_probability;
                result_total_probabilities[perk_pair] += result_probability;
                probability_sum += result_probability;
            }
            next_combination:
            continue;
        }
    }

    probability_t normalisation_divisor;
    if (check_target) {
        normalisation_divisor = 1.0;
    } else if (include_no_effect && probability_sum < 1.0) {
        result_total_probabilities[{{Perk::no_effect, 0},
                                    {Perk::no_effect, 0}}] += 1.0 - probability_sum;
        normalisation_divisor = 1.0;
    } else {
        normalisation_divisor = probability_sum;
    }

    // Convert results to vector.
    for (const auto &pair : result_total_probabilities) {
        results.push_back({pair.first, pair.second / normalisation_divisor});
    }

    return results;
}

std::ostream &operator<<(std::ostream &strm, const GizmoResult &gizmo_result) {
    if (gizmo_result.second.perk.id != no_effect_id) {
        return strm << gizmo_result.first << ", " << gizmo_result.second;
    } else {
        return strm << gizmo_result.first;
    }
}

std::ostream &operator<<(std::ostream &strm, const GizmoResultProbability &gizmo_result_probability) {
    return strm << gizmo_result_probability.result << " -- " << gizmo_result_probability.probability;
}
