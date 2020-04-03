//
// Created by Alexander Logan on 03/04/2020.
//

#ifndef RSPERKS_GIZMO_H
#define RSPERKS_GIZMO_H

#include <array>

namespace rs {
    enum EquipmentType {
        WEAPON, TOOL, ARMOUR
    };
    enum GizmoType {
        STANDARD, ANCIENT
    };

    size_t slotsForType(GizmoType type) {
        switch (type) {
            case STANDARD:
                return 5;
            case ANCIENT:
                return 9;
        }
    }

    class Gizmo {
    public:
        Gizmo() = delete;

//        Gizmo(EquipmentType equipmentType, std::vector<uint8_t> ids);

//        Gizmo(EquipmentType equipmentType, std::array<uint8_t, 5> ids);

//        Gizmo(EquipmentType equipmentType, std::array<uint8_t, 9> ids);

        Gizmo(EquipmentType equipmentType, GizmoType gizmoType, std::vector<uint8_t> ids);

    private:
        std::array<uint8_t, 9> components;
    };
}

#endif //RSPERKS_GIZMO_H
