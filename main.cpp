// Compile: g++ -std=c++17 main.cpp -o weather_app -lws2_32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
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
        if(cityName.empty()) cityName = "Topi"; 

        size_t pos;
        while ((pos = cityName.find("%20")) != string::npos) {
            cityName.replace(pos, 3, " ");
        }

        City* c = engine.getCity(cityName);
        
        if (c) {
            stringstream json;
            json << "{";
            json << "\"city\": \"" << c->name << "\",";
            json << "\"lat\": " << c->lat << ",";
            json << "\"lon\": " << c->lon << ",";
            json << "\"current\": {";
            json << "\"temperature_2d\": " << c->temp << ",";
            json << "\"wind_speed_10m\": " << c->wind << ",";
            json << "\"relative_humidity_2d\": " << c->humidity << ",";
            json << "\"rain\": 0,"; 
            json << "\"aqi\": 45,";
            json << "\"wind_dir\": " << c->wind_dir << ","; // <--- FIXED: Now uses real direction
            json << "\"condition\": \"" << c->condition << "\"";
            json << "},";
            
            // --- Serialize Time Series Vectors ---
            json << "\"hourly\": [";
            for(size_t i=0; i<c->hourlyData.size(); i++) json << c->hourlyData[i] << (i<c->hourlyData.size()-1?",":"");
            json << "],";

            json << "\"weekly\": [";
            for(size_t i=0; i<c->weeklyData.size(); i++) json << c->weeklyData[i] << (i<c->weeklyData.size()-1?",":"");
            json << "],";

            json << "\"monthly\": [";
            for(size_t i=0; i<c->monthlyData.size(); i++) json << c->monthlyData[i] << (i<c->monthlyData.size()-1?",":"");
            json << "],";

            json << "\"yearly\": [";
            for(size_t i=0; i<c->yearlyData.size(); i++) json << c->yearlyData[i] << (i<c->yearlyData.size()-1?",":"");
            json << "],";

            // --- Serialize Forecast Objects ---
            json << "\"forecast\": [";
            for(size_t i = 0; i < c->tenDayForecast.size(); i++) {
                json << "{";
                json << "\"day\": \"" << c->tenDayForecast[i].dayName << "\",";
                json << "\"high\": " << c->tenDayForecast[i].high << ",";
                json << "\"low\": " << c->tenDayForecast[i].low << ",";
                json << "\"rain_prob\": " << (c->tenDayForecast[i].condition == "Rainy" ? 80 : 10) << ",";
                json << "\"cond\": \"" << c->tenDayForecast[i].condition << "\"";
                json << "}";
                if (i < c->tenDayForecast.size() - 1) json << ",";
            }
            json << "],";

            // --- Graph Neighbors ---
            vector<string> neighbors = engine.getNeighbors(cityName);
            json << "\"neighbors\": [";
            for(size_t i=0; i<neighbors.size(); i++) {
                json << "\"" << neighbors[i] << "\"" << (i < neighbors.size()-1 ? "," : "");
            }
            json << "],";

            // --- Hottest Cities ---
            vector<City> hottest = engine.getHottestCities(5);
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
    // Helper: Forecasts
    vector<DailyForecast> standardFc = {
        {"Mon", 32, 24, "Sunny"}, {"Tue", 34, 25, "Sunny"}, {"Wed", 30, 22, "Cloudy"},
        {"Thu", 29, 21, "Rainy"}, {"Fri", 31, 23, "Sunny"}, {"Sat", 33, 24, "Clear"},
        {"Sun", 32, 24, "Cloudy"}, {"Mon", 35, 26, "Hot"},   {"Tue", 36, 27, "Hot"}, {"Wed", 30, 22, "Windy"}
    };
    vector<DailyForecast> hotFc = {
        {"Mon", 42, 30, "Hot"}, {"Tue", 43, 31, "Hot"}, {"Wed", 41, 29, "Sunny"},
        {"Thu", 40, 28, "Sunny"}, {"Fri", 42, 30, "Scorching"}, {"Sat", 44, 32, "Hot"},
        {"Sun", 43, 31, "Hot"}, {"Mon", 41, 29, "Sunny"},   {"Tue", 40, 28, "Clear"}, {"Wed", 39, 27, "Dry"}
    };

    // 1. TOPI (315 degrees NW)
    engine.addCity({"Topi", 34.07, 72.6, 21, 45, 15, 315, "Windy",
        {17,18,19,21,23,24,23,22,21,20,19,18,17,17,18,19,20,21,22,23,24,23,22,21},
        {19,21,20,22,23,21,20},
        {17,19,21,24,27,29,27,25,23,21,19,17,14,11,9,11,14,17,19,21,23,24,23,21,19,17,15,14,17,19},
        {11, 13, 18, 24, 29, 32, 31, 29, 27, 22, 17, 12}, standardFc
    });

    // 2. ISLAMABAD (45 degrees NE)
    engine.addCity({"Islamabad", 33.6, 73.0, 22, 50, 10, 45, "Rainy", 
        {18,19,20,22,24,25,24,23,22,21,20,19,18,18,19,20,21,22,23,24,25,24,23,22}, 
        {20,22,21,23,24,22,21}, 
        {18,20,22,25,28,30,28,26,24,22,20,18,15,12,10,12,15,18,20,22,24,25,24,22,20,18,16,15,18,20},
        {10, 13, 18, 24, 30, 32, 31, 29, 28, 24, 17, 12}, standardFc
    });

    // 3. SERAIKISTAN (180 degrees S)
    engine.addCity({"Seraikistan", 29.3, 71.6, 41, 15, 6, 180, "Scorching",
        {37,38,39,41,43,44,43,42,41,40,39,38,37,37,38,39,40,41,42,43,44,43,42,41},
        {39,41,40,42,43,41,40},
        {37,39,41,44,47,49,47,45,43,41,39,37,34,31,29,31,34,37,39,41,43,44,43,41,39,37,35,34,37,39},
        {14, 18, 24, 31, 36, 38, 36, 34, 33, 29, 22, 16}, hotFc
    });

    // 4. LAHORE (135 degrees SE)
    engine.addCity({"Lahore", 31.5, 74.3, 32, 40, 5, 135, "Sunny",
        {28,29,30,32,34,35,34,33,32,31,30,29,28,28,29,30,31,32,33,34,35,34,33,32},
        {30,32,31,33,34,32,31},
        {28,30,32,35,38,40,38,36,34,32,30,28,25,22,20,22,25,28,30,32,34,35,34,32,30,28,26,25,28,30},
        {13, 16, 22, 28, 33, 34, 32, 31, 30, 26, 20, 14}, standardFc
    });

    // 5. KARACHI (270 degrees W - Sea Breeze)
    engine.addCity({"Karachi", 24.8, 67.0, 35, 70, 20, 270, "Windy",
        {30,31,31,32,33,33,32,31,30,30,29,29,28,28,29,30,31,32,33,33,32,31,30,29},
        {32,33,32,33,34,33,32},
        {30,31,32,33,34,35,34,33,32,31,30,29,28,27,26,27,28,29,30,31,32,33,32,31,30,29,28,27,29,31},
        {19, 21, 25, 28, 31, 32, 30, 29, 29, 28, 25, 21}, standardFc
    });

     // 6. MULTAN (225 degrees SW)
    engine.addCity({"Multan", 30.1, 71.4, 38, 20, 8, 225, "Hot",
        {34,35,36,38,40,41,40,39,38,37,36,35,34,34,35,36,37,38,39,40,41,40,39,38},
        {36,38,37,39,40,38,37},
        {34,36,38,41,44,46,44,42,40,38,36,34,31,28,26,28,31,34,36,38,40,41,40,38,36,34,32,31,34,36},
        {13, 16, 22, 29, 34, 36, 34, 33, 32, 27, 20, 14}, hotFc
    });

    // 7. PESHAWAR (90 degrees E)
    engine.addCity({"Peshawar", 34.0, 71.5, 20, 55, 12, 90, "Cloudy", 
        {11, 13, 18, 24, 30, 33, 32, 30, 28, 23, 17, 12, 13, 14, 15, 16, 17, 18, 19, 18, 17, 16, 15, 14},
        {20,22,21,23,24,22,21},
        {18,20,22,25,28,30,28,26,24,22,20,18,15,12,10,12,15,18,20,22,24,25,24,22,20,18,16,15,18,20},
        {10, 13, 18, 24, 30, 32, 31, 29, 28, 24, 17, 12}, standardFc
    });

    // Graph Connections
    engine.addRoute("Islamabad", "Peshawar");
    engine.addRoute("Islamabad", "Lahore");
    engine.addRoute("Islamabad", "Topi"); 
    engine.addRoute("Peshawar", "Topi");
    engine.addRoute("Lahore", "Multan");
    engine.addRoute("Multan", "Seraikistan");
    engine.addRoute("Karachi", "Seraikistan");

    // Alerts
    engine.addAlert(10, "Heatwave", "Seraikistan");
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
    cout << "Backend: C++17 with Custom Graph & HashMaps." << endl;

    while (true) {
        SOCKET clientSock = accept(serverSock, nullptr, nullptr);
        thread(handleClient, clientSock).detach();
    }
    
    return 0;
}