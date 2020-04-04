//
// Created by Alexander Logan on 03/04/2020.
//

#include "Perk.h"
#include <fstream>
#include <sstream>


std::string Perk::name() const {
    return Perk::perk_names_.at(this->id);
}

std::vector<Rank> Perk::ranks() const {
    return Perk::perk_ranks_.at(this->id);
}

bool Perk::twoSlot() const {
    return Perk::perk_is_two_slot_.at(this->id);
}

Rank Perk::rank(rank_t rank) const {
    if (rank == 0) {
        return {0, 0, 0, false};
    }
    return ranks().at(rank - 1);
}

Perk Perk::get(perk_id_t id) {
    return Perk::perks_by_id_.at(id);
}

Perk Perk::get(std::string name) {
    return Perk::perks_by_name_.at(name);
}

std::vector<Rank> Perk::ranks(perk_id_t perk_id) {
    return Perk::get(perk_id).ranks();
}

size_t Perk::registerPerks(std::string filename) {
    // Register no effect if it hasn't been already.
    if (!perks_by_id_.count(no_effect_id)) {
        perks_by_id_.insert({no_effect_id, no_effect});
        perks_by_name_.insert({"No Effect", no_effect});
        perk_names_.insert({no_effect_id, "No Effect"});
        perk_is_two_slot_.insert({no_effect_id, false});
        perk_ranks_.insert({no_effect_id, {}});
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
        if (!perks_by_id_.count(perk_id)) {
            // Not encountered this perk before.
            Perk new_perk = {perk_id};
            perks_by_id_.insert({perk_id, new_perk});
            perks_by_name_.insert({perk_name, new_perk});
            perk_names_.insert({perk_id, perk_name});
            if (perk_name == "Enhanced Devoted" || perk_name == "Enhanced Efficient") {
                perk_is_two_slot_.insert({perk_id, true});
            } else {
                perk_is_two_slot_.insert({perk_id, false});
            }
            perk_ranks_.insert({perk_id, {}});
        }

        // Add rank information.
        perk_ranks_.at(perk_id).push_back({perk_rank, perk_cost, perk_threshold, ancient});
    }

    return 0;
}

std::unordered_map<perk_id_t, std::string> Perk::perk_names_;
std::unordered_map<perk_id_t, std::vector<Rank>> Perk::perk_ranks_;
std::unordered_map<perk_id_t, bool> Perk::perk_is_two_slot_;

std::unordered_map<perk_id_t, Perk> Perk::perks_by_id_;
std::unordered_map<std::string, Perk> Perk::perks_by_name_;

std::ostream &operator<<(std::ostream &strm, const Perk &perk) {
    return strm << perk.name();
}

std::ostream &operator<<(std::ostream &strm, const GeneratedPerk &generated_perk) {
    strm << generated_perk.perk;
    if (generated_perk.perk.ranks().size() > 1) {
        strm << " " << unsigned(generated_perk.rank);
    }
    return strm;
}

rank_cost_t GeneratedPerk::cost() const {
    return this->perk.rank(this->rank).cost;
}
