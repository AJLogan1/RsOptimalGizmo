#ifndef RSPERKS_INVENTIONCALC_H
#define RSPERKS_INVENTIONCALC_H

#include <numeric>
#include <unordered_map>
#include "InventionData.h"

namespace {
    template<typename T>
    inline std::vector<T> convolve(const std::vector<T> &a, const std::vector<T> &b);

    template<typename T>
    std::vector<double> calculatePDF(const std::vector<T> &rolls);

    template<typename T>
    std::vector<double> calculateCDF(const std::vector<T> &rolls);

    /**
     * Returns a pair of <Insertion Order, <Bases, Rolls>>.
     */
    std::pair<std::vector<uint8_t>, std::pair<std::map<uint8_t, uint8_t>, std::map<uint8_t, std::vector<uint8_t>>>>
    possiblePerksGenerated(const rs::Gizmo &gizmo);

    std::unordered_map<uint8_t, std::vector<std::pair<uint8_t, double>>>
    perkRankProbabilities(const std::vector<uint8_t> &insertion_order,
                          const std::map<uint8_t, uint8_t> &bases,
                          const std::map<uint8_t, std::vector<uint8_t>> &rolls);

    std::vector<std::pair<double, std::vector<rs::GizmoPerk>>>
    perkCombinations(
            std::unordered_map<unsigned char, std::vector<std::pair<unsigned char, double>>> &perk_rank_probabilities,
            const std::vector<uint8_t> &insertion_order);

    std::vector<double> inventionBudgetCDF(uint8_t invention_level);

    std::unordered_map<std::pair<rs::GizmoPerk, rs::GizmoPerk>, double>
    perkCombinationProbabilities(const std::vector<std::pair<double, std::vector<rs::GizmoPerk>>> &perk_combinations,
                                 const std::vector<double> &budget_cdf);

    void normaliseCombinationProbabilities(
            std::unordered_map<std::pair<rs::GizmoPerk, rs::GizmoPerk>, double> &perk_combo_probabilities);
}

namespace rs {
    GizmoPerkProbabilities
    perkProbabilities(const Gizmo &gizmo, uint8_t invention_level, bool normalise = true);
}

#endif //RSPERKS_INVENTIONCALC_H
