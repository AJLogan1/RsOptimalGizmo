//
// Created by Alexander Logan on 03/04/2020.
//

#ifndef RSPERKS_COMPONENT_H
#define RSPERKS_COMPONENT_H


#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include "InventionTypes.h"
#include "Perk.h"


struct PerkContribution {
    Perk perk;
    contribution_base_t base;
    contribution_roll_t roll;

    [[nodiscard]] int totalPotentialContribution() const {
        return base + roll;
    }
};

struct Component {
    component_id_t id;

    [[nodiscard]] std::string name() const;

    [[nodiscard]] std::vector<PerkContribution> perkContributions(EquipmentType type) const;

    [[nodiscard]] bool ancient() const;

    [[nodiscard]] int totalPotentialContribution(EquipmentType equipment, perk_id_t perk) const;

    [[nodiscard]] static Component get(component_id_t comp_id);

    [[nodiscard]] static Component get(std::string name);

    static const Component empty;

    static size_t registerComponents(std::string filename);

    static size_t registerCosts(std::string filename);

private:
    static std::unordered_map<component_id_t, std::string> component_names_;
    static std::array<std::unordered_map<component_id_t, std::vector<PerkContribution>>, EquipmentType::SIZE>
            component_perk_contributions_;
    static std::unordered_map<component_id_t, size_t> component_costs_;
    static std::unordered_map<component_id_t, bool> component_ancient_status_;

    static std::unordered_map<component_id_t, Component> components_by_id_;
    static std::unordered_map<std::string, Component> components_by_name_;
};

std::ostream &operator<<(std::ostream &strm, const Component &component);

constexpr component_id_t empty_component_id = 255;
inline const Component Component::empty = {empty_component_id};


#endif //RSPERKS_COMPONENT_H
