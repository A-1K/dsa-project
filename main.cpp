#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// ==========================================
// 1. LINKED LIST (10-Day Forecast)
// ==========================================
struct Node {
    string day; int high, low; Node* next;
    Node(string d, int h, int l) : day(d), high(h), low(l), next(nullptr) {}
};

class ForecastList {
public:
    Node* head;
    ForecastList() : head(nullptr) {}
    
    void add(string d, int h, int l) {
        Node* n = new Node(d, h, l);
        if(!head) head = n;
        else { Node* t = head; while(t->next) t=t->next; t->next = n; }
    }

    void generate10Days(int base) {
        string days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        for(int i=0; i<10; i++) {
            int h = base + (rand()%6) - 2; 
            int l = base - (rand()%6) - 5;
            add(days[i%7], h, l);
        }
    }
};

// ==========================================
// 2. BST (City Search)
// ==========================================
struct City { 
    string name; 
    double lat, lon; 
    int temp, wind, hum; 
    int aqi;        // Air Quality Index
    double rain;    // Rain in mm
    int wind_dir;   // Wind Direction (0-360)
    string cond; 
    ForecastList fc; 
};

struct BSTNode { City data; BSTNode *left, *right; BSTNode(City c) : data(c), left(nullptr), right(nullptr) {} };

class CityBST {
    BSTNode* root;
    BSTNode* insert(BSTNode* n, City c) {
        if(!n) return new BSTNode(c);
        if(c.name < n->data.name) n->left = insert(n->left, c);
        else n->right = insert(n->right, c);
        return n;
    }
    BSTNode* search(BSTNode* n, string name) {
        if(!n || n->data.name == name) return n;
        if(name < n->data.name) return search(n->left, name);
        return search(n->right, name);
    }
    void inorder(BSTNode* n, vector<City>& list) {
        if(!n) return;
        inorder(n->left, list);
        list.push_back(n->data);
        inorder(n->right, list);
    }
public:
    CityBST() : root(nullptr) {}
    void add(City c) { root = insert(root, c); }
    City* find(string n) { BSTNode* r = search(root, n); return r ? &r->data : nullptr; }
    vector<City> getAll() { vector<City> l; inorder(root, l); return l; }
};

CityBST tree;

// ==========================================
// 3. DATA LOADING
// ==========================================
void createCSV() {
    ofstream f("weather_data.csv");
    // New Header with 10 columns
    f << "City,Lat,Lon,Temp,Condition,Wind,Humidity,AQI,Rain,WindDir\n";
    // Data populated with AQI, Rain, and Wind Direction
    f << "Topi,34.07,72.63,12,Partly Cloudy,10,72,45,0.0,180\n";
    f << "Islamabad,33.68,73.04,18,Rainy,12,65,120,5.2,45\n";
    f << "Lahore,31.52,74.35,25,Sunny,8,40,350,0.0,90\n";
    f << "Karachi,24.86,67.00,30,Windy,25,70,150,0.0,220\n";
    f << "Seraikistan,29.35,71.69,32,Clear,5,20,90,0.0,135\n";
    f << "Peshawar,34.01,71.52,15,Cloudy,7,55,180,1.5,300\n";
    f << "Quetta,30.17,66.97,5,Snowy,15,40,30,12.0,10\n";
    f << "Multan,30.15,71.52,28,Hot,6,30,200,0.0,270\n";
    f.close();
    cout << "Data file updated to new format." << endl;
}

void loadData() {
    ifstream f("weather_data.csv");
    string line; 
    
    // --- SMART CHECK: REGENERATE IF OLD FORMAT ---
    bool regenerate = false;
    if(!f.good()) {
        regenerate = true;
    } else {
        getline(f, line); // Read header
        // If header doesn't contain "AQI", it's the old file
        if(line.find("AQI") == string::npos) regenerate = true;
    }

    if(regenerate) {
        f.close();
        createCSV(); // Force overwrite
        f.open("weather_data.csv");
        getline(f, line); // Skip new header
    }
    // ---------------------------------------------

    while(getline(f, line)) {
        stringstream ss(line); string item; vector<string> row;
        while(getline(ss, item, ',')) row.push_back(item);
        
        // Only load if we have all 10 columns
        if(row.size() >= 10) {
            City c;
            c.name = row[0]; 
            c.lat = stod(row[1]); 
            c.lon = stod(row[2]);
            c.temp = stoi(row[3]); 
            c.cond = row[4]; 
            c.wind = stoi(row[5]); 
            c.hum = stoi(row[6]);
            c.aqi = stoi(row[7]);
            c.rain = stod(row[8]);
            c.wind_dir = stoi(row[9]);

            c.fc.generate10Days(c.temp);
            tree.add(c);
        }
    }
}

