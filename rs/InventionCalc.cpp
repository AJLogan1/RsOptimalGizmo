#include "InventionCalc.h"
#include "RSSort.h"

#include <mutex>

namespace {
    template<typename T>
    std::vector<T> convolve(const std::vector<T> &a, const std::vector<T> &b) {
        size_t a_len = a.size();
        size_t b_len = b.size();
        size_t result_size = a_len + b_len - 1;
        std::vector<T> out(result_size, T());
        for (size_t i = 0; i < result_size; ++i) {
            size_t j_min = (i > b_len - 1) ? i - b_len + 1 : 0;
            size_t j_max = (i < a_len - 1) ? i : a_len - 1;
            for (size_t j = j_min; j <= j_max; ++j) {
                out[i] += a[j] * b[i - j];
            }
        }
        return out;
    }

    template<typename T>
    std::vector<double> calculatePDF(const std::vector<T> &rolls) {
        std::vector<double> probabilities(rolls[0], 1.0 / rolls[0]);
        for (size_t i = 0; i < rolls[0]; ++i) {
            probabilities[i] = 1.0 / rolls[0];
        }
        for (size_t roll = 1; roll < rolls.size(); ++roll) {
            std::vector<double> other(rolls[roll], 1.0 / rolls[roll]);
            probabilities = convolve(probabilities, other);
        }

        return probabilities;
    }

    template<typename T>
    std::vector<double> calculateCDF(const std::vector<T> &rolls) {
        auto pdf = calculatePDF(rolls);
        std::vector<double> cdf(pdf.size());
        std::partial_sum(pdf.begin(), pdf.end(), cdf.begin());
        return cdf;
    }

    std::pair<std::vector<uint8_t>, std::pair<std::map<uint8_t, uint8_t>, std::map<uint8_t, std::vector<uint8_t>>>>
    possiblePerksGenerated(const rs::Gizmo &gizmo) {
        std::map<uint8_t, uint8_t> bases;
        std::map<uint8_t, std::vector<uint8_t>> rolls;
        std::vector<uint8_t> insertion_order;
        const rs::InventionComponent *const *middle_comp = &gizmo.middle;
        const rs::InventionComponent *const *bottom_comp = &gizmo.bottom;
        const rs::InventionComponent *const *current_comp = middle_comp;
        for (; current_comp <= bottom_comp; ++current_comp) {
            const rs::InventionComponent *comp = *current_comp;

            switch (gizmo.type) {
                case rs::WEAPON:
                    for (const auto &perk : comp->weapon_perks) {
                        if (std::find(insertion_order.begin(), insertion_order.end(), perk.perk->id) ==
                            insertion_order.end()) {
                            insertion_order.push_back(perk.perk->id);
                        }
                        bases[perk.perk->id] += perk.base;
                        rolls[perk.perk->id].push_back(perk.roll);
                    }
                    break;
                case rs::TOOL:
                    for (const auto &perk : comp->tool_perks) {
                        if (std::find(insertion_order.begin(), insertion_order.end(), perk.perk->id) ==
                            insertion_order.end()) {
                            insertion_order.push_back(perk.perk->id);
                        }
                        bases[perk.perk->id] += perk.base;
                        rolls[perk.perk->id].push_back(perk.roll);
                    }
                    break;
                case rs::ARMOUR:
                    for (const auto &perk : comp->armour_perks) {
                        if (std::find(insertion_order.begin(), insertion_order.end(), perk.perk->id) ==
                            insertion_order.end()) {
                            insertion_order.push_back(perk.perk->id);
                        }
                        bases[perk.perk->id] += perk.base;
                        rolls[perk.perk->id].push_back(perk.roll);
                    }
                    break;
            }
        }

        return std::make_pair(insertion_order, std::make_pair(bases, rolls));
    }

