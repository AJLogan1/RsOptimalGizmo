#include "InventionData.h"

#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>

namespace rs {
    int operator-(const GizmoPerk &a, const GizmoPerk &b) {
        if (a.rank == 0 && b.rank == 0) {
            return 0;
        } else if (a.rank == 0) {
            return -1;
        } else if (b.rank == 0) {
            return 1;
        }
        return a.perk->ranks[a.rank - 1].cost - b.perk->ranks[b.rank - 1].cost;
    }

    unsigned int operator+(const GizmoPerk &a, const GizmoPerk &b) {
        if (a.rank == 0 && b.rank == 0) {
            return 0;
        } else if (a.rank == 0) {
            return b.perk->ranks[b.rank - 1].cost;
        } else if (b.rank == 0) {
            return a.perk->ranks[a.rank - 1].cost;
        }
        return a.perk->ranks[a.rank - 1].cost + b.perk->ranks[b.rank - 1].cost;
    }

    unsigned int operator<(const GizmoPerk &a, const GizmoPerk &b) {
        return a - b < 0;
    }

    bool operator==(const GizmoPerk &a, const GizmoPerk &b) {
        return a.perk == b.perk && a.rank == b.rank;
    }

    std::pair<GizmoPerk, GizmoPerk> makePerkPair(GizmoPerk a, GizmoPerk b) {
        std::pair<GizmoPerk, GizmoPerk> pair;
        if (a.rank == 0 && b.rank == 0) {
            pair = std::make_pair(rs::GizmoPerk{rs::perks::named_perks["No effect"].get(), 0},
                                  rs::GizmoPerk{rs::perks::named_perks["No effect"].get(), 0});
        } else if (a.rank == 0) {
            pair = std::make_pair(b, rs::GizmoPerk{rs::perks::named_perks["No effect"].get(), 0});
        } else if (b.rank == 0 || a.perk->isTwoSlot()) {
            pair = std::make_pair(a, rs::GizmoPerk{rs::perks::named_perks["No effect"].get(), 0});
        } else {
            if (a.perk->id < b.perk->id) {
                pair = std::make_pair(a, b);
            } else {
                pair = std::make_pair(b, a);
            }
        }

        return pair;
    }

    bool operator<(const Gizmo &a, const Gizmo &b) {
        if (a.type != b.type) {
            return a.type < b.type;
        }
        if (a.middle != b.middle) {
            return a.middle < b.middle;
        }
        if (a.top != b.top) {
            return a.top < b.top;
        }
        if (a.left != b.left) {
            return a.left < b.left;
        }
        if (a.right != b.right) {
            return a.right < b.right;
        }
        if (a.bottom != b.bottom) {
            return a.bottom < b.bottom;
        }
        return false;
    }

    namespace perks {
        std::map<std::string, std::shared_ptr<InventionPerk>> named_perks;
        std::map<uint8_t, std::shared_ptr<InventionPerk>> id_perks;

        void loadPerks(const std::string &filename) {
            std::ifstream perkDataFile;
            perkDataFile.open(filename);
            if (!perkDataFile) {
                std::cerr << "Error: Perk data file " << filename << " could not be opened." << std::endl;
                exit(1);
            }

            std::string line;
            while (std::getline(perkDataFile, line)) {
                std::vector<std::string> data;
                boost::split(data, line, [](char c) { return c == ','; });

                uint8_t perk_id = std::stoi(data[0]);
                uint8_t rank = std::stoi(data[2]);
                uint8_t cost = std::stoi(data[3]);
                uint8_t threshold = std::stoi(data[4]);

                if (id_perks.count(perk_id)) {
                    // Perk already exists in our maps, add the new rank information.
                    id_perks[perk_id]->ranks.push_back({rank, cost, threshold});
                } else {
                    // Create a new perk and link to it in the maps.
                    auto new_perk = std::make_shared<InventionPerk>(perk_id, data[1]);
                    new_perk->ranks.push_back({rank, cost, threshold});
                    named_perks[data[1]] = new_perk;
                    id_perks[perk_id] = new_perk;
                }
            }

            perkDataFile.close();
//            std::cout << "Loaded " << id_perks.size() << " perks." << std::endl;
        }
    }

