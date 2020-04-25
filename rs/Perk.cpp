//
// Created by Alexander Logan on 03/04/2020.
//

#include "Perk.h"
#include <fstream>
#include <sstream>


std::string Perk::name() const {
    return Perk::perk_names_.at(this->id);
}

rank_list_t &Perk::ranks() const {
    return Perk::perk_ranks_[this->id];
}

bool Perk::twoSlot() const {
    return Perk::perk_is_two_slot_[this->id];
}

Rank Perk::rank(rank_t rank) const {
    return ranks().at(rank);
}

bool Perk::operator==(const Perk &other) const {
    return this->id == other.id;
}

std::vector<Perk> &Perk::all() {
    return all_;
}

Perk Perk::get(perk_id_t id) {
    return Perk::perks_by_id_[id];
}

Perk Perk::get(std::string name) {
    return Perk::perks_by_name_.at(name);
}

size_t Perk::registerPerks(std::string filename) {
    // Register no effect if it hasn't been already.
    if (!perks_by_id_[no_effect_id].id) {
        all_.push_back(no_effect);
        perks_by_id_[no_effect_id] = no_effect;
        perks_by_name_.insert({"No Effect", no_effect});
        perk_names_.insert({no_effect_id, "No Effect"});
        perk_is_two_slot_[no_effect_id] = false;
    }

    std::ifstream perk_data_file;
    perk_data_file.open(filename);
    if (!perk_data_file) {
        std::cerr << "[Error] Could not open data file to register perks: " << filename << std::endl;
        exit(1);
    }

    std::string line;
    // File format is ID,Name,Rank,Cost,Threshold,Ancient
    while (std::getline(perk_data_file, line)) {
        std::stringstream ls(line);
        std::string token;

        // Get ID
        std::getline(ls, token, ',');
        perk_id_t perk_id = std::stoi(token);

        // Get Name
        std::getline(ls, token, ',');
        std::string perk_name(std::move(token));

        // Get Rank
        std::getline(ls, token, ',');
        rank_t perk_rank = std::stoi(token);

        // Get Cost
        std::getline(ls, token, ',');
        rank_cost_t perk_cost = std::stoi(token);

        // Get Threshold
        std::getline(ls, token, ',');
        rank_threshold_t perk_threshold = std::stoi(token);

        // Get Ancient flag
        std::getline(ls, token, ',');
        bool ancient = std::stoi(token) != 0;

        // Create objects and add to relevant stores.
        if (!perks_by_id_[perk_id].id) {
            // Not encountered this perk before.
            Perk new_perk = {perk_id, perk_rank};
            all_.push_back(new_perk);
            perks_by_id_[perk_id] = new_perk;
            perks_by_name_.insert({perk_name, new_perk});
            perk_names_.insert({perk_id, perk_name});
            perk_is_two_slot_[perk_id] = (perk_name == "Enhanced Devoted" || perk_name == "Enhanced Efficient");
            perk_ranks_[perk_id][0] = {0, 0, 0, false};
        }

        // Add rank information.
        perk_ranks_[perk_id][perk_rank] = {perk_rank, perk_cost, perk_threshold, ancient};
        rank_t max_rank = std::max(perks_by_id_[perk_id].max_rank, perk_rank);
        perks_by_id_[perk_id].max_rank = max_rank;
        perks_by_name_[perk_name].max_rank = max_rank;
    }

    return 0;
}

std::vector<Perk> Perk::all_;

std::unordered_map<perk_id_t, std::string> Perk::perk_names_;
std::array<rank_list_t, std::numeric_limits<perk_id_t>::max()> Perk::perk_ranks_;
std::array<bool, std::numeric_limits<perk_id_t>::max()> Perk::perk_is_two_slot_;

std::array<Perk, std::numeric_limits<perk_id_t>::max()> Perk::perks_by_id_;
std::unordered_map<std::string, Perk> Perk::perks_by_name_;

std::ostream &operator<<(std::ostream &strm, const Perk &perk) {
    return strm << perk.name();
}

std::ostream &operator<<(std::ostream &strm, const GeneratedPerk &generated_perk) {
    strm << generated_perk.perk;
    if (generated_perk.perk.max_rank > 1) {
        strm << " " << unsigned(generated_perk.rank);
    }
    return strm;
}

GeneratedPerk::GeneratedPerk(Perk p, rank_t r) : perk(p), rank(r) {
    this->cost = getCost();
}

rank_cost_t GeneratedPerk::getCost() const {
    return this->perk.rank(this->rank).cost;
}
