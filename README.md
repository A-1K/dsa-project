# DSA Weather Analytics Dashboard

A portfolio-ready C++17 weather analytics dashboard that demonstrates practical data structures and algorithms through a small REST-style weather server and an interactive browser UI.

![Dashboard preview](docs/screenshots/dashboard.png)

## Why This Project Exists

This started as a DSA class project and has been upgraded into a GitHub/resume project. The goal is not to compete with production weather APIs, but to show that core DSA concepts can power a real, inspectable application: city lookup, route planning, weather ranking, alert prioritization, autocomplete, and request history.

## Features

- C++17 HTTP server with REST-style API endpoints.
- CSV-backed city weather dataset in `data/weather_data.csv`.
- Interactive dashboard in `public/index.html`.
- City weather lookup with forecast, humidity, wind, AQI, rain, and temperature trends.
- Graph-based route planner:
  - BFS for fewest route hops.
  - Dijkstra for lowest weather-risk route.
- Trie-backed city autocomplete.
- Priority queue weather alerts.
- Partial-sort hottest/coldest city rankings.
- Lightweight C++ tests for the core engine.
- Reproducible dashboard preview generator.

## DSA Concepts Used

| Concept | Where It Appears | Purpose |
| --- | --- | --- |
| Hash map | `cityDatabase` | O(1) average city lookup by name |
| Graph adjacency list | `cityGraph` | Stores city-to-city weather routes |
| BFS | `shortestRouteBfs` | Finds the route with the fewest hops |
| Dijkstra | `safestRouteDijkstra` | Finds the lowest weather-risk route |
| Priority queue / max heap | `alertSystem` | Returns most severe alerts first |
| Trie | `autocomplete` | Fast prefix search for city names |
| Vector | Time-series weather arrays | Stores hourly, weekly, monthly, yearly trends |
| Partial sort | `getHottestCities`, `getColdestCities` | Efficient top-k ranking |
| Stack-style log | `requestLogStack` | Keeps recent server requests |

## Project Structure

```text
.
|-- CMakeLists.txt
|-- data/
|   `-- weather_data.csv
|-- docs/
|   `-- screenshots/
|       `-- dashboard.png
|-- include/
|   |-- NetworkUtils.hpp
|   `-- WeatherEngine.hpp
|-- public/
|   `-- index.html
|-- src/
|   `-- main.cpp
|-- tests/
|   `-- weather_engine_tests.cpp
`-- tools/
    `-- generate_dashboard_preview.ps1
```

## Build And Run

### Prerequisites

- C++17 compiler
- CMake 3.16+

On Windows, install Visual Studio Build Tools or MinGW-w64. On macOS/Linux, install Clang or GCC.

### Build

```bash
cmake -S . -B build
cmake --build build
```

### Run The Dashboard

```bash
./build/weather_dashboard
```

On Windows, the executable may be:

```powershell
.\build\Debug\weather_dashboard.exe
```

Then open:

```text
http://localhost:8080
```

## Run Tests

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The tests cover CSV loading, city lookup, Trie autocomplete, graph neighbors, BFS, Dijkstra, priority queue alerts, rankings, and request logging.

## API Endpoints

```text
GET /api/cities
GET /api/weather?city=Lahore
GET /api/suggest?q=is
GET /api/hottest?k=5
GET /api/coldest?k=3
GET /api/alerts?k=5
GET /api/route?from=Topi&to=Karachi&mode=safe
GET /api/route?from=Topi&to=Karachi&mode=bfs
GET /api/requests
```

Example route response:

```json
{
  "found": true,
  "algorithm": "Dijkstra lowest weather risk",
  "path": ["Topi", "Islamabad", "Lahore", "Multan", "Quetta", "Karachi"],
  "hops": 5,
  "total_risk": 21,
  "total_distance_km": 1120
}
```

## Regenerate The Preview Image

The README preview image can be regenerated on Windows with:

```powershell
powershell -ExecutionPolicy Bypass -File tools\generate_dashboard_preview.ps1
```

The live dashboard itself is in `public/index.html` and uses the C++ API when the server is running. If opened directly as a file, it falls back to static demo data so the UI can still be viewed.

## Resume Bullets

- Built a C++17 weather analytics dashboard with a custom HTTP server, CSV-backed dataset, and interactive browser UI.
- Implemented hash maps, graphs, BFS, Dijkstra, priority queues, Trie autocomplete, vectors, stack-style logging, and partial sorting to power application features.
- Designed REST-style endpoints for city lookup, route planning, weather alerts, rankings, autocomplete, and request history.
- Added CMake build support, focused engine tests, and GitHub-ready documentation with architecture and complexity notes.

## Future Improvements

- Add real weather API ingestion behind the CSV loader.
- Persist request logs and alerts to a lightweight database.
- Add GitHub Actions CI for CMake build and tests.
- Add route visualization lines to the dashboard network panel.
