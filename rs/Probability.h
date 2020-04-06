//
// Created by Alexander Logan on 03/04/2020.
//

#ifndef RSPERKS_PROBABILITY_H
#define RSPERKS_PROBABILITY_H

#include "InventionTypes.h"
#include <vector>
#include <array>
#include <numeric>
#include <algorithm>

typedef std::vector<probability_t> PDF;
typedef std::vector<probability_t> CDF;

template<typename T>
inline std::vector<T> convolve(const std::vector<T> &a, const std::vector<T> &b) {
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
inline PDF Pdf(const std::vector<T> &rolls) {
    PDF pdf(static_cast<probability_t>(rolls[0]), 1.0 / static_cast<probability_t>(rolls[0]));
    std::for_each(rolls.begin() + 1, rolls.end(), [&pdf](T roll) {
        PDF other(static_cast<probability_t>(roll), 1.0 / static_cast<probability_t>(roll));
        pdf = convolve(pdf, other);
    });
    return pdf;
}

template<typename T>
inline CDF Cdf(const std::vector<T> &rolls) {
    PDF pdf = Pdf(rolls);
    CDF cdf(pdf.size());
    std::partial_sum(pdf.begin(), pdf.end(), cdf.begin());
    return cdf;
}

inline std::array<bool, std::numeric_limits<level_t>::max()> __inv_cache_built;
inline std::array<CDF, std::numeric_limits<level_t>::max()> __inv_cache_cdf;

inline CDF inventionBudgetCdf(level_t invention_level, bool ancient) {
    if (__inv_cache_built[invention_level]) {
        return __inv_cache_cdf[invention_level];
    }

    level_t inv_budget_roll_max = invention_level / 2 + 20;
    PDF budget_pdf = Pdf(std::vector<level_t>(ancient ? 6 : 5, inv_budget_roll_max));
    for (size_t i = 0; i < invention_level; ++i) {
        budget_pdf[invention_level] += budget_pdf[i];
        budget_pdf[i] = 0;
    }
    CDF budget_cdf(budget_pdf.size());
    std::partial_sum(budget_pdf.begin(), budget_pdf.end(), budget_cdf.begin());

    __inv_cache_cdf[invention_level] = budget_cdf;
    __inv_cache_built[invention_level] = true;
    return budget_cdf;
}

#endif //RSPERKS_PROBABILITY_H