    namespace components {
        std::map<std::string, std::shared_ptr<InventionComponent>> named_components;
        std::map<uint8_t, std::shared_ptr<InventionComponent>> id_components;
        std::map<InventionComponent *, double> component_costs;

        InventionComponent empty = {255, "Empty"};

        void loadComponents(const std::string &filename) {
            std::ifstream compDataFile;
            compDataFile.open(filename);
            if (!compDataFile) {
                std::cerr << "Error: Component data file " << filename << " could not be opened." << std::endl;
                exit(1);
            }

            std::string line;
            while (std::getline(compDataFile, line)) {
                std::vector<std::string> data;
                boost::split(data, line, [](char c) { return c == ','; });

                uint8_t comp_id = std::stoi(data[0]);
                std::string comp_name = data[1];
                std::string gizmo_type = data[2];
                std::string perk_name = data[3];
                uint8_t base = std::stoi(data[4]);
                uint8_t roll = std::stoi(data[5]);

                std::shared_ptr<InventionComponent> comp;

                if (id_components.count(comp_id)) {
                    comp = id_components[comp_id];
                } else {
                    auto new_component = std::make_shared<InventionComponent>(comp_id, comp_name);
                    named_components[comp_name] = new_component;
                    id_components[comp_id] = new_component;
                    comp = new_component;
                }

                InventionPerk *perk = rs::perks::named_perks[perk_name].get();

                if (gizmo_type == "weapon") {
                    comp->weapon_perks.push_back({perk, base, roll});
                }
                if (gizmo_type == "tool") {
                    comp->tool_perks.push_back({perk, base, roll});
                }
                if (gizmo_type == "armour") {
                    comp->armour_perks.push_back({perk, base, roll});
                }
            }

            compDataFile.close();
//            std::cout << "Loaded " << id_components.size() << " components." << std::endl;
        }

        void loadComponentCosts(const std::string &filename) {
            std::ifstream compDataFile;
            compDataFile.open(filename);
            if (!compDataFile) {
                std::cerr << "Error: Component cost data file " << filename << " could not be opened." << std::endl;
                exit(1);
            }

            std::string line;
            while (std::getline(compDataFile, line)) {
                std::vector<std::string> data;
                boost::split(data, line, [](char c) { return c == ','; });

                uint8_t comp_id = std::stoi(data[0]);
                int price = std::stoi(data[2]);

                component_costs[id_components[comp_id].get()] = static_cast<double>(price);
            }

            compDataFile.close();
        }

        std::vector<InventionComponent *>
        componentsWhichCanGenerate(const InventionPerk &perk, GizmoType type) {
            std::vector<InventionComponent *> results;

            for (const auto &pair : id_components) {
                if (type == WEAPON) {
                    if (std::any_of(pair.second->weapon_perks.begin(),
                                    pair.second->weapon_perks.end(),
                                    [&perk](const auto &x) { return x.perk->id == perk.id; })) {
                        results.push_back(pair.second.get());
                    }
                }
                if (type == ARMOUR) {
                    if (std::any_of(pair.second->armour_perks.begin(),
                                    pair.second->armour_perks.end(),
                                    [&perk](const auto &x) { return x.perk->id == perk.id; })) {
                        results.push_back(pair.second.get());
                    }
                }
                if (type == TOOL) {
                    if (std::any_of(pair.second->tool_perks.begin(),
                                    pair.second->tool_perks.end(),
                                    [&perk](const auto &x) { return x.perk->id == perk.id; })) {
                        results.push_back(pair.second.get());
                    }
                }
            }

            return results;
        }
    }
}
