//
// Created by Alexander Logan on 03/04/2020.
//

#include "Gizmo.h"
#include "RSSort.h"
#include <bitset>
#include <iomanip>
#include <cassert>

GizmoResult::GizmoResult(GeneratedPerk first, GeneratedPerk second) : first(first), second(second) {
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
    this->insertion_order_ = perkInsertionOrder();
}

EquipmentType Gizmo::equipmentType() const {
    return this->equipment_type_;
}

GizmoType Gizmo::type() const {
    return this->gizmo_type_;
}

const std::array<Component, 9> &Gizmo::components() const {
    return this->components_;
}

size_t Gizmo::cost() const {
    return std::accumulate(begin(), end(), 0, [](size_t total_cost, const Component &comp) {
        return total_cost + comp.cost();
    });
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
                                                          const GizmoResult &target,
                                                          bool exact_target) const {
    return gizmoResultProbabilities(invention_level, false, target, exact_target);
}

std::vector<Perk> Gizmo::perkInsertionOrder() const {
    std::bitset<std::numeric_limits<perk_id_t>::max()> perk_set;
    std::vector<Perk> insertion_order;
    std::for_each(begin(), end(), [&](const Component &comp) {
        auto &component_perks = comp.perkContributions(this->equipment_type_);
        std::for_each(component_perks.begin(), component_perks.end(), [&](const PerkContribution &contrib) {
            if (!perk_set.test(contrib.perk.id)) {
                insertion_order.push_back(Perk::get(contrib.perk.id));
                perk_set.set(contrib.perk.id);
            }
        });
    });
    return insertion_order;
}

std::vector<CDF> Gizmo::perkRollCdf() const {
    std::array<int, std::numeric_limits<perk_id_t>::max()> bases{};
    std::array<std::vector<int>, std::numeric_limits<perk_id_t>::max()> rolls{};
    std::for_each(begin(), end(), [&](const Component &comp) {
        auto &component_perks = comp.perkContributions(this->equipment_type_);
        std::for_each(component_perks.begin(), component_perks.end(), [&](const PerkContribution &contrib) {
            bases[contrib.perk.id] += (this->gizmo_type_ == ANCIENT && !comp.ancient()) ?
                                      0.8 * contrib.base : contrib.base;
            rolls[contrib.perk.id].push_back((this->gizmo_type_ == ANCIENT && !comp.ancient()) ?
                                             0.8 * contrib.roll : contrib.roll);
        });
    });

    std::vector<CDF> cdfs;
    cdfs.reserve(insertion_order_.size());

    for (Perk perk : insertion_order_) {
        CDF base_cdf = CDF(bases[perk.id], 0);
        CDF perk_cdf = Cdf(rolls[perk.id]);
        base_cdf.reserve(base_cdf.size() + perk_cdf.size());
        base_cdf.insert(base_cdf.end(), perk_cdf.begin(), perk_cdf.end());
        cdfs.emplace_back(std::move(base_cdf));
    }

    return cdfs;
}