    std::unordered_map<uint8_t, std::vector<std::pair<uint8_t, double>>>
    perkRankProbabilities(const std::vector<uint8_t> &insertion_order, const std::map<uint8_t, uint8_t> &bases,
                          const std::map<uint8_t, std::vector<uint8_t>> &rolls) {
        std::unordered_map<uint8_t, std::vector<std::pair<uint8_t, double>>> perk_rank_probabilities;

        for (auto perk_id : insertion_order) {
            auto ranks = rs::perks::id_perks[perk_id]->ranks;
            auto base = bases.at(perk_id);
            const auto &roll = rolls.at(perk_id);

            // Calculate CDF for the perk rolls.
            auto cdf = calculateCDF(roll);

            std::vector<int> roll_thresholds(ranks.size());
            std::transform(ranks.begin(), ranks.end(),
                           roll_thresholds.begin(),
                           [&base](auto x) { return static_cast<int>(x.threshold) - base; });
            for (size_t i = roll_thresholds.size() - 1; i < roll_thresholds.size(); --i) {
                auto threshold = roll_thresholds[i];
                if (threshold > static_cast<int>(cdf.size() - 1)) {
                    // Cannot generate this rank.
                    continue;
                }

                threshold = threshold < 0 ? 0 : threshold;

                // Now calculate the probability the roll will fall between this threshold and the rank above's
                // threshold.
                double rank_prob;
                if (i == roll_thresholds.size() - 1 || roll_thresholds[i + 1] > cdf.size() - 1) {
                    if (threshold > 0) {
                        rank_prob = 1.0 - cdf[threshold - 1];
                    } else {
                        rank_prob = 1.0;
                    }
                } else {
                    if (threshold > 0) {
                        rank_prob = cdf[roll_thresholds[i + 1] - 1] - cdf[threshold - 1];
                    } else {
                        rank_prob = cdf[roll_thresholds[i + 1] - 1];
                    }
                }

                // Add perk to vector.
                if (rank_prob > 0) {
                    perk_rank_probabilities[perk_id].push_back(std::make_pair(ranks[i].rank, rank_prob));
                }

                // If threshold is 0, then we know we can't generate any perks of lower rank.
                if (threshold == 0) {
                    break;
                }

            }

            // If the minimum threshold is greater than zero, there is a chance we generate a zero-rank perk.
            // Add that probability now.
            if (roll_thresholds[0] > 0) {
                if (roll_thresholds[0] >= cdf.size()) {
                    roll_thresholds[0] = cdf.size() - 1;
                }
                // Add here.
                perk_rank_probabilities[perk_id].push_back(std::make_pair(0, cdf[roll_thresholds[0] - 1]));
            }
        }

        return perk_rank_probabilities;
    }

    std::vector<std::pair<double, std::vector<rs::GizmoPerk>>> perkCombinations(
            std::unordered_map<unsigned char, std::vector<std::pair<unsigned char, double>>> &perk_rank_probabilities,
            const std::vector<uint8_t> &insertion_order) {
        std::vector<std::pair<double, std::vector<rs::GizmoPerk>>> perk_combinations;
        perk_combinations.reserve(512);
        std::vector<size_t> indices(perk_rank_probabilities.size());

        while (indices[0] < perk_rank_probabilities[insertion_order[0]].size()) {
            double combined_prob = 1.0;
            size_t i;
            std::vector<rs::GizmoPerk> combination;
            for (i = 0; i < insertion_order.size(); ++i) {
                combined_prob *= perk_rank_probabilities[insertion_order[i]][indices[i]].second;
                combination.push_back({rs::perks::id_perks[insertion_order[i]].get(),
                                       perk_rank_probabilities[insertion_order[i]][indices[i]].first});
            }
            perk_combinations.emplace_back(combined_prob, combination);

            // Increment indices.
            for (i = indices.size() - 1; i < indices.size(); --i) {
                indices[i]++;
                if (indices[i] == perk_rank_probabilities[insertion_order[i]].size()) {
                    if (i == 0) {
                        break;
                    }
                    indices[i] = 0;
                    continue;
                } else {
                    break;
                }
            }
        }

        return perk_combinations;
    }

    std::mutex inv_budget_cache_lock;
    std::unordered_map<uint8_t, std::vector<double>> inv_budget_cdf_cache;

    std::vector<double> inventionBudgetCDF(uint8_t invention_level) {
        {
            std::scoped_lock cache_lock(inv_budget_cache_lock);
            if (inv_budget_cdf_cache.count(invention_level)) {
                return inv_budget_cdf_cache[invention_level];
            }
        }

        uint8_t inv_budget_roll_max = invention_level / 2 + 20;
        auto budget_pdf = calculatePDF(std::vector<uint8_t>(5, inv_budget_roll_max));
        for (size_t i = 0; i < invention_level; ++i) {
            budget_pdf[invention_level] += budget_pdf[i];
            budget_pdf[i] = 0;
        }
        std::vector<double> budget_cdf(budget_pdf.size());
        std::partial_sum(budget_pdf.begin(), budget_pdf.end(), budget_cdf.begin());
        {
            std::scoped_lock cache_lock(inv_budget_cache_lock);
            inv_budget_cdf_cache[invention_level] = budget_cdf;
        }
        return budget_cdf;
    }

