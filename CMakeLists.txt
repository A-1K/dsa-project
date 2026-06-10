cmake_minimum_required(VERSION 3.16)

project(dsa_weather_dashboard VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(weather_engine INTERFACE)
target_include_directories(weather_engine INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(weather_dashboard src/main.cpp)
target_link_libraries(weather_dashboard PRIVATE weather_engine)

if (WIN32)
    target_compile_definitions(weather_dashboard PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(weather_dashboard PRIVATE ws2_32)
endif()

add_executable(weather_engine_tests tests/weather_engine_tests.cpp)
target_link_libraries(weather_engine_tests PRIVATE weather_engine)

enable_testing()
add_test(NAME weather_engine_tests COMMAND weather_engine_tests)
