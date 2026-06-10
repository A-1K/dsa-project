#include "NetworkUtils.hpp"
#include "WeatherEngine.hpp"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
WeatherEngine engine;
bool running = true;

std::string jsonEscape(const std::string& value) {
    std::ostringstream out;
    for (char ch : value) {
        switch (ch) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << ch; break;
        }
    }
    return out.str();
}

template <typename T>
std::string numberArrayJson(const std::vector<T>& values) {
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < values.size(); i++) {
        json << values[i];
        if (i + 1 < values.size()) json << ",";
    }
    json << "]";
    return json.str();
}

std::string stringArrayJson(const std::vector<std::string>& values) {
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < values.size(); i++) {
        json << "\"" << jsonEscape(values[i]) << "\"";
        if (i + 1 < values.size()) json << ",";
    }
    json << "]";
    return json.str();
}

std::string forecastJson(const std::vector<DailyForecast>& forecast) {
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < forecast.size(); i++) {
        const DailyForecast& day = forecast[i];
        json << "{"
             << "\"day\":\"" << jsonEscape(day.dayName) << "\","
             << "\"high\":" << day.high << ","
             << "\"low\":" << day.low << ","
             << "\"rain_prob\":" << day.rainProbability << ","
             << "\"cond\":\"" << jsonEscape(day.condition) << "\""
             << "}";
        if (i + 1 < forecast.size()) json << ",";
    }
    json << "]";
    return json.str();
}

std::string routeEdgesJson(const std::vector<RouteEdge>& edges) {
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < edges.size(); i++) {
        json << "{"
             << "\"city\":\"" << jsonEscape(edges[i].city) << "\","
             << "\"distance_km\":" << static_cast<int>(edges[i].distanceKm + 0.5) << ","
             << "\"weather_risk\":" << edges[i].weatherRisk
             << "}";
        if (i + 1 < edges.size()) json << ",";
    }
    json << "]";
    return json.str();
}

std::string cityListJson(const std::vector<City>& cities, bool includeTemp = false) {
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < cities.size(); i++) {
        json << "{"
             << "\"name\":\"" << jsonEscape(cities[i].name) << "\"";
        if (includeTemp) {
            json << ",\"temp\":" << cities[i].temp
                 << ",\"condition\":\"" << jsonEscape(cities[i].condition) << "\"";
        }
        json << "}";
        if (i + 1 < cities.size()) json << ",";
    }
    json << "]";
    return json.str();
}

std::string weatherJson(const City& city) {
    std::ostringstream json;
    json << "{"
         << "\"city\":\"" << jsonEscape(city.name) << "\","
         << "\"lat\":" << city.lat << ","
         << "\"lon\":" << city.lon << ","
         << "\"condition\":\"" << jsonEscape(city.condition) << "\","
         << "\"current\":{"
         << "\"temperature_2d\":" << city.temp << ","
         << "\"wind_speed_10m\":" << city.wind << ","
         << "\"relative_humidity_2d\":" << city.humidity << ","
         << "\"rain\":" << city.rain << ","
         << "\"aqi\":" << city.aqi << ","
         << "\"wind_dir\":" << city.windDir << ","
         << "\"condition\":\"" << jsonEscape(city.condition) << "\""
         << "},"
         << "\"hourly\":" << numberArrayJson(city.hourlyData) << ","
         << "\"weekly\":" << numberArrayJson(city.weeklyData) << ","
         << "\"monthly\":" << numberArrayJson(city.monthlyData) << ","
         << "\"yearly\":" << numberArrayJson(city.yearlyData) << ","
         << "\"forecast\":" << forecastJson(city.tenDayForecast) << ","
         << "\"neighbors\":" << routeEdgesJson(engine.getNeighbors(city.name)) << ","
         << "\"hottest_cities\":" << cityListJson(engine.getHottestCities(5), true) << ","
         << "\"coldest_cities\":" << cityListJson(engine.getColdestCities(5), true)
         << "}";
    return json.str();
}