// ==========================================
// 4. JSON GENERATOR
// ==========================================
string getJSON(string name) {
    City* c = tree.find(name);
    // Safety check: if Topi isn't found, tree might be empty
    if(!c) {
        // Try to get root if specific city fails
        vector<City> all = tree.getAll();
        if(!all.empty()) c = &all[0];
        else return "{}"; // Empty JSON if no data
    }

    stringstream ss;
    ss << "{ \"city\": \"" << c->name << "\", \"lat\": " << c->lat << ", \"lon\": " << c->lon << ",";
    ss << "\"current\": {";
    ss << "\"temperature_2d\": " << c->temp << ",";
    ss << "\"condition\": \"" << c->cond << "\",";
    ss << "\"wind_speed_10m\": " << c->wind << ",";
    ss << "\"relative_humidity_2d\": " << c->hum << ",";
    ss << "\"aqi\": " << c->aqi << ",";
    ss << "\"rain\": " << c->rain << ",";
    ss << "\"wind_dir\": " << c->wind_dir;
    ss << " },";

    ss << "\"forecast\": [";
    Node* curr = c->fc.head;
    while(curr) {
        ss << "{\"day\": \"" << curr->day << "\", \"high\": " << curr->high << ", \"low\": " << curr->low << "}";
        if(curr->next) ss << ",";
        curr = curr->next;
    }
    ss << "],";

    vector<City> all = tree.getAll();
    for(size_t i=0; i<all.size()-1; i++) {
        for(size_t j=0; j<all.size()-i-1; j++) {
            if(all[j].temp < all[j+1].temp) swap(all[j], all[j+1]);
        }
    }

    ss << "\"hottest_cities\": [";
    for(size_t i=0; i<all.size(); i++) {
        ss << "{\"name\": \"" << all[i].name << "\", \"temp\": " << all[i].temp << "}";
        if(i < all.size()-1) ss << ",";
    }
    ss << "] }";
    
    return ss.str();
}

// ==========================================
// 5. SERVER
// ==========================================
string loadHTML() {
    ifstream f("index.html");
    if(f) { stringstream b; b << f.rdbuf(); return b.str(); }
    return "<h1>Error: Missing index.html</h1>";
}

void handle(SOCKET s) {
    char buf[4096] = {0};
    int val = recv(s, buf, 4096, 0);
    if(val > 0) {
        string req(buf);
        if(req.find("GET /data") != string::npos) {
            string name = "Topi";
            size_t p = req.find("city=");
            if(p != string::npos) {
                size_t e = req.find(" ", p);
                name = req.substr(p+5, e-(p+5));
            }
            string json = getJSON(name);
            string resp = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + json;
            send(s, resp.c_str(), resp.length(), 0);
        } else if(req.find("GET / ") != string::npos || req.find("GET /index.html") != string::npos) {
            string html = loadHTML();
            string resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + html;
            send(s, resp.c_str(), resp.length(), 0);
        }
    }
    closesocket(s);
}

int main() {
    loadData();
    WSADATA w; WSAStartup(MAKEWORD(2,2), &w);
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a; a.sin_family=AF_INET; a.sin_port=htons(8080); a.sin_addr.s_addr=INADDR_ANY;
    bind(s, (sockaddr*)&a, sizeof(a));
    listen(s, 5);
    cout << "SERVER RUNNING: http://localhost:8080" << endl;
    while(true) {
        SOCKET c = accept(s, 0, 0);
        if(c != INVALID_SOCKET) thread(handle, c).detach();
    }
    return 0;
}