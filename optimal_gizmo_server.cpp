#include <drogon/drogon.h>
#include <chrono>
#include <vector>
#include "rs/InventionData.h"
#include "rs/OptimalGizmo.h"

void respondError(std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string &&error_msg) {
    Json::Value json;
    json["error"] = error_msg;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    callback(resp);
}

Json::Value getComponentCosts() {
    Json::Value costs;
    for (const auto &pair : rs::components::component_costs) {
        costs[pair.first->name] = pair.second;
    }
    return costs;
}

void handleSearchPossibleGizmo(const drogon::HttpRequestPtr &req,
                               std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    Json::Value json;
    // INPUT HANDLING
    auto perk_1_name = req->getParameter("perk1");
    auto perk_2_name = req->getParameter("perk2");
    bool use_p2 = true;
    auto perk_1_rank_s = req->getParameter("rank1");
    auto perk_2_rank_s = req->getParameter("rank2");
    uint8_t perk_1_rank = 0;
    uint8_t perk_2_rank = 0;

    uint8_t invention_level = 120;

    if (!rs::perks::named_perks.count(perk_1_name)) {
        respondError(std::move(callback), "Perk 1 name does not exist.");
        return;
    } else {
        try {
            perk_1_rank = std::stoi(perk_1_rank_s);
        } catch (std::invalid_argument &e) {
            respondError(std::move(callback), "Perk 1 rank could not be interpreted as integer.");
            return;
        }
    }
    if (!rs::perks::named_perks.count(perk_2_name)) {
        use_p2 = false;
    } else {
        try {
            perk_2_rank = std::stoi(perk_2_rank_s);
        } catch (std::invalid_argument &e) {
            respondError(std::move(callback), "Perk 2 rank could not be interpreted as integer.");
            return;
        }
    }

    rs::InventionPerk* target_1 = rs::perks::named_perks[perk_1_name].get();
    rs::InventionPerk* target_2;
    if (use_p2) {
        target_2 = rs::perks::named_perks[perk_2_name].get();
    }

    std::pair<rs::GizmoPerk, rs::GizmoPerk> target;
    if (use_p2) {
        target = rs::makePerkPair({target_1, perk_1_rank}, {target_2, perk_2_rank});
    } else {
        target = rs::makePerkPair({target_1, perk_1_rank}, {nullptr, 0});
    }

    try {
        invention_level = std::stoi(req->getParameter("level"));
    } catch (std::invalid_argument &e) {
        respondError(std::move(callback), "Invention level could not be interpreted as integer.");
        return;
    }

    auto target_type_s = req->getParameter("type");
    rs::GizmoType type;
    if (target_type_s == "weapon") {
        type = rs::WEAPON;
    } else if (target_type_s == "armour") {
        type = rs::ARMOUR;
    } else if (target_type_s == "tool") {
        type = rs::TOOL;
    } else {
        respondError(std::move(callback), "Unknown gizmo type (or type not specified).");
        return;
    }

    // OUTPUT

    // Timing
    auto start = std::chrono::high_resolution_clock::now();
    auto perk_results = rs::findPossibleGizmos(target, type, invention_level, {}, rs::components::component_costs);

    if (perk_results.empty()) {
        respondError(std::move(callback), "No results found :(");
        return;
    }

    // For cheapest
    std::sort(perk_results.begin(), perk_results.end(), [](const auto &a, const auto &b) {
        if (std::abs(a.second.first - b.second.first) > 0.0000001) {
            return a.second.second < b.second.second;
        } else {
            if (std::abs(a.second.second - b.second.second) > 0.0000001) {
                return a.second.second < b.second.second;
            } else {
                return a.first.compUsedIdSum() < b.first.compUsedIdSum();
            }
        }
    });

    // Remove duplicates
    perk_results.erase(std::unique(perk_results.begin(), perk_results.end(), [](const auto &a, const auto &b) {
        return std::abs(a.second.first - b.second.first) < 0.0000001 &&
               a.first.compUsedIdSum() == b.first.compUsedIdSum();
    }), perk_results.end());

    // End timing
    auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    json["search_time_ms"] = Json::Value(static_cast<long long>(duration.count()));
    json["num_results"] = static_cast<int>(perk_results.size());

    Json::Value combo_list(Json::arrayValue);

    for (const auto &res : perk_results.size() > 10 ? std::vector<std::pair<rs::Gizmo, std::pair<double, double>>>(perk_results.begin(), perk_results.begin() + 10)
                                                    : perk_results) {
        Json::Value combo;
        auto giz = res.first;
        combo["probability"] = res.second.first;
        combo["expected_cost"] = res.second.second;
        combo["cost"] = res.first.cost(rs::components::component_costs);
        combo["middle"] = giz.middle->name;
        combo["top"] = giz.top->name;
        combo["left"] = giz.left->name;
        combo["right"] = giz.right->name;
        combo["bottom"] = giz.bottom->name;

        combo_list.append(combo);
    }

    json["cheapest"] = combo_list;

    // For highest chance
    std::sort(perk_results.begin(), perk_results.end(), [](const auto &a, const auto &b) {
        if (std::abs(a.second.first - b.second.first) > 0.0000001) {
            return a.second.first > b.second.first;
        } else {
            return a.second.second < b.second.second;
        }
    });
    Json::Value chance_list(Json::arrayValue);
    for (const auto &res : perk_results.size() > 10 ? std::vector<std::pair<rs::Gizmo, std::pair<double, double>>>(perk_results.begin(), perk_results.begin() + 10)
                                                    : perk_results) {
        Json::Value combo;
        auto giz = res.first;
        combo["probability"] = res.second.first;
        combo["expected_cost"] = res.second.second;
        combo["cost"] = res.first.cost(rs::components::component_costs);
        combo["middle"] = giz.middle->name;
        combo["top"] = giz.top->name;
        combo["left"] = giz.left->name;
        combo["right"] = giz.right->name;
        combo["bottom"] = giz.bottom->name;

        chance_list.append(combo);
    }

    json["highest_probable"] = chance_list;

    json["component_costs"] = getComponentCosts();

    json["interp_p1"] = target.first.perk->name;
    json["interp_p1_rank"] = target.first.rank;
    json["interp_p2"] = target.second.perk->name;
    json["interp_p2_rank"] = target.second.rank;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    callback(resp);
}

int main() {
    // Load our data files.
    rs::perks::loadPerks("../perkdata.csv");
    rs::components::loadComponents("../compdata.csv");
    rs::components::loadComponentCosts("../compcost.csv");

    drogon::app().registerHandler("/gizmo",
                                  [](const drogon::HttpRequestPtr &req,
                                     std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
                                      handleSearchPossibleGizmo(req, std::move(callback));
                                  },
                                  {drogon::Get});

    drogon::app().setLogPath("./")
            .setLogLevel(trantor::Logger::kWarn)
            .addListener("127.0.0.1", 8001)
            .setThreadNum(0)
            .run();
}
