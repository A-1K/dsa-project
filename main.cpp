// Compile command: 
// g++ -std=c++17 main.cpp -o weather_app -lws2_32

#include "WeatherEngine.hpp"
#include "NetworkUtils.hpp"
#include <thread>
#include <iostream>

using namespace std;

// Global Engine Instance
WeatherEngine engine;

void handleClient(SOCKET clientSock) {
    char buffer[4096];
    int bytesReceived = recv(clientSock, buffer, 4096, 0);
    if (bytesReceived <= 0) {
        closesocket(clientSock);
        return;
    }

    string request(buffer, bytesReceived);
    stringstream ss(request);
    string method, url;
    ss >> method >> url;

    engine.logRequest(method + " " + url);

    if (url == "/" || url == "/index.html") {
        string html = SimpleServer::loadHtmlFile("index.html");
        SimpleServer::sendResponse(clientSock, html, "text/html");
    }
    else if (url.find("/data") != string::npos) {
        string cityName = SimpleServer::getQueryParam(url, "city");
        if(cityName.empty()) cityName = "Islamabad"; 

        // Handle spaces in URL (e.g. Seraikistan or names with spaces)
        size_t pos;
        while ((pos = cityName.find("%20")) != string::npos) {
            cityName.replace(pos, 3, " ");
        }

        City* c = engine.getCity(cityName);
        
        if (c) {
            stringstream json;
            json << "{";
            json << "\"city\": \"" << c->name << "\",";
            json << "\"current\": {";
            json << "\"temperature_2d\": " << c->temp << ",";
            json << "\"wind_speed_10m\": " << c->wind << ",";
            json << "\"relative_humidity_2d\": " << c->humidity << ",";
            json << "\"weather_code\": 1"; 
            json << "},";
            
            vector<string> neighbors = engine.getNeighbors(cityName);
            json << "\"neighbors\": [";
            for(size_t i=0; i<neighbors.size(); i++) {
                json << "\"" << neighbors[i] << "\"" << (i < neighbors.size()-1 ? "," : "");
            }
            json << "],";

            vector<City> hottest = engine.getHottestCities(5); // Increased to 5 to show new cities
            json << "\"hottest_cities\": [";
            for(size_t i=0; i<hottest.size(); i++) {
                json << "{\"name\": \"" << hottest[i].name << "\", \"temp\": " << hottest[i].temp << "}" << (i < hottest.size()-1 ? "," : "");
            }
            json << "]";

            json << "}";
            SimpleServer::sendResponse(clientSock, json.str(), "application/json");
        } else {
            SimpleServer::sendResponse(clientSock, "{}", "application/json");
        }
    }
    else {
        SimpleServer::sendResponse(clientSock, "404 Not Found", "text/plain");
    }

    closesocket(clientSock);
}

void loadDummyData() {
    // --- EXISTING CITIES ---
    engine.addCity({"Islamabad", 33.6, 73.0, 22, 50, 10, "Rainy"});
    engine.addCity({"Lahore", 31.5, 74.3, 32, 40, 5, "Sunny"});
    engine.addCity({"Karachi", 24.8, 67.0, 35, 70, 20, "Windy"});
    engine.addCity({"Peshawar", 34.0, 71.5, 20, 55, 12, "Cloudy"});
    engine.addCity({"Multan", 30.1, 71.4, 38, 20, 8, "Hot"});

    // --- NEW CITIES ADDED ---
    
    // 6. Topi (GIKI Region) - Pleasant/Windy
    engine.addCity({"Topi", 34.07, 72.6, 21, 45, 15, "Clear"});

    // 7. Seraikistan (Region/South Punjab) - Hot/Dry
    engine.addCity({"Seraikistan", 29.3, 71.6, 41, 15, 6, "Scorching"});

    // --- GRAPH ROUTES (DSA: Adjacency List) ---
    engine.addRoute("Islamabad", "Peshawar");
    engine.addRoute("Islamabad", "Lahore");
    engine.addRoute("Lahore", "Multan");
    engine.addRoute("Multan", "Karachi");

    // NEW GRAPH CONNECTIONS
    // Topi is connected to Islamabad and Peshawar
    engine.addRoute("Islamabad", "Topi"); 
    engine.addRoute("Peshawar", "Topi");

    // Seraikistan (South) connected to Multan and Karachi
    engine.addRoute("Multan", "Seraikistan");
    engine.addRoute("Karachi", "Seraikistan");

    // --- ALERTS ---
    engine.addAlert(10, "Heatwave Warning", "Seraikistan"); // High Priority
    engine.addAlert(5, "High Winds", "Topi");
    engine.addAlert(8, "Urban Flood", "Karachi");
}

int main() {
    SimpleServer::initWinsock();
    loadDummyData(); 

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSock, 10);

    cout << "DSA Weather Server running on http://localhost:8080" << endl;
    cout << "Added Topi and Seraikistan to Graph Database..." << endl;

    while (true) {
        SOCKET clientSock = accept(serverSock, nullptr, nullptr);
        thread(handleClient, clientSock).detach();
    }
    
    return 0;
}