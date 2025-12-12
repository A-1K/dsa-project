#ifndef WEATHER_ENGINE_HPP
#define WEATHER_ENGINE_HPP

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <stack>
#include <list>
#include <mutex>

// Removed "using namespace std;" to prevent conflict with Windows "byte"

// --- DATA MODELS ---
struct City {
    std::string name;
    double lat, lon;
    int temp, humidity, wind;
    std::string condition;
};

// Priority Level for Heap
struct Alert {
    int severity; // 10 = High, 1 = Low
    std::string message;
    std::string city;

    // Operator overload for Max-Heap (Highest severity on top)
    bool operator<(const Alert& other) const {
        return severity < other.severity;
    }
};

// --- THE ENGINE CLASS ---
class WeatherEngine {
private:
    // DSA 1: Hash Map for O(1) City Lookup
    std::unordered_map<std::string, City> cityDatabase;

    // DSA 2: Graph (Adjacency List) to connect cities (e.g., Isb -> Rawalpindi)
    std::unordered_map<std::string, std::vector<std::string>> cityGraph;

    // DSA 3: Max-Heap (Priority Queue) for Alerts
    std::priority_queue<Alert> alertSystem;

    // DSA 4: Stack for Request History (LIFO)
    std::stack<std::string> requestLogs;

    std::mutex engineMutex; // Thread safety

public:
    // --- 1. HASH MAP OPERATIONS ---
    void addCity(const City& c) {
        std::lock_guard<std::mutex> lock(engineMutex);
        cityDatabase[c.name] = c;
    }

    City* getCity(const std::string& name) {
        if (cityDatabase.find(name) != cityDatabase.end()) {
            return &cityDatabase[name];
        }
        return nullptr;
    }

    // --- 2. GRAPH OPERATIONS ---
    void addRoute(const std::string& cityA, const std::string& cityB) {
        std::lock_guard<std::mutex> lock(engineMutex);
        cityGraph[cityA].push_back(cityB);
        cityGraph[cityB].push_back(cityA); // Undirected graph
    }

    std::vector<std::string> getNeighbors(const std::string& name) {
        if (cityGraph.find(name) != cityGraph.end()) {
            return cityGraph[name];
        }
        return {};
    }

    // --- 3. HEAP OPERATIONS ---
    void addAlert(int severity, std::string msg, std::string city) {
        std::lock_guard<std::mutex> lock(engineMutex);
        alertSystem.push({severity, msg, city});
    }

    std::vector<Alert> getTopAlerts(int k) {
        std::lock_guard<std::mutex> lock(engineMutex);
        std::vector<Alert> topAlerts;
        std::priority_queue<Alert> temp = alertSystem; // Copy so we don't empty original

        while (!temp.empty() && k > 0) {
            topAlerts.push_back(temp.top());
            temp.pop();
            k--;
        }
        return topAlerts;
    }

    // --- 4. SORTING & LIST OPERATIONS ---
    // Returns top k cities sorted by Temperature (Hot to Cold)
    std::vector<City> getHottestCities(int k) {
        std::lock_guard<std::mutex> lock(engineMutex);
        std::vector<City> allCities;
        
        // Convert Hash Map to Vector for sorting
        for (auto& pair : cityDatabase) {
            allCities.push_back(pair.second);
        }

        // Optimization: Partial Sort is faster than full sort for Top K
        if (k > allCities.size()) k = allCities.size();
        
        std::partial_sort(allCities.begin(), allCities.begin() + k, allCities.end(), 
            [](const City& a, const City& b) {
                return a.temp > b.temp; // Descending order
            });

        // Resize to keep only top k
        allCities.resize(k);
        return allCities;
    }

    // --- 5. STACK OPERATIONS ---
    void logRequest(std::string req) {
        std::lock_guard<std::mutex> lock(engineMutex);
        requestLogs.push(req);
        if(requestLogs.size() > 50) requestLogs.pop(); // Keep size manageable
    }
};

#endif