    std::unordered_map<std::pair<rs::GizmoPerk, rs::GizmoPerk>, double>
    perkCombinationProbabilities(const std::vector<std::pair<double, std::vector<rs::GizmoPerk>>> &perk_combinations,
                                 const std::vector<double> &budget_cdf) {
        std::unordered_map<std::pair<rs::GizmoPerk, rs::GizmoPerk>, double> perk_combo_probabilities;

        // For each combination, sort it using modified quicksort and then calculate probabilities of the combinations.
        for (const auto &combination : perk_combinations) {
            auto probability = combination.first;
            auto perks = combination.second;

            quicksort(perks.begin(), perks.end());
            perks.insert(perks.begin(), rs::GizmoPerk{rs::perks::named_perks["No effect"].get(), 0});

            bool continue_loop = true;
            unsigned int prev_cost = 0;
            // Loop backwards through the sorted combinations to calculate probability of each occurring.
            for (size_t i = perks.size() - 1; i < perks.size(); --i) {
                if (!continue_loop) {
                    break;
                }

                if (perks[i].rank == 0) {
                    continue;
                }

                for (size_t j = i - 1; j < i; --j) {
                    // Calculate the sum cost of the combination.
                    auto combo_cost = perks[i] + perks[j];

                    if (combo_cost >= budget_cdf.size()) {
                        // Cannot generate this combination.
                        continue;
                    }

                    // Cannot possibly generate a combo which costs more than a previously generated one.
                    if (prev_cost != 0 && combo_cost >= prev_cost) {
                        continue;
                    }

                    std::pair<rs::GizmoPerk, rs::GizmoPerk> perk_pair = rs::makePerkPair(perks[i], perks[j]);

                    double combo_prob;
                    if (prev_cost == 0) {
                        prev_cost = combo_cost;
                        // P(inv_budget <= combo_cost)
                        combo_prob = 1.0 - budget_cdf[combo_cost];
                    } else {
                        combo_prob = budget_cdf[prev_cost] - budget_cdf[combo_cost];
                        prev_cost = combo_cost;
                    }

                    if (combo_prob == 0) {
                        continue_loop = false;
                        break;
                    }

                    perk_combo_probabilities[perk_pair] += combo_prob * probability;
                }
            }
        }

        return perk_combo_probabilities;
    }

    void normaliseCombinationProbabilities(
            std::unordered_map<std::pair<rs::GizmoPerk, rs::GizmoPerk>, double> &perk_combo_probabilities) {
        double sum_prob = std::accumulate(std::begin(perk_combo_probabilities),
                                          std::end(perk_combo_probabilities),
                                          0.0,
                                          [](double a, const auto &b) {
                                              return a + b.second;
                                          });
        std::for_each(std::begin(perk_combo_probabilities),
                      std::end(perk_combo_probabilities),
                      [sum_prob](auto &a) {
                          a.second = a.second / sum_prob;
                      });
    }
}

namespace rs {

    GizmoPerkProbabilities perkProbabilities(const Gizmo &gizmo, uint8_t invention_level, bool normalise) {
        // Calculate possible rolls for perks based on the components used in the Gizmo.
        auto possible_perks_result = possiblePerksGenerated(gizmo);
        std::vector<std::pair<double, std::vector<rs::GizmoPerk>>> perk_combinations;
        auto insertion_order = possible_perks_result.first;
        auto bases = possible_perks_result.second.first;
        auto rolls = possible_perks_result.second.second;

        // Check if any perks exist which can be generated.
        if (insertion_order.empty()) {
            return rs::GizmoPerkProbabilities{gizmo, invention_level, {}};
        }

        // Calculate the probabilities each rank occurs for each possible perk.
        std::unordered_map<unsigned char, std::vector<std::pair<uint8_t, double>>> perk_rank_probabilities;
        perk_rank_probabilities = perkRankProbabilities(insertion_order, bases, rolls);

        // Calculate possible perk-rank combinations, and their associated probabilities.
        // Vector is of <combination probability, perk-rank combination>.
        //std::vector<std::pair<double, std::vector<rs::GizmoPerk>>> perk_combinations;
        perk_combinations = perkCombinations(perk_rank_probabilities, insertion_order);

        // Calculate CDF of invention budget.
        auto budget_cdf = inventionBudgetCDF(invention_level);

        // Based on invention budget, calculate the probability of perk combinations occurring.
        // Map is from <perk 1, perk 2> -> probability.
        std::unordered_map<std::pair<GizmoPerk, GizmoPerk>, double> perk_combo_probabilities;
        perk_combo_probabilities = perkCombinationProbabilities(perk_combinations, budget_cdf);

        // Normalise probabilities (removes no effect chance).
        if (normalise) {
            normaliseCombinationProbabilities(perk_combo_probabilities);
        }

        std::vector<PerkProbability> output_probabilities;
        output_probabilities.reserve(perk_combo_probabilities.size());
        for (const auto &pair : perk_combo_probabilities) {
            output_probabilities.emplace_back(PerkProbability{pair.first.first,
                                                              pair.first.second,
                                                              pair.second});
        }

        return rs::GizmoPerkProbabilities{gizmo, invention_level, output_probabilities};
    }
}
