#ifndef WEATHER_ENGINE_HPP
#define WEATHER_ENGINE_HPP

#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <fstream>
#include <limits>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

struct DailyForecast {
    std::string dayName;
    int high;
    int low;
    int rainProbability;
    std::string condition;
};

struct City {
    std::string name;
    double lat = 0.0;
    double lon = 0.0;
    int temp = 0;
    int humidity = 0;
    int wind = 0;
    int aqi = 0;
    double rain = 0.0;
    int windDir = 0;
    std::string condition;
    std::vector<int> hourlyData;
    std::vector<int> weeklyData;
    std::vector<int> monthlyData;
    std::vector<int> yearlyData;
    std::vector<DailyForecast> tenDayForecast;
};

struct RouteEdge {
    std::string city;
    double distanceKm = 0.0;
    int weatherRisk = 0;
};

struct Alert {
    int severity = 0;
    std::string message;
    std::string city;

    bool operator<(const Alert& other) const {
        return severity < other.severity;
    }
};

struct RouteResult {
    bool found = false;
    int totalRisk = 0;
    double totalDistanceKm = 0.0;
    std::vector<std::string> path;
};

class WeatherEngine {
private:
    struct TrieNode {
        bool terminal = false;
        std::vector<std::string> names;
        std::unordered_map<char, std::unique_ptr<TrieNode>> children;
    };

    std::unordered_map<std::string, City> cityDatabase;
    std::unordered_map<std::string, std::vector<RouteEdge>> cityGraph;
    std::priority_queue<Alert> alertSystem;
    std::vector<std::string> requestLogStack;
    std::unique_ptr<TrieNode> trieRoot = std::make_unique<TrieNode>();

    static std::string trim(const std::string& value) {
        size_t start = 0;
        while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) start++;

        size_t end = value.size();
        while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) end--;

        return value.substr(start, end - start);
    }

    static std::string normalize(const std::string& value) {
        std::string out = trim(value);
        std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return out;
    }

    static std::vector<std::string> splitCsvLine(const std::string& line) {
        std::vector<std::string> cols;
        std::string current;
        bool inQuotes = false;

        for (char ch : line) {
            if (ch == '"') {
                inQuotes = !inQuotes;
            } else if (ch == ',' && !inQuotes) {
                cols.push_back(trim(current));
                current.clear();
            } else {
                current.push_back(ch);
            }
        }

        cols.push_back(trim(current));
        return cols;
    }

    static std::vector<int> buildHourlySeries(int temp) {
        std::vector<int> data;
        for (int hour = 0; hour < 24; hour++) {
            double wave = std::sin((hour - 6) * 3.14159265 / 12.0);
            data.push_back(temp + static_cast<int>(std::round(wave * 4)));
        }
        return data;
    }

    static std::vector<int> buildWeeklySeries(int temp) {
        return {temp - 2, temp, temp + 1, temp + 2, temp + 1, temp - 1, temp};
    }

    static std::vector<int> buildMonthlySeries(int temp) {
        std::vector<int> data;
        for (int day = 0; day < 30; day++) {
            data.push_back(temp + static_cast<int>((day % 9) - 4));
        }
        return data;
    }

    static std::vector<int> buildYearlySeries(int temp) {
        return {temp - 13, temp - 10, temp - 5, temp, temp + 5, temp + 8,
                temp + 7, temp + 5, temp + 2, temp - 2, temp - 7, temp - 11};
    }

    static std::vector<DailyForecast> buildForecast(int temp, const std::string& condition, double rain) {
        const std::vector<std::string> days = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun", "Mon", "Tue", "Wed"};
        std::vector<DailyForecast> forecast;
        for (size_t i = 0; i < days.size(); i++) {
            int drift = static_cast<int>(i % 5) - 2;
            int rainProbability = rain > 0 ? std::min(95, 55 + static_cast<int>(rain * 5)) : 10 + static_cast<int>((i * 7) % 25);
            forecast.push_back({days[i], temp + drift + 3, temp + drift - 4, rainProbability, condition});
        }
        return forecast;
    }

    static double degToRad(double deg) {
        return deg * 3.14159265358979323846 / 180.0;
    }

    static double haversineKm(const City& a, const City& b) {
        constexpr double earthRadiusKm = 6371.0;
        double dLat = degToRad(b.lat - a.lat);
        double dLon = degToRad(b.lon - a.lon);
        double lat1 = degToRad(a.lat);
        double lat2 = degToRad(b.lat);

        double h = std::sin(dLat / 2) * std::sin(dLat / 2) +
                   std::cos(lat1) * std::cos(lat2) *
                   std::sin(dLon / 2) * std::sin(dLon / 2);
        return 2 * earthRadiusKm * std::asin(std::sqrt(h));
    }

    static int riskScore(const City& a, const City& b) {
        int avgAqi = (a.aqi + b.aqi) / 2;
        int avgWind = (a.wind + b.wind) / 2;
        int tempDelta = std::abs(a.temp - b.temp);
        int rainRisk = static_cast<int>(std::round((a.rain + b.rain) * 3.0));
        return std::max(1, avgAqi / 45 + avgWind / 8 + tempDelta / 4 + rainRisk);
    }

    void insertIntoTrie(const std::string& displayName) {
        TrieNode* node = trieRoot.get();
        const std::string key = normalize(displayName);
        for (char ch : key) {
            if (!node->children[ch]) {
                node->children[ch] = std::make_unique<TrieNode>();
            }
            node = node->children[ch].get();
            node->names.push_back(displayName);
        }
        node->terminal = true;
    }

