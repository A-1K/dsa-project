#ifndef NETWORK_UTILS_HPP
#define NETWORK_UTILS_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

// Removed using namespace std;

class SimpleServer {
public:
    static void initWinsock() {
        #ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        #endif
    }

    // Reads the actual index.html file from disk!
    static std::string loadHtmlFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return "<h1>Error: index.html not found</h1>";
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    static void sendResponse(SOCKET clientSock, std::string body, std::string type = "text/html") {
        std::string response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: " + type + "\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Access-Control-Allow-Origin: *\r\n" // Allow CORS for local testing
            "Connection: close\r\n\r\n" + 
            body;
        
        send(clientSock, response.c_str(), response.size(), 0);
    }
    
    // Helper to parse query params (e.g., /data?city=Lahore)
    static std::string getQueryParam(std::string url, std::string key) {
        size_t start = url.find(key + "=");
        if (start == std::string::npos) return "";
        start += key.length() + 1;
        size_t end = url.find('&', start);
        if (end == std::string::npos) end = url.length();
        return url.substr(start, end - start);
    }
};

#endif