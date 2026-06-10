#include "WeatherEngine.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

namespace {
std::string findDataPath() {
    const std::string paths[] = {
        "data/weather_data.csv",
        "../data/weather_data.csv",
        "../../data/weather_data.csv"
    };

    for (const std::string& path : paths) {
        std::ifstream file(path);
        if (file.good()) return path;
    }

    return "data/weather_data.csv";
}

void buildEngine(WeatherEngine& engine) {
    std::string error;
    assert(engine.loadCitiesFromCsv(findDataPath(), &error) && "CSV data should load");

    engine.addRoute("Islamabad", "Peshawar");
    engine.addRoute("Islamabad", "Lahore");
    engine.addRoute("Islamabad", "Topi");
    engine.addRoute("Islamabad", "Rawalpindi");
    engine.addRoute("Peshawar", "Topi");
    engine.addRoute("Rawalpindi", "Lahore");
    engine.addRoute("Lahore", "Multan");
    engine.addRoute("Lahore", "Faisalabad");
    engine.addRoute("Faisalabad", "Multan");
    engine.addRoute("Multan", "Hyderabad");
    engine.addRoute("Hyderabad", "Karachi");
    engine.addRoute("Karachi", "Quetta");
    engine.addRoute("Quetta", "Multan");

    engine.addAlert(9, "Heat advisory", "Multan");
    engine.addAlert(4, "Light rain", "Islamabad");
    engine.addAlert(8, "Air quality warning", "Lahore");
}
}

int main() {
    WeatherEngine engine;
    buildEngine(engine);

    City lahore;
    assert(engine.getCity("lahore", lahore));
    assert(lahore.name == "Lahore");
    assert(lahore.hourlyData.size() == 24);
    assert(lahore.tenDayForecast.size() == 10);

    auto suggestions = engine.autocomplete("is");
    assert(!suggestions.empty());
    assert(suggestions[0] == "Islamabad");

    auto neighbors = engine.getNeighbors("Islamabad");
    assert(neighbors.size() == 4);

    auto bfs = engine.shortestRouteBfs("Topi", "Karachi");
    assert(!bfs.empty());
    assert(bfs.front() == "Topi");
    assert(bfs.back() == "Karachi");

    RouteResult safest = engine.safestRouteDijkstra("Topi", "Karachi");
    assert(safest.found);
    assert(safest.totalRisk > 0);
    assert(safest.path.front() == "Topi");
    assert(safest.path.back() == "Karachi");

    auto alerts = engine.getTopAlerts(2);
    assert(alerts.size() == 2);
    assert(alerts[0].severity >= alerts[1].severity);

    auto hottest = engine.getHottestCities(3);
    assert(hottest.size() == 3);
    assert(hottest[0].temp >= hottest[1].temp);

    engine.logRequest("GET /api/weather?city=Topi");
    auto logs = engine.recentRequests();
    assert(!logs.empty());
    assert(logs[0] == "GET /api/weather?city=Topi");

    std::cout << "All WeatherEngine tests passed." << std::endl;
    return 0;
}
