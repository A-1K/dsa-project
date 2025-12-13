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

// New Struct for Daily Forecast (DSA: Struct/Object)
struct DailyForecast {
    std::string dayName;
    int high;
    int low;
    std::string condition; // e.g., "Sunny", "Rainy"
};

struct City {
    std::string name;
    double lat, lon;
    int temp, humidity, wind;
    std::string condition;
    
    // --- DSA: VECTORS FOR TIME-SERIES DATA ---
    // Instead of generating these in JS, we store them here.
    std::vector<int> hourlyData;  // 24 points
    std::vector<int> weeklyData;  // 7 points
    std::vector<int> monthlyData; // 30 points
    std::vector<int> yearlyData;  // 12 points

    // --- DSA: VECTOR FOR FORECAST OBJECTS ---
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
    std::unordered_map<std::string, City> cityDatabase;
    std::unordered_map<std::string, std::vector<std::string>> cityGraph;
    std::priority_queue<Alert> alertSystem;
    std::stack<std::string> requestLogs;
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
        cityGraph[cityA].push_back(cityB);
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
        alertSystem.push({severity, msg, city});
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
        
        std::partial_sort(allCities.begin(), allCities.begin() + k, allCities.end(), 
            [](const City& a, const City& b) {
                return a.temp > b.temp; 
            });

        allCities.resize(k);
        return allCities;
    }

    void logRequest(std::string req) {
        std::lock_guard<std::mutex> lock(engineMutex);
        requestLogs.push(req);
        if(requestLogs.size() > 50) requestLogs.pop();
    }
};

#endif