std::string alertsJson(int k) {
    std::vector<Alert> alerts = engine.getTopAlerts(k);
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < alerts.size(); i++) {
        json << "{"
             << "\"severity\":" << alerts[i].severity << ","
             << "\"message\":\"" << jsonEscape(alerts[i].message) << "\","
             << "\"city\":\"" << jsonEscape(alerts[i].city) << "\""
             << "}";
        if (i + 1 < alerts.size()) json << ",";
    }
    json << "]";
    return json.str();
}

std::string routeJson(const RouteResult& route, const std::string& algorithm) {
    std::ostringstream json;
    json << "{"
         << "\"found\":" << (route.found ? "true" : "false") << ","
         << "\"algorithm\":\"" << algorithm << "\","
         << "\"path\":" << stringArrayJson(route.path) << ","
         << "\"hops\":" << (route.path.empty() ? 0 : route.path.size() - 1) << ","
         << "\"total_risk\":" << route.totalRisk << ","
         << "\"total_distance_km\":" << static_cast<int>(route.totalDistanceKm + 0.5)
         << "}";
    return json.str();
}

std::string findExistingPath(const std::vector<std::string>& candidates) {
    for (const std::string& path : candidates) {
        if (!SimpleServer::loadTextFile(path).empty()) return path;
    }
    return "";
}

int parseIntParam(const std::unordered_map<std::string, std::string>& params,
                  const std::string& key,
                  int fallback) {
    auto it = params.find(key);
    if (it == params.end()) return fallback;
    try {
        return std::stoi(it->second);
    } catch (...) {
        return fallback;
    }
}

void sendJson(SOCKET clientSock, const std::string& body, int statusCode = 200, const std::string& statusText = "OK") {
    SimpleServer::sendResponse(clientSock, body, "application/json", statusCode, statusText);
}

void seedRoutesAndAlerts() {
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

    for (const City& city : engine.getAllCities()) {
        if (city.temp >= 30) {
            engine.addAlert(9, "Heat advisory: high temperature trend", city.name);
        }
        if (city.aqi >= 150) {
            engine.addAlert(8, "Air quality warning: reduce outdoor exposure", city.name);
        }
        if (city.rain >= 5.0) {
            engine.addAlert(7, "Rainfall alert: possible slick routes", city.name);
        }
        if (city.wind >= 20) {
            engine.addAlert(6, "Wind advisory: strong gusts expected", city.name);
        }
    }
}

void handleApi(SOCKET clientSock, const std::string& path, const std::unordered_map<std::string, std::string>& params) {
    if (path == "/api/cities") {
        sendJson(clientSock, cityListJson(engine.getAllCities()));
        return;
    }

    if (path == "/api/weather" || path == "/data") {
        std::string cityName = params.count("city") ? params.at("city") : "Topi";
        City city;
        if (!engine.getCity(cityName, city)) {
            sendJson(clientSock, "{\"error\":\"City not found\"}", 404, "Not Found");
            return;
        }
        sendJson(clientSock, weatherJson(city));
        return;
    }

    if (path == "/api/suggest") {
        std::string query = params.count("q") ? params.at("q") : "";
        sendJson(clientSock, stringArrayJson(engine.autocomplete(query, 8)));
        return;
    }

    if (path == "/api/hottest") {
        int k = parseIntParam(params, "k", 5);
        sendJson(clientSock, cityListJson(engine.getHottestCities(k), true));
        return;
    }

    if (path == "/api/coldest") {
        int k = parseIntParam(params, "k", 5);
        sendJson(clientSock, cityListJson(engine.getColdestCities(k), true));
        return;
    }

    if (path == "/api/alerts") {
        int k = parseIntParam(params, "k", 5);
        sendJson(clientSock, alertsJson(k));
        return;
    }

    if (path == "/api/route") {
        std::string from = params.count("from") ? params.at("from") : "";
        std::string to = params.count("to") ? params.at("to") : "";
        std::string mode = params.count("mode") ? params.at("mode") : "safe";

        if (from.empty() || to.empty()) {
            sendJson(clientSock, "{\"error\":\"Route requires from and to query params\"}", 400, "Bad Request");
            return;
        }

        if (mode == "bfs") {
            std::vector<std::string> path = engine.shortestRouteBfs(from, to);
            RouteResult result;
            result.found = !path.empty();
            result.path = path;
            sendJson(clientSock, routeJson(result, "BFS shortest hops"));
            return;
        }

        sendJson(clientSock, routeJson(engine.safestRouteDijkstra(from, to), "Dijkstra lowest weather risk"));
        return;
    }

    if (path == "/api/requests") {
        sendJson(clientSock, stringArrayJson(engine.recentRequests(10)));
        return;
    }

    sendJson(clientSock, "{\"error\":\"Unknown API endpoint\"}", 404, "Not Found");
}

