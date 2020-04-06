//
// Created by Alexander Logan on 06/04/2020.
//

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <chrono>
#include <thread>
#include "../rs/InventionTypes.h"
#include "../rs/Component.h"
#include "../rs/Perk.h"
#include "../rs/Gizmo.h"
#include "../rs/OptimalGizmoSearch.h"

#define REL_VERSION "1.0"


#ifdef __clang__
constexpr const char* true_cxx = "clang++";
#else
constexpr const char *true_cxx = "g++";
#endif

void printUsage() {
    std::cout << "No usage information yet :(" << std::endl;
}

bool valid_number(const std::string &s) {
    return !s.empty() &&
           std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

template<typename T>
std::vector<T> search_filter_names(const std::vector<T> &objs, const std::string &search) {
    // Use lowercase search term.
    std::string lookup(search);
    std::transform(search.begin(), search.end(), lookup.begin(), ::tolower);

    std::vector<T> results;
    std::copy_if(objs.begin(), objs.end(), std::back_inserter(results),
                 [&lookup](const T &obj) {
                     std::string obj_name(obj.name());
                     // Convert name to lowercase.
                     std::transform(obj_name.begin(), obj_name.end(), obj_name.begin(), ::tolower);
                     // Check if lookup matches start of object name.
                     return std::equal(lookup.begin(), lookup.end(), obj_name.begin());
                 });
    return results;
}

void printProgress(OptimalGizmoSearch *const obj) {
    while (obj->results_searched < obj->total_candidates) {
        double progress = static_cast<double>(obj->results_searched) / static_cast<double>(obj->total_candidates);
        std::cout << "\33[2K\rProgress: Searched "
                  << obj->results_searched << "/" << obj->total_candidates
                  << " (" << std::fixed << std::setprecision(1) << progress * 100 << "%)"
                  << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

int main(int argc, char **argv) {
    // Read arguments.
    std::vector<std::string> args;
    if (argc == 1) {
        std::cout << "No arguments provided." << std::endl;
        printUsage();
        exit(1);
    }
    args.assign(argv + 1, argv + argc);

    // Print welcome.
    std::cout << "Optimal Gizmo Search Tool (" << REL_VERSION << ") by AJLogan (github.com/ajlogan1/RsOptimalGizmo)"
              << std::endl;
    std::cout << "  " << "Built with " << true_cxx << " version " << __VERSION__ << " on " << __DATE__ << " "
              << __TIME__ << std::endl;

    // Load configuration.
    Perk::registerPerks("../perkdata.csv");
    Component::registerComponents("../compdata.csv");

    // Options and defaults.
    EquipmentType equipment_type;
    GizmoType gizmo_type = STANDARD;
    level_t invention_level = 120;
    bool strict_target = true;
    size_t max_results = 1;
    std::vector<Component> excluded_components;

    // Targets.
    Perk target_1 = Perk::no_effect;
    rank_t target_1_rank = 0;
    Perk target_2 = Perk::no_effect;
    rank_t target_2_rank = 0;

    // Parse arguments and fill options.
    size_t arg_idx = 0;
    while (arg_idx < args.size()) {
        std::string &token = args[arg_idx];

        // Setting - Equipment Type
        if (token == "-w" || token == "--weapon") {
            equipment_type = WEAPON;
        }
        if (token == "-t" || token == "--tool") {
            equipment_type = TOOL;
        }
        if (token == "-a" || token == "--armour") {
            equipment_type = ARMOUR;
        }

        // Setting - Gizmo Type
        if (token == "-std" || token == "--standard") {
            gizmo_type = STANDARD;
        }
        if (token == "-anc" || token == "--ancient") {
            gizmo_type = ANCIENT;
        }

        // Setting - Invention Level
        if (token == "-l" || token == "--level") {
            // Next token is level to set.
            arg_idx++;
            std::string &lvl_token = args[arg_idx];
            invention_level = std::stoi(lvl_token);
        }

        // Setting - Max Results
        if (token == "-n" || token == "--num-results") {
            // Next token is number of results.
            arg_idx++;
            std::string &num_token = args[arg_idx];
            max_results = std::stoi(num_token);
        }

        // Target Perks
        if (token == "-p" || token == "--target") {
            // We read until we reach another token which starts with a '-'.
            arg_idx++;
            std::vector<std::string> target_tokens;
            while (arg_idx < args.size() && args[arg_idx][0] != '-') {
                target_tokens.push_back(args[arg_idx]);
                arg_idx++;
            }
            arg_idx--;
            rank_t target_rank = 1;

            // If the last number in the target tokens is a number, interpret it as the rank.
            bool rank_specified = false;
            if (valid_number(target_tokens[target_tokens.size() - 1])) {
                target_rank = std::stoi(target_tokens[target_tokens.size() - 1]);
                rank_specified = true;
            }

            // Build perk name from other tokens.
            std::string target_name = std::accumulate(target_tokens.begin(),
                                                      target_tokens.end() - (rank_specified ? 1 : 0),
                                                      std::string(),
                                                      [](const std::string &acc, const std::string &token) {
                                                          return acc + (acc.length() > 0 ? " " : "") + token;
                                                      });

            // Search for target perks.
            std::vector<Perk> perk_search_results = search_filter_names(Perk::all(), target_name);
            if (perk_search_results.size() == 0) {
                std::cout << "[Error] Perk '" << target_name << "' could not be found." << std::endl;
                exit(2);
            }
            if (perk_search_results.size() > 1) {
                std::cout << "[Error] Perk '" << target_name << "' is ambiguous. Could be one of: " << std::endl;
                for (Perk result : perk_search_results) {
                    std::cout << "    " << result.name() << std::endl;
                }
                std::cout << "Please specify one of these." << std::endl;
                exit(2);
            }

            Perk target_perk = perk_search_results[0];

            // Set the perks.
            if (target_1 == Perk::no_effect) {
                target_1 = target_perk;
                target_1_rank = target_rank;
            } else if (target_2 == Perk::no_effect) {
                target_2 = target_perk;
                target_2_rank = target_rank;
            } else {
                std::cout << "[Error] You can only specify up to two perks to search for." << std::endl;
                exit(2);
            }
        }

        // Excluded components.
        if (token == "-x" || token == "--exclude") {
            // We read until we reach another token which starts with a '-'.
            arg_idx++;
            std::vector<std::string> target_tokens;
            while (arg_idx < args.size() && args[arg_idx][0] != '-') {
                target_tokens.push_back(args[arg_idx]);
                arg_idx++;
            }
            arg_idx--;
            rank_t target_rank = 1;

            // Build component name from other tokens.
            std::string target_name = std::accumulate(target_tokens.begin(),
                                                      target_tokens.end(),
                                                      std::string(),
                                                      [](const std::string &acc, const std::string &token) {
                                                          return acc + (acc.length() > 0 ? " " : "") + token;
                                                      });

            // Search for target perks.
            std::vector<Component> component_search_results = search_filter_names(Component::all(), target_name);
            if (component_search_results.size() == 0) {
                std::cout << "[Error] Component '" << target_name << "' could not be found." << std::endl;
                exit(2);
            }
            if (component_search_results.size() > 1) {
                std::cout << "[Error] Component '" << target_name << "' is ambiguous. Could be one of: " << std::endl;
                for (Component result : component_search_results) {
                    std::cout << "    " << result.name() << std::endl;
                }
                std::cout << "Please specify one of these." << std::endl;
                exit(2);
            }

            excluded_components.push_back(component_search_results[0]);
        }

        arg_idx++;
    }

    // Build target gizmo result.
    GizmoResult target({target_1, target_1_rank}, {target_2, target_2_rank});

    // Options set up.
    // Echo the options.
    std::cout << std::endl << "Search configuration:" << std::endl;
    std::cout << std::setw(18) << "Gizmo Type: " << gizmo_type << std::endl;
    std::cout << std::setw(18) << "Equipment Type: " << equipment_type << std::endl;
    std::cout << std::setw(18) << "Invention Level: " << unsigned(invention_level) << std::endl;
    std::cout << std::setw(18) << "Target Perks: " << target << std::endl;
    if (excluded_components.size() > 0) {
        std::cout << std::setw(18) << "Excluded: " << excluded_components[0] << std::endl;
        std::for_each(excluded_components.begin() + 1,
                      excluded_components.end(),
                      [](const Component &comp) {
                          std::cout << std::setw(18) << " " << comp << std::endl;
                      });
    }
    std::cout << std::endl;

    // Begin the search.
    OptimalGizmoSearch search(equipment_type, gizmo_type, target);

    std::cout << "Status: Generating candidate gizmos..." << std::flush;
    size_t num_candidates = search.build_candidate_list(excluded_components);
    std::cout << "\33[2K\rStatus: Searching " << num_candidates << " candidate gizmos..." << std::flush;
    std::thread progressThread(printProgress, &search);

    auto start = std::chrono::high_resolution_clock::now();
    auto results = search.results(invention_level);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    progressThread.join();
    double gizmo_per_second = static_cast<double>(num_candidates) / duration.count();
    std::cout.imbue(std::locale());
    std::cout << "\33[2K\rSearch completed in "
              << duration.count()
              << "ms! (~" << unsigned(gizmo_per_second * 1000) << " gizmos/s)" << std::endl;

    std::cout << std::endl << "Results:" << std::endl;

    for (size_t i = 0; i < (results.size() > max_results ? max_results : results.size()); i++) {
        std::cout << results[i] << std::endl << std::endl;
    }
}
