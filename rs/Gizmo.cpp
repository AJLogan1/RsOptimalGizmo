//
// Created by Alexander Logan on 03/04/2020.
//

#include "Gizmo.h"
#include "RSSort.h"
#include <bitset>

GizmoResult::GizmoResult(GeneratedPerk first, GeneratedPerk second) {
//    if (first.perk.twoSlot()) {
//        this->first = first;
//        this->second = {Perk::no_effect, 0};
//        return;
//    }

//    if (second.perk.twoSlot()) {
//        this->first = {Perk::no_effect, 0};
//        this->second = {Perk::no_effect, 0};
//        return;
//    }

    if (second.perk.id == no_effect_id || first.perk.id < second.perk.id) {
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
    assert(!(gizmo_type == STANDARD) || (components.size() == slotsForType(STANDARD)));
    assert(!(gizmo_type == ANCIENT) || (components.size() == slotsForType(ANCIENT)));
    std::move(components.begin(), components.end(), components_.begin());
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
    std::vector<perk_id_t> insertion_order = perkInsertionOrder();
    std::unordered_map<perk_id_t, int> bases;
    std::unordered_map<perk_id_t, std::vector<contribution_roll_t>> rolls;
    std::for_each(begin(), end(), [&](Component comp) {
        auto component_perks = comp.perkContributions(this->equipment_type_);
        std::for_each(component_perks.begin(), component_perks.end(), [&](PerkContribution contrib) {
            bases[contrib.perk.id] += contrib.base;
            rolls[contrib.perk.id].emplace_back(contrib.roll);
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
    std::vector<perk_id_t> insertion_order = perkInsertionOrder();
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
    std::vector<perk_id_t> insertion_order = perkInsertionOrder();
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

GizmoResultProbabilityList Gizmo::gizmoResultProbabilities(level_t invention_level,
                                                           bool include_no_effect) const {
    GizmoResultProbabilityList results;
    std::unordered_map<GizmoResult, probability_t, GizmoResultHash> result_total_probabilities;
    auto perk_combination_probabilities = perkCombinationProbabilities();
    CDF budget_cdf = inventionBudgetCdf(invention_level, this->gizmo_type_ == ANCIENT);

    probability_t no_effect_chance = 1.0;

    // For each combination, sort it using modified quicksort and then calculate probabilities of the combination.
    for (const auto &combination : perk_combination_probabilities) {
        std::vector<GeneratedPerk> perks = combination.first;
        probability_t probability = combination.second;

        rs::quicksort(perks.begin(), perks.end(), [](const GeneratedPerk &a) -> int { return a.cost(); });
        perks.insert(perks.begin(), {Perk::no_effect});

        rank_cost_t prev_cost = 0;
        GeneratedPerk no_effect_result = {Perk::no_effect, 0};
        // Loop backwards through the sorted combinations to calculate probability of each occurring.
        for (size_t i = perks.size() - 1; i < perks.size(); --i) {
            if (perks[i].rank == 0) {
                continue;
            }

            bool first_perk_two_slot = perks[i].perk.twoSlot();

            for (size_t j = i - 1; j < i; --j) {
                GizmoResult perk_pair = {
                        perks[i],
                        (first_perk_two_slot || perks[j].perk.twoSlot()) ? no_effect_result : perks[j]
                };

                int combo_cost = perk_pair.first.cost() + perk_pair.second.cost();

                if (combo_cost >= budget_cdf.size()) {
                    // Cannot generate this combination (budget too low).
                    continue;
                }

                // Cannot possibly generate a combination which costs more than a previously generated one.
                if (prev_cost != 0 && combo_cost >= prev_cost) {
                    continue;
                }

                probability_t combo_probability;
                if (prev_cost == 0) {
                    // Finding P(inv_budget <= combo_cost)
                    combo_probability = 1.0 - budget_cdf[combo_cost];
                } else {
                    combo_probability = budget_cdf[prev_cost] - budget_cdf[combo_cost];
                }
                prev_cost = combo_cost;

                if (combo_probability == 0) {
                    goto next_combination;
                }

                result_total_probabilities[perk_pair] += probability * combo_probability;
                no_effect_chance -= probability * combo_probability;
            }
        }
        next_combination:
        continue;
    }

    probability_t normalisation_divisor = 1.0 - no_effect_chance;
    if (include_no_effect) {
        result_total_probabilities[{{Perk::no_effect, 0},
                                    {Perk::no_effect, 0}}] += no_effect_chance;
        normalisation_divisor = 1.0;
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