void handleClient(SOCKET clientSock, const std::string& indexPath) {
    char buffer[4096];
    int bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        closesocket(clientSock);
        return;
    }

    std::string request(buffer, bytesReceived);
    std::stringstream requestStream(request);
    std::string method;
    std::string url;
    requestStream >> method >> url;
    engine.logRequest(method + " " + url);

    const std::string path = SimpleServer::pathOnly(url);
    const auto params = SimpleServer::parseQuery(url);

    if (path.rfind("/api/", 0) == 0 || path == "/data") {
        handleApi(clientSock, path, params);
    } else if (path == "/" || path == "/index.html") {
        std::string html = SimpleServer::loadTextFile(indexPath);
        if (html.empty()) {
            SimpleServer::sendResponse(clientSock, "<h1>public/index.html not found</h1>", "text/html", 500, "Server Error");
        } else {
            SimpleServer::sendResponse(clientSock, html, "text/html");
        }
    } else {
        SimpleServer::sendResponse(clientSock, "404 Not Found", "text/plain", 404, "Not Found");
    }

    closesocket(clientSock);
}

void stopServer(int) {
    running = false;
}
}

int main() {
    std::signal(SIGINT, stopServer);

    const std::string dataPath = findExistingPath({"data/weather_data.csv", "../data/weather_data.csv"});
    const std::string indexPath = findExistingPath({"public/index.html", "../public/index.html", "index.html"});

    if (dataPath.empty()) {
        std::cerr << "Could not find data/weather_data.csv" << std::endl;
        return 1;
    }

    std::string loadError;
    if (!engine.loadCitiesFromCsv(dataPath, &loadError)) {
        std::cerr << loadError << std::endl;
        return 1;
    }
    seedRoutesAndAlerts();

    if (!SimpleServer::initNetwork()) {
        std::cerr << "Failed to initialize network stack" << std::endl;
        return 1;
    }

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        std::cerr << "Failed to create server socket" << std::endl;
        SimpleServer::cleanupNetwork();
        return 1;
    }

    sockaddr_in serverAddr {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Could not bind to port 8080. Is another server already running?" << std::endl;
        closesocket(serverSock);
        SimpleServer::cleanupNetwork();
        return 1;
    }

    if (listen(serverSock, 10) == SOCKET_ERROR) {
        std::cerr << "Failed to listen on port 8080" << std::endl;
        closesocket(serverSock);
        SimpleServer::cleanupNetwork();
        return 1;
    }

    std::cout << "DSA Weather Analytics Dashboard running at http://localhost:8080" << std::endl;
    std::cout << "Loaded " << engine.getAllCities().size() << " cities from " << dataPath << std::endl;
    std::cout << "API examples: /api/weather?city=Lahore, /api/route?from=Topi&to=Karachi" << std::endl;

    while (running) {
        SOCKET clientSock = accept(serverSock, nullptr, nullptr);
        if (clientSock == INVALID_SOCKET) continue;
        handleClient(clientSock, indexPath);
    }

    closesocket(serverSock);
    SimpleServer::cleanupNetwork();
    return 0;
}
