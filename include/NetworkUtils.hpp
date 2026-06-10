#ifndef NETWORK_UTILS_HPP
#define NETWORK_UTILS_HPP

#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

class SimpleServer {
public:
    static bool initNetwork() {
#ifdef _WIN32
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
        return true;
#endif
    }

    static void cleanupNetwork() {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    static std::string loadTextFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return "";
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    static void sendResponse(SOCKET clientSock,
                             const std::string& body,
                             const std::string& type = "text/html",
                             int statusCode = 200,
                             const std::string& statusText = "OK") {
        std::string response =
            "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n"
            "Content-Type: " + type + "; charset=utf-8\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n\r\n" +
            body;

        send(clientSock, response.c_str(), static_cast<int>(response.size()), 0);
    }

    static std::string urlDecode(const std::string& value) {
        std::ostringstream decoded;
        for (size_t i = 0; i < value.size(); i++) {
            if (value[i] == '%' && i + 2 < value.size()) {
                std::string hex = value.substr(i + 1, 2);
                char ch = static_cast<char>(std::stoi(hex, nullptr, 16));
                decoded << ch;
                i += 2;
            } else if (value[i] == '+') {
                decoded << ' ';
            } else {
                decoded << value[i];
            }
        }
        return decoded.str();
    }

    static std::unordered_map<std::string, std::string> parseQuery(const std::string& url) {
        std::unordered_map<std::string, std::string> params;
        size_t queryStart = url.find('?');
        if (queryStart == std::string::npos) return params;

        std::string query = url.substr(queryStart + 1);
        std::stringstream stream(query);
        std::string pair;
        while (std::getline(stream, pair, '&')) {
            size_t eq = pair.find('=');
            if (eq == std::string::npos) continue;
            std::string key = urlDecode(pair.substr(0, eq));
            std::string value = urlDecode(pair.substr(eq + 1));
            params[key] = value;
        }
        return params;
    }

    static std::string pathOnly(const std::string& url) {
        size_t queryStart = url.find('?');
        return queryStart == std::string::npos ? url : url.substr(0, queryStart);
    }
};

#endif
