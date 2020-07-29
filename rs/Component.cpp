//
// Created by Alexander Logan on 03/04/2020.
//

#include "Component.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>


std::string Component::name() const {
    return Component::component_names_.at(this->id);
}

std::vector<PerkContribution> &Component::perkContributions(EquipmentType type) const {
    return Component::component_perk_contributions_.at(type).at(this->id);
}

bool Component::ancient() const {
    return Component::component_ancient_status_[this->id];
}

int Component::totalPotentialContribution(EquipmentType equipment, perk_id_t perk) const {
    std::vector<PerkContribution> component_contribution = this->perkContributions(equipment);
    auto found_contrib = std::find_if(component_contribution.begin(),
                                      component_contribution.end(),
                                      [&perk](const PerkContribution &contrib) {
                                          return contrib.perk.id == perk;
                                      });
    return found_contrib != component_contribution.end() ? found_contrib->totalPotentialContribution() : 0;
}

size_t Component::cost() const {
    return component_costs_[this->id];
}

const std::bitset<std::numeric_limits<perk_id_t>::max()> &Component::possiblePerkBitset(EquipmentType equipment) const {
    return possible_perk_bitsets_[equipment].at(this->id);
}

bool Component::operator==(const Component &other) const {
    return this->id == other.id;
}

Component Component::get(component_id_t comp_id) {
    return Component::components_by_id_.at(comp_id);
}

Component Component::get(std::string name) {
    return Component::components_by_name_.at(name);
}

std::vector<Component> &Component::all() {
    return all_;
}

size_t Component::registerComponents(std::string filename) {
    // Register the empty component, if it has not been already.
    if (!components_by_id_[empty_component_id].id) {
        all_.push_back(Component::empty);
        components_by_id_[empty_component_id] = Component::empty;
        components_by_name_.insert({"Empty", Component::empty});
        component_ancient_status_[empty_component_id] = false;
        component_names_.insert({empty_component_id, "Empty"});
        component_perk_contributions_[WEAPON].insert({empty_component_id, {}});
        component_perk_contributions_[TOOL].insert({empty_component_id, {}});
        component_perk_contributions_[ARMOUR].insert({empty_component_id, {}});
        possible_perk_bitsets_[WEAPON].insert({empty_component_id, {}});
        possible_perk_bitsets_[TOOL].insert({empty_component_id, {}});
        possible_perk_bitsets_[ARMOUR].insert({empty_component_id, {}});
        component_costs_[empty_component_id] = 0;
    }

    std::ifstream comp_data_file;
    comp_data_file.open(filename);
    if (!comp_data_file) {
        std::cerr << "[Error] Could not open data file to register components: " << filename << std::endl;
        exit(1);
    }

    std::string line;
    // File format is ID,Name,EquipmentType,PerkName,Base,Roll,Ancient
    while (std::getline(comp_data_file, line)) {
        std::stringstream ls(line);
        std::string token;

        // Get ID
        std::getline(ls, token, ',');
        component_id_t component_id = std::stoi(token);

        // Get Name
        std::getline(ls, token, ',');
        std::string component_name(std::move(token));

        // Get Equipment Type
        std::getline(ls, token, ',');
        EquipmentType perk_equip_type = stoet(std::move(token));

        // Get Perk Name
        std::getline(ls, token, ',');
        Perk possible_perk = Perk::get(std::move(token));

        // Get Base
        std::getline(ls, token, ',');
        contribution_base_t perk_base = std::stoi(token);

        // Get Roll
        std::getline(ls, token, ',');
        contribution_roll_t perk_roll = std::stoi(token);

        // Get Ancient flag
        std::getline(ls, token, ',');
        bool ancient = std::stoi(token) != 0;

        // Create objects and add to relevant stores.
        if (!components_by_id_[component_id].id) {
            // Not encountered this component before.
            Component new_comp = {component_id};
            all_.push_back(new_comp);
            components_by_id_[component_id] = new_comp;
            components_by_name_.insert({component_name, new_comp});
            component_ancient_status_[component_id] = ancient;
            component_names_.insert({component_id, component_name});

            component_perk_contributions_[WEAPON].insert({component_id, {}});
            component_perk_contributions_[TOOL].insert({component_id, {}});
            component_perk_contributions_[ARMOUR].insert({component_id, {}});

            possible_perk_bitsets_[WEAPON].insert({component_id, {}});
            possible_perk_bitsets_[TOOL].insert({component_id, {}});
            possible_perk_bitsets_[ARMOUR].insert({component_id, {}});

            // Note: Cost can be overridden later.
            component_costs_[component_id] = 0;
        }

        // Add perk contribution.
        component_perk_contributions_[perk_equip_type].at(component_id).push_back({possible_perk,
                                                                                   perk_base,
                                                                                   perk_roll});
        // Set bit in possible perk bitsets.
        possible_perk_bitsets_[perk_equip_type].at(component_id).set(possible_perk.id);
    }

    return 0;
}

size_t Component::registerCosts(std::string filename) {
    // This function must always be called AFTER registerComponents.

    std::ifstream comp_cost_file;
    comp_cost_file.open(filename);
    if (!comp_cost_file) {
        std::cerr << "[Error] Could not open data file to register costs: " << filename << std::endl;
        exit(1);
    }

    std::string line;
    // File format is ID,Name,Cost
    while (std::getline(comp_cost_file, line)) {
        std::stringstream ls(line);
        std::string token;

        // Get ID
        std::getline(ls, token, ',');
        component_id_t component_id = std::stoi(token);

        // Get Name
        std::getline(ls, token, ',');
        std::string component_name(std::move(token));

        // Get Cost
        std::getline(ls, token, ',');
        size_t component_cost = std::stoi(token);

        // Insert component cost mapping.
        component_costs_[component_id] = component_cost;
    }

    return 0;
}

std::vector<Component> Component::all_;

std::unordered_map<component_id_t, std::string> Component::component_names_;
std::array<std::unordered_map<component_id_t, std::vector<PerkContribution>>, EquipmentType::SIZE>
        Component::component_perk_contributions_;
std::array<size_t, std::numeric_limits<component_id_t>::max()> Component::component_costs_;
std::array<bool, std::numeric_limits<component_id_t>::max()> Component::component_ancient_status_;
std::array<std::unordered_map<component_id_t, std::bitset<std::numeric_limits<perk_id_t>::max()>>, EquipmentType::SIZE>
        Component::possible_perk_bitsets_;

std::array<Component, std::numeric_limits<component_id_t>::max()> Component::components_by_id_;
std::unordered_map<std::string, Component> Component::components_by_name_;

std::ostream &operator<<(std::ostream &strm, const Component &component) {
    return strm << component.name();
}