std::vector<std::vector<std::pair<rank_t, probability_t>>> Gizmo::perkRankProbabilities() const {
    std::vector<std::vector<std::pair<rank_t, probability_t>>> rank_probabilities;
    std::vector<Perk> insertion_order = this->insertion_order_;
    std::vector<CDF> perk_contrib_cdf = perkRollCdf();

    for (size_t i = 0; i < insertion_order.size(); ++i) {
        Perk perk = insertion_order[i];
        const CDF &perk_cdf = perk_contrib_cdf[i];
        const rank_list_t &ranks = perk.ranks();
        std::vector<std::pair<rank_t, probability_t>> perk_rank_probabilities;

        // Loop backwards through ranks.
        for (size_t rank_i = perk.max_rank; rank_i > 0; --rank_i) {
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
            if (rank_i == perk.max_rank ||
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
        if (ranks[1].threshold >= perk_cdf.size() || perk_cdf[ranks[1].threshold] > 0) {
            if (ranks[1].threshold >= perk_cdf.size()) {
                perk_rank_probabilities.emplace_back(0, 1.0);
            } else {
                perk_rank_probabilities.emplace_back(0, perk_cdf[ranks[1].threshold - 1]);
            }
        }

        assert(perk_rank_probabilities.size() != 0);
        rank_probabilities.emplace_back(perk_rank_probabilities);
    }

    return rank_probabilities;
}

std::vector<std::pair<std::vector<GeneratedPerk>, probability_t>> Gizmo::perkCombinationProbabilities() const {
    std::vector<std::vector<std::pair<rank_t, probability_t>>> perk_rank_probabilities = perkRankProbabilities();

    std::vector<std::pair<std::vector<GeneratedPerk>, probability_t>> perk_combinations;
    perk_combinations.reserve(1024);

    GeneratedPerk no_effect_result = {Perk::no_effect, 0};

    std::vector<size_t> indices(perk_rank_probabilities.size(), 0);
    while (indices[0] < perk_rank_probabilities[0].size()) {
        probability_t combined_prob = 1.0;
        size_t i = 0;
        std::vector<GeneratedPerk> combination;
        combination.reserve(insertion_order_.size() + 1);
        combination.emplace_back(no_effect_result);
        for (i = 0; i < insertion_order_.size(); ++i) {
            rank_t rank = perk_rank_probabilities[i][indices[i]].first;
            probability_t rank_probability = perk_rank_probabilities[i][indices[i]].second;
            combined_prob *= rank_probability;
            combination.emplace_back(insertion_order_[i], rank);
        }
        rs::safeQuicksort(1, combination.size() - 1, combination,
                          [](const GeneratedPerk &a) -> int { return static_cast<int>(a.cost); });
        perk_combinations.emplace_back(std::move(combination), combined_prob);

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

[[maybe_unused]] std::vector<std::pair<std::vector<GeneratedPerk>, probability_t>>
Gizmo::perkCombinationProbabilities(const GizmoResult &target, bool exact) const {
    std::vector<std::vector<std::pair<rank_t, probability_t>>> perk_rank_probabilities = perkRankProbabilities();

    std::vector<std::pair<std::vector<GeneratedPerk>, probability_t>> perk_combinations;
    perk_combinations.reserve(1024);

    std::vector<size_t> indices(perk_rank_probabilities.size());
    while (indices[0] < perk_rank_probabilities[0].size()) {
        probability_t combined_prob = 1.0;
        size_t i = 0;
        std::vector<GeneratedPerk> combination;
        combination.reserve(insertion_order_.size());
        for (i = 0; i < insertion_order_.size(); ++i) {
            rank_t rank = perk_rank_probabilities[i][indices[i]].first;

            if (insertion_order_[i] == target.first.perk) {
                if ((exact && rank != target.first.rank) || rank < target.first.rank) {
                    goto skip;
                }
            }
            if (insertion_order_[i] == target.second.perk) {
                if ((exact && rank != target.second.rank) || rank < target.second.rank) {
                    goto skip;
                }
            }

            probability_t rank_probability = perk_rank_probabilities[i][indices[i]].second;
            combined_prob *= rank_probability;
            combination.emplace_back(insertion_order_[i], rank);
        }
        perk_combinations.emplace_back(std::move(combination), combined_prob);

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
    auto perk_combination_probabilities = perkCombinationProbabilities();
    CDF budget_cdf = inventionBudgetCdf(invention_level, this->gizmo_type_ == ANCIENT);
    GeneratedPerk no_effect_result = {Perk::no_effect, 0};

    bool check_target = target.first.perk.id != no_effect_id;

    probability_t probability_sum = 0.0;

    // For each combination, sort it using modified quicksort and then calculate probabilities of the combination.
    for (const auto &combination : perk_combination_probabilities) {
        const std::vector<GeneratedPerk> &perks = combination.first;
        probability_t probability = combination.second;

        size_t prev_cost = budget_cdf.size() - 1;
        // Loop backwards through the sorted combinations to calculate probability of each occurring.
        for (size_t i = perks.size() - 1; i < perks.size(); --i) {
            if (perks[i].rank == 0) {
                continue;
            }

            for (size_t j = i - 1; j < i; --j) {
                size_t combo_cost = perks[i].cost + perks[j].cost;

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

                GizmoResult perk_pair = {perks[i], perks[j]};

                // If either perk was a two-slot, set the other to nothing.
                if (perk_pair.first.perk.twoSlot()) {
                    perk_pair.second = no_effect_result;
                }
                if (perk_pair.second.perk.twoSlot()) {
                    perk_pair.first = perk_pair.second;
                    perk_pair.second = no_effect_result;
                }

                probability_t result_probability = probability * combo_probability;
                if (!check_target || perk_pair == target) {
                    result_total_probabilities[perk_pair] += result_probability;
                }
                probability_sum += result_probability;
            }
            next_combination:
            continue;
        }
    }

    probability_t normalisation_divisor;
    if (include_no_effect && probability_sum < 1.0) {
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

std::ostream &operator<<(std::ostream &strm, const Gizmo &gizmo) {
    strm << gizmo.type() << " " << gizmo.equipmentType() << " Gizmo:" << std::endl;
    strm << std::setw(15) << "Middle: " << gizmo.components()[0] << std::endl;
    strm << std::setw(15) << "Top: " << gizmo.components()[1] << std::endl;
    strm << std::setw(15) << "Left: " << gizmo.components()[2] << std::endl;
    strm << std::setw(15) << "Right: " << gizmo.components()[3] << std::endl;
    strm << std::setw(15) << "Bottom: " << gizmo.components()[4];
    if (gizmo.type() == ANCIENT) {
        strm << std::endl << std::setw(15) << "Top Left: " << gizmo.components()[5] << std::endl;
        strm << std::setw(15) << "Top Right: " << gizmo.components()[6] << std::endl;
        strm << std::setw(15) << "Bottom Left: " << gizmo.components()[7] << std::endl;
        strm << std::setw(15) << "Bottom Right: " << gizmo.components()[8];
    }

    return strm;
}
