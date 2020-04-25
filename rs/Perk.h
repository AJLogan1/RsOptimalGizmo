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

typedef std::array<Rank, 7> rank_list_t;

struct Perk {
    perk_id_t id;
    rank_t max_rank;

    [[nodiscard]] std::string name() const;

    [[nodiscard]] rank_list_t &ranks() const;

    [[nodiscard]] bool twoSlot() const;

    [[nodiscard]] Rank rank(rank_t rank) const;

    bool operator==(const Perk &other) const;

    [[nodiscard]] static std::vector<Perk> &all();

    [[nodiscard]] static Perk get(perk_id_t id);

    [[nodiscard]] static Perk get(std::string name);

    static const Perk no_effect;

    static size_t registerPerks(std::string filename);

private:
    static std::vector<Perk> all_;

    static std::unordered_map<perk_id_t, std::string> perk_names_;
    static std::array<rank_list_t, std::numeric_limits<perk_id_t>::max()> perk_ranks_;
    static std::array<bool, std::numeric_limits<perk_id_t>::max()> perk_is_two_slot_;

    static std::array<Perk, std::numeric_limits<perk_id_t>::max()> perks_by_id_;
    static std::unordered_map<std::string, Perk> perks_by_name_;
};

std::ostream &operator<<(std::ostream &strm, const Perk &perk);

constexpr perk_id_t no_effect_id = 0;
inline const Perk Perk::no_effect = {no_effect_id, 0};

struct GeneratedPerk {
    Perk perk;
    rank_t rank;
    rank_cost_t cost;

    GeneratedPerk(Perk p, rank_t r);

private:
    [[nodiscard]] rank_cost_t getCost() const;
};

std::ostream &operator<<(std::ostream &strm, const GeneratedPerk &generated_perk);


#endif //RSPERKS_PERK_H
