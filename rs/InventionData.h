#ifndef RSPERKS_INVENTIONDATA_H
#define RSPERKS_INVENTIONDATA_H

#include <memory>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <bitset>

namespace rs {
    struct InventionPerk {
        struct PerkRank {
            const uint8_t rank;
            const uint8_t cost;
            const uint8_t threshold;
        };

        const uint8_t id;
        const std::string name;
        std::vector<InventionPerk::PerkRank> ranks;

        InventionPerk(uint8_t i, std::string n) : id(i), name(std::move(n)) {};

        bool isTwoSlot() const {
            if (name == "Enhanced Devoted" || name == "Enhanced Efficient") {
                return true;
            }

            return false;
        }
    };

    enum GizmoType {
        WEAPON, TOOL, ARMOUR
    };

    struct InventionComponent {
        struct ComponentPerk {
            const InventionPerk *perk;
            const uint8_t base;
            const uint8_t roll;

            [[nodiscard]] uint8_t totalPotentialContribution() const {
                return base + roll;
            }
        };

        const uint8_t id;
        const std::string name;
        std::vector<InventionComponent::ComponentPerk> weapon_perks;
        std::vector<InventionComponent::ComponentPerk> tool_perks;
        std::vector<InventionComponent::ComponentPerk> armour_perks;

        InventionComponent(uint8_t i, std::string n) : id(i), name(std::move(n)) {};

        [[nodiscard]] std::vector<InventionComponent::ComponentPerk> perksForType(GizmoType t) const {
            switch (t) {
                case WEAPON:
                    return weapon_perks;
                case TOOL:
                    return tool_perks;
                case ARMOUR:
                    return armour_perks;
                default:
                    return weapon_perks;
            }
        }

        [[nodiscard]] std::bitset<128> possiblePerksBitset(GizmoType t) {
            auto perks_for_type = perksForType(t);
            std::bitset<128> set;
            std::for_each(perks_for_type.begin(), perks_for_type.end(), [&set] (const auto &a) {
                set.set(a.perk->id);
            });
            return set;
        }

        [[nodiscard]] uint8_t totalPotentialContribution(GizmoType t, InventionPerk* perk) const {
            for (const auto &p : perksForType(t)) {
                if (p.perk->id == perk->id) {
                    return p.totalPotentialContribution();
                }
            }

            return 0;
        }
    };

    struct Gizmo {
        Gizmo(GizmoType gt,
              InventionComponent *m,
              InventionComponent *t,
              InventionComponent *l,
              InventionComponent *r,
              InventionComponent *b) :
                type(gt), middle(m), top(t), left(l), right(r), bottom(b) {
        }

        Gizmo() = default;

        void ensureNormalForm() {
            InventionComponent** indifferent = &middle;
            InventionComponent** back = &bottom;
            std::bitset<128> gen_perks;

            while (indifferent < back) {
                const auto& perks_gen_by_pos = (*indifferent)->perksForType(type);
                bool has_new = false;
                for (const auto& p : perks_gen_by_pos) {
                    if (!gen_perks.test(p.perk->id)) {
                        has_new = true;
                        gen_perks.set(p.perk->id);
                    }
                }
                if (has_new) {
                    indifferent++;
                } else {
                    std::swap(*indifferent, *back);
                    back--;
                }
            }

            if (indifferent < &bottom) {
                // Sort by component ID
                back = &bottom;
                while (indifferent < back) {
                    InventionComponent** val = indifferent;
                    while (val < &bottom) {
                        if ((*val)->id > (*(val + 1))->id) {
                            std::swap(*val, *(val + 1));
                        }
                        val++;
                    }
                    back--;
                }
            }

            // Gizmo should now be in normal form.
        }

        [[nodiscard]] uint32_t backFourEncoding() const {
            return (top->id << 24u) | (left->id << 16u) | (right->id << 8u) | bottom->id;
        }

//        [[nodiscard]] uint64_t numEncoding() const {
//            return (middle->id << 32u) | (top->id << 24u) | (left->id << 16u) | (right->id << 8u) | bottom->id;
//        }

        /*
         * Note that this function is used to remove cases where a gizmo contains the same components in a different
         * order, but has resulted in same perk probability.
         * Clearly this is NOT guaranteed to work, but seems to be good (and fast) enough for now...
         */
        [[nodiscard]] uint32_t compUsedIdSum() const {
            return middle->id + top->id + left->id + right->id + bottom->id;
        }

        double cost(std::map<InventionComponent *, double> costs) const {
            double total = 0.0;
            total += costs[middle];
            total += costs[top];
            total += costs[left];
            total += costs[right];
            total += costs[bottom];
            return total;
        }

        GizmoType type{};

        InventionComponent *middle;
        InventionComponent *top;
        InventionComponent *left;
        InventionComponent *right;
        InventionComponent *bottom;
    };

    bool operator<(const Gizmo &a, const Gizmo &b);

    struct GizmoPerk {
        InventionPerk *perk;
        uint8_t rank;
    };

    int operator-(const GizmoPerk &a, const GizmoPerk &b);

    unsigned int operator+(const GizmoPerk &a, const GizmoPerk &b);

    unsigned int operator<(const GizmoPerk &a, const GizmoPerk &b);

    bool operator==(const GizmoPerk &a, const GizmoPerk &b);

    struct PerkProbability {
        GizmoPerk perk1;
        GizmoPerk perk2;
        double probability{};

        [[nodiscard]] uint32_t encodePerks() const {
            return (perk1.perk->id << 24) | (perk1.rank << 16) | (perk2.perk->id << 8) | perk2.rank;
        }
    };

    struct GizmoPerkProbabilities {
        Gizmo gizmo;
        uint8_t invention_level;
        std::vector<PerkProbability> possible_perks;
    };

    namespace perks {
        extern std::map<std::string, std::shared_ptr<InventionPerk>> named_perks;
        extern std::map<uint8_t, std::shared_ptr<InventionPerk>> id_perks;

        void loadPerks(const std::string &filename);
    }

    std::pair<GizmoPerk, GizmoPerk> makePerkPair(GizmoPerk a, GizmoPerk b);

    namespace components {
        extern std::map<std::string, std::shared_ptr<InventionComponent>> named_components;
        extern std::map<uint8_t, std::shared_ptr<InventionComponent>> id_components;
        extern std::map<InventionComponent *, double> component_costs;

        extern InventionComponent empty;

        void loadComponents(const std::string &filename);
        void loadComponentCosts(const std::string &filename);

        std::vector<InventionComponent *> componentsWhichCanGenerate(const InventionPerk &perk,
                                                                     GizmoType type);
    }
}

namespace std {
    template<>
    struct hash<rs::GizmoPerk> {
        size_t operator()(const rs::GizmoPerk &k) const {
            return std::hash<string>()(k.perk->name) ^ std::hash<uint8_t>()(k.rank);
        }
    };

    template<>
    struct hash<std::pair<rs::GizmoPerk, rs::GizmoPerk>> {
        size_t operator()(const std::pair<rs::GizmoPerk, rs::GizmoPerk> &k) const {
            return std::hash<rs::GizmoPerk>()(k.first) ^ std::hash<rs::GizmoPerk>()(k.second);
        }
    };
}

#endif //RSPERKS_INVENTIONDATA_H