public:
    bool loadCitiesFromCsv(const std::string& path, std::string* error = nullptr) {
        std::ifstream file(path);
        if (!file.is_open()) {
            if (error) *error = "Could not open " + path;
            return false;
        }

        std::string line;
        std::getline(file, line);
        int loaded = 0;

        while (std::getline(file, line)) {
            if (trim(line).empty()) continue;
            std::vector<std::string> cols = splitCsvLine(line);
            if (cols.size() < 10) continue;

            City city;
            city.name = cols[0];
            city.lat = std::stod(cols[1]);
            city.lon = std::stod(cols[2]);
            city.temp = std::stoi(cols[3]);
            city.condition = cols[4];
            city.wind = std::stoi(cols[5]);
            city.humidity = std::stoi(cols[6]);
            city.aqi = std::stoi(cols[7]);
            city.rain = std::stod(cols[8]);
            city.windDir = std::stoi(cols[9]);
            city.hourlyData = buildHourlySeries(city.temp);
            city.weeklyData = buildWeeklySeries(city.temp);
            city.monthlyData = buildMonthlySeries(city.temp);
            city.yearlyData = buildYearlySeries(city.temp);
            city.tenDayForecast = buildForecast(city.temp, city.condition, city.rain);
            addCity(city);
            loaded++;
        }

        if (loaded == 0 && error) {
            *error = "No city rows were loaded from " + path;
        }
        return loaded > 0;
    }

    void addCity(const City& city) {
        cityDatabase[normalize(city.name)] = city;
        insertIntoTrie(city.name);
    }

    bool getCity(const std::string& name, City& out) const {
        auto it = cityDatabase.find(normalize(name));
        if (it == cityDatabase.end()) return false;
        out = it->second;
        return true;
    }

    std::vector<City> getAllCities() const {
        std::vector<City> cities;
        for (const auto& pair : cityDatabase) {
            cities.push_back(pair.second);
        }
        std::sort(cities.begin(), cities.end(), [](const City& a, const City& b) {
            return a.name < b.name;
        });
        return cities;
    }

    void addRoute(const std::string& cityA, const std::string& cityB) {
        auto a = cityDatabase.find(normalize(cityA));
        auto b = cityDatabase.find(normalize(cityB));
        if (a == cityDatabase.end() || b == cityDatabase.end()) return;

        double distance = haversineKm(a->second, b->second);
        int risk = riskScore(a->second, b->second);
        cityGraph[normalize(cityA)].push_back({b->second.name, distance, risk});
        cityGraph[normalize(cityB)].push_back({a->second.name, distance, risk});
    }

    void addRoute(const std::string& cityA, const std::string& cityB, int weatherRisk, double distanceKm) {
        auto a = cityDatabase.find(normalize(cityA));
        auto b = cityDatabase.find(normalize(cityB));
        if (a == cityDatabase.end() || b == cityDatabase.end()) return;

        cityGraph[normalize(cityA)].push_back({b->second.name, distanceKm, weatherRisk});
        cityGraph[normalize(cityB)].push_back({a->second.name, distanceKm, weatherRisk});
    }

    std::vector<RouteEdge> getNeighbors(const std::string& name) const {
        auto it = cityGraph.find(normalize(name));
        if (it == cityGraph.end()) return {};
        return it->second;
    }

    std::vector<std::string> shortestRouteBfs(const std::string& start, const std::string& goal) const {
        const std::string startKey = normalize(start);
        const std::string goalKey = normalize(goal);
        if (!cityDatabase.count(startKey) || !cityDatabase.count(goalKey)) return {};

        std::queue<std::string> pending;
        std::unordered_set<std::string> visited;
        std::unordered_map<std::string, std::string> parent;
        pending.push(startKey);
        visited.insert(startKey);

        while (!pending.empty()) {
            std::string current = pending.front();
            pending.pop();
            if (current == goalKey) break;

            auto graphIt = cityGraph.find(current);
            if (graphIt == cityGraph.end()) continue;
            for (const RouteEdge& edge : graphIt->second) {
                std::string next = normalize(edge.city);
                if (visited.count(next)) continue;
                visited.insert(next);
                parent[next] = current;
                pending.push(next);
            }
        }

        if (!visited.count(goalKey)) return {};

        std::vector<std::string> path;
        for (std::string at = goalKey; !at.empty(); at = parent.count(at) ? parent[at] : "") {
            path.push_back(cityDatabase.at(at).name);
            if (at == startKey) break;
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    RouteResult summarizePath(const std::vector<std::string>& path) const {
        RouteResult result;
        result.path = path;
        result.found = !path.empty();
        if (path.size() < 2) return result;

        for (size_t i = 0; i + 1 < path.size(); i++) {
            const std::string fromKey = normalize(path[i]);
            const std::string toKey = normalize(path[i + 1]);
            auto graphIt = cityGraph.find(fromKey);
            if (graphIt == cityGraph.end()) {
                result.found = false;
                return result;
            }

            bool edgeFound = false;
            for (const RouteEdge& edge : graphIt->second) {
                if (normalize(edge.city) == toKey) {
                    result.totalRisk += edge.weatherRisk;
                    result.totalDistanceKm += edge.distanceKm;
                    edgeFound = true;
                    break;
                }
            }

            if (!edgeFound) {
                result.found = false;
                return result;
            }
        }

        return result;
    }

    RouteResult safestRouteDijkstra(const std::string& start, const std::string& goal) const {
        const std::string startKey = normalize(start);
        const std::string goalKey = normalize(goal);
        RouteResult result;
        if (!cityDatabase.count(startKey) || !cityDatabase.count(goalKey)) return result;

        using QueueItem = std::pair<int, std::string>;
        std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem>> pending;
        std::unordered_map<std::string, int> risk;
        std::unordered_map<std::string, double> distance;
        std::unordered_map<std::string, std::string> parent;

        for (const auto& pair : cityDatabase) {
            risk[pair.first] = std::numeric_limits<int>::max();
            distance[pair.first] = std::numeric_limits<double>::infinity();
        }
        risk[startKey] = 0;
        distance[startKey] = 0.0;
        pending.push({0, startKey});

        while (!pending.empty()) {
            QueueItem item = pending.top();
            int currentRisk = item.first;
            std::string current = item.second;
            pending.pop();
            if (currentRisk != risk[current]) continue;
            if (current == goalKey) break;

            auto graphIt = cityGraph.find(current);
            if (graphIt == cityGraph.end()) continue;
            for (const RouteEdge& edge : graphIt->second) {
                std::string next = normalize(edge.city);
                int candidateRisk = currentRisk + edge.weatherRisk;
                double candidateDistance = distance[current] + edge.distanceKm;
                if (candidateRisk < risk[next]) {
                    risk[next] = candidateRisk;
                    distance[next] = candidateDistance;
                    parent[next] = current;
                    pending.push({candidateRisk, next});
                }
            }
        }

        if (risk[goalKey] == std::numeric_limits<int>::max()) return result;

        result.found = true;
        result.totalRisk = risk[goalKey];
        result.totalDistanceKm = distance[goalKey];
        for (std::string at = goalKey; !at.empty(); at = parent.count(at) ? parent[at] : "") {
            result.path.push_back(cityDatabase.at(at).name);
            if (at == startKey) break;
        }
        std::reverse(result.path.begin(), result.path.end());
        return result;
    }

    void addAlert(int severity, const std::string& message, const std::string& city) {
        alertSystem.push({severity, message, city});
    }

    std::vector<Alert> getTopAlerts(int k) const {
        std::vector<Alert> alerts;
        std::priority_queue<Alert> copy = alertSystem;
        while (!copy.empty() && k-- > 0) {
            alerts.push_back(copy.top());
            copy.pop();
        }
        return alerts;
    }

    std::vector<City> getHottestCities(int k) const {
        std::vector<City> cities = getAllCities();
        if (k < 0) k = 0;
        if (static_cast<size_t>(k) > cities.size()) k = static_cast<int>(cities.size());
        std::partial_sort(cities.begin(), cities.begin() + k, cities.end(), [](const City& a, const City& b) {
            return a.temp > b.temp;
        });
        cities.resize(k);
        return cities;
    }

    std::vector<City> getColdestCities(int k) const {
        std::vector<City> cities = getAllCities();
        if (k < 0) k = 0;
        if (static_cast<size_t>(k) > cities.size()) k = static_cast<int>(cities.size());
        std::partial_sort(cities.begin(), cities.begin() + k, cities.end(), [](const City& a, const City& b) {
            return a.temp < b.temp;
        });
        cities.resize(k);
        return cities;
    }

    std::vector<std::string> autocomplete(const std::string& prefix, size_t limit = 5) const {
        const std::string key = normalize(prefix);
        const TrieNode* node = trieRoot.get();
        for (char ch : key) {
            auto it = node->children.find(ch);
            if (it == node->children.end()) return {};
            node = it->second.get();
        }

        std::vector<std::string> matches = node->names;
        std::sort(matches.begin(), matches.end());
        matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
        if (matches.size() > limit) matches.resize(limit);
        return matches;
    }

    void logRequest(const std::string& request) {
        requestLogStack.push_back(request);
        if (requestLogStack.size() > 50) {
            requestLogStack.erase(requestLogStack.begin());
        }
    }

    std::vector<std::string> recentRequests(size_t limit = 10) const {
        std::vector<std::string> out;
        for (auto it = requestLogStack.rbegin(); it != requestLogStack.rend() && out.size() < limit; ++it) {
            out.push_back(*it);
        }
        return out;
    }
};

#endif
