//
// Created by Alexander Logan on 03/04/2020.
//

#ifndef RSPERKS_PERK_H
#define RSPERKS_PERK_H


#include "InventionTypes.h"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>


struct Rank {
    rank_t rank;
    rank_cost_t cost;
    rank_threshold_t threshold;
    bool ancient;
};

struct Perk {
    perk_id_t id;

    [[nodiscard]] std::string name() const;

    [[nodiscard]] std::vector<Rank> ranks() const;

    [[nodiscard]] bool twoSlot() const;

    [[nodiscard]] Rank rank(rank_t rank) const;

    [[nodiscard]] static Perk get(perk_id_t id);

    [[nodiscard]] static Perk get(std::string name);

    [[nodiscard]] static std::vector<Rank> ranks(perk_id_t perk_id);

    static const Perk no_effect;

    static size_t registerPerks(std::string filename);

private:
    static std::unordered_map<perk_id_t, std::string> perk_names_;
    static std::unordered_map<perk_id_t, std::vector<Rank>> perk_ranks_;
    static std::unordered_map<perk_id_t, bool> perk_is_two_slot_;

    static std::unordered_map<perk_id_t, Perk> perks_by_id_;
    static std::unordered_map<std::string, Perk> perks_by_name_;
};

std::ostream &operator<<(std::ostream &strm, const Perk &perk);

constexpr perk_id_t no_effect_id = 0;
inline const Perk Perk::no_effect = {no_effect_id};

struct GeneratedPerk {
    Perk perk;
    rank_t rank;

    [[nodiscard]] rank_cost_t cost() const;
};

std::ostream &operator<<(std::ostream &strm, const GeneratedPerk &generated_perk);


#endif //RSPERKS_PERK_H
