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

// --- DATA MODELS ---

struct DailyForecast {
    std::string dayName;
    int high;
    int low;
    std::string condition;
};

struct City {
    std::string name;
    double lat, lon;
    int temp, humidity, wind;
    int wind_dir; // <--- NEW: Stores wind direction (0-360 degrees)
    std::string condition;
    
    // --- DSA: VECTORS FOR TIME-SERIES ---
    std::vector<int> hourlyData; // dynamic array vector stores contiguous sequences of temperature integers for efficient iteration
    std::vector<int> weeklyData;
    std::vector<int> monthlyData;
    std::vector<int> yearlyData;

    // --- DSA: FORECAST OBJECTS ---
    std::vector<DailyForecast> tenDayForecast;
};

// Priority Level for Heap
struct Alert {
    int severity; 
    std::string message;
    std::string city;

    bool operator<(const Alert& other) const {
        return severity < other.severity;
    }
};

// --- THE ENGINE CLASS ---
class WeatherEngine {
private:
    // DSA 1: Hash Map
    std::unordered_map<std::string, City> cityDatabase; // hash map provides constant time lookup for city data objects using string keys

    // DSA 2: Graph (Adjacency List)
    std::unordered_map<std::string, std::vector<std::string>> cityGraph; // adjacency list implementation uses a map of vectors to represent graph connections between cities

    // DSA 3: Max-Heap (Priority Queue)
    std::priority_queue<Alert> alertSystem; // max-heap priority queue automatically orders alerts so the highest severity is always at the top

    // DSA 4: Stack
    std::stack<std::string> requestLogs; // stack data structure follows last-in-first-out logic to store the history of server requests

    std::mutex engineMutex;

public:
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

    void addRoute(const std::string& cityA, const std::string& cityB) {
        std::lock_guard<std::mutex> lock(engineMutex);
        cityGraph[cityA].push_back(cityB); // graph edge insertion adds a connection between two nodes in the adjacency list
        cityGraph[cityB].push_back(cityA); 
    }

    std::vector<std::string> getNeighbors(const std::string& name) {
        if (cityGraph.find(name) != cityGraph.end()) {
            return cityGraph[name];
        }
        return {};
    }

    void addAlert(int severity, std::string msg, std::string city) {
        std::lock_guard<std::mutex> lock(engineMutex);
        alertSystem.push({severity, msg, city}); // heap insertion adds an element and rebalances the tree based on the severity comparison
    }

    std::vector<Alert> getTopAlerts(int k) {
        std::lock_guard<std::mutex> lock(engineMutex);
        std::vector<Alert> topAlerts;
        std::priority_queue<Alert> temp = alertSystem; 
        while (!temp.empty() && k > 0) {
            topAlerts.push_back(temp.top());
            temp.pop();
            k--;
        }
        return topAlerts;
    }

    std::vector<City> getHottestCities(int k) {
        std::lock_guard<std::mutex> lock(engineMutex);
        std::vector<City> allCities;
        for (auto& pair : cityDatabase) {
            allCities.push_back(pair.second);
        }
        if (k > allCities.size()) k = allCities.size();
        
        // sorting algorithm rearranges only the top k elements to optimize ranking performance
        std::partial_sort(allCities.begin(), allCities.begin() + k, allCities.end(), 
            [](const City& a, const City& b) {
                return a.temp > b.temp; 
            });

        allCities.resize(k);
        return allCities;
    }

    void logRequest(std::string req) {
        std::lock_guard<std::mutex> lock(engineMutex);
        requestLogs.push(req); // stack push operation places the most recent request string onto the top of the log history
        if(requestLogs.size() > 50) requestLogs.pop();
    }
};

#endif