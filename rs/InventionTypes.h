//
// Created by Alexander Logan on 03/04/2020.
//

#ifndef RSPERKS_INVENTIONTYPES_H
#define RSPERKS_INVENTIONTYPES_H

#include <ostream>
#include <cstddef>
#include <cstdint>
#include <iostream>

enum EquipmentType {
    WEAPON = 0,
    TOOL,
    ARMOUR,

    // SIZE will automatically be the number of equipment types.
    SIZE
};

inline std::ostream &operator<<(std::ostream &strm, const EquipmentType &equipment_type) {
    switch (equipment_type) {
        case WEAPON:
            return strm << "Weapon";
        case TOOL:
            return strm << "Tool";
        case ARMOUR:
            return strm << "Armour";
        default:
            return strm << "INVALID";
    }
}

inline EquipmentType stoet(std::string str) {
    if (str == "weapon") {
        return WEAPON;
    }
    if (str == "tool") {
        return TOOL;
    }
    if (str == "armour") {
        return ARMOUR;
    }

    std::cerr << "[Error] Unrecognised equipment type: " << str << std::endl
              << "Hint: Equipment type must be one of 'weapon', 'tool', 'armour'." << std::endl;
    exit(2);
}

enum GizmoType {
    STANDARD, ANCIENT
};

inline std::ostream &operator<<(std::ostream &strm, const GizmoType &gizmo_type) {
    switch (gizmo_type) {
        case STANDARD:
            return strm << "Standard";
        case ANCIENT:
            return strm << "Ancient";
        default:
            return strm << "INVALID";
    }
}

inline size_t slotsForType(GizmoType type) {
    switch (type) {
        case STANDARD:
            return 5;
        case ANCIENT:
            return 9;
    }
    return 0;
}

// Use doubles for probabilities.
typedef double probability_t;

typedef uint8_t level_t;

typedef uint8_t rank_t;
typedef uint8_t rank_cost_t;
typedef uint16_t rank_threshold_t;

typedef uint8_t perk_id_t;

typedef uint8_t contribution_base_t;
typedef uint8_t contribution_roll_t;

typedef uint8_t component_id_t;

#endif //RSPERKS_INVENTIONTYPES_